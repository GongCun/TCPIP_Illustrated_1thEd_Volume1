#include "tcpi.h"

#define NROUTES 9 /* number of record route slots */

static uint16_t id, seq;
static int buflen = 56;
int fd, rroute = 0;
struct sockaddr *sockaddr;

static void usage(const char *str)
{
        err_quit("%s [-s packetsize] [-R] Host", str);
}

static void send_icmp(int fd, uint16_t id, uint16_t seq, struct sockaddr *dest_addr, socklen_t dest_len)
{
        char buf[MAXLINE], data[MAXLINE];
        struct timeval *tv;
        
        if (buflen < sizeof(struct timeval))
                buflen = sizeof(struct timeval);

        memset(data, 0x80, buflen);

        tv = (struct timeval *)data;

        if (gettimeofday(tv, NULL) < 0)
                err_sys("gettimeofday error");
        
        icmp_build_echo((u_char *)buf, buflen, 8, 0, id, seq, (u_char *)data);

        if (sendto(fd, buf, 8+buflen, 0, dest_addr, dest_len) != 8+buflen)
                err_sys("sendto ICMP echo request error");
        alarm(1);

        return;
}

static void sig_alrm(int signo)
{
        send_icmp(fd, id, seq++, sockaddr, sizeof(*sockaddr));
        return;
}

static void proc_icmp(int fd, uint16_t id);

static void ipopt_init(u_char *buf)
{
        u_char *optr = buf;
        *optr++ = 1; /* no operation */
        *optr++ = 7; /* record packet route */
        *optr++ = 3 + 4*NROUTES; /* max to 9 addresses */
        *optr++ = 4; /* offset to first address */

        return;
}

static void ipopt_print(u_char *buf)
{
        u_char *ptr, ch;
        ptr = buf + 20;
        int len;
        char str[32];
        static char old_rr[MAXLINE];
        static int old_rrlen = 0;

        while ((ch = *ptr++) == 1)
                ; /* skip any leading NOPs */
        if (ch != 7)
               return; 
        len = *ptr++ - 3;
        ptr++; /* skip over pointer */

        if (len == old_rrlen && memcmp((char *)ptr, old_rr, len) == 0) {
                printf("(same route) ");
                return;
        }
        old_rrlen = len;
        memmove(old_rr, (char *)ptr, len);
        printf("(RR:");
        while (len > 0 && *ptr != 0) {
                printf(" %s", inet_ntop(AF_INET, ptr, str, sizeof(str)));
                ptr += sizeof(struct in_addr);
                len -= sizeof(struct in_addr);
        }
        printf(") ");

}

int main(int argc, char *argv[])
{
        int sockfd, recvfd, size, ch;
        struct hostent *hptr;
        struct sockaddr_in to;
        char **pptr;

#ifdef _DEBUG
        printf("pid = %ld\n", (long)getpid());
#endif

        opterr = 0; /* don't want getopt() writing to stderr */
        while ((ch = getopt(argc, argv, "s:R")) != -1)
                switch(ch) {
                        case 's':
                                buflen = atoi(optarg);
                                break;
                        case 'R':
                                rroute = 1;
                                break;
                        default:
                                usage(basename(argv[0]));
                }

        if (optind != argc-1)
                usage(basename(argv[0]));

        bzero(&to, sizeof(struct sockaddr_in));
        if ((hptr = gethostbyname(argv[optind])) == NULL) {
                if (h_errno == HOST_NOT_FOUND) {
                        /* Check if a dotted-decimal string */
                        if (inet_pton(AF_INET, argv[optind], &to.sin_addr) != 1) {
                                errno = EINVAL;
                                err_sys("The address %s error", argv[optind]);
                        } else
                                printf("inet_pton(): address is %s\n", inet_ntoa(to.sin_addr));
                }
        } else {
                pptr = hptr->h_addr_list;
#ifdef _DEBUG
                printf("gethostbyname(): address is %s\n", inet_ntoa(*((struct in_addr *)(*pptr))));
#endif
                memmove(&to.sin_addr, (struct in_addr *)(*pptr), sizeof(struct in_addr));
        }
        
        to.sin_family = AF_INET;

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
                err_sys("socket sockfd error");
        if ((recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
                err_sys("socket recvfd error");
        setuid(getuid());       /* don't need special permissions any more */

        size = 60 * 1024;       /* OK if setsockopt fails */
        setsockopt(recvfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

        if (rroute == 1) {
                char optspace[40];
                bzero(optspace, sizeof(optspace));
                ipopt_init((u_char *)optspace);
                if (setsockopt(sockfd, IPPROTO_IP, IP_OPTIONS, optspace, sizeof(optspace)) < 0)
                        err_sys("setsockopt IP_OPTIONS error");
        }

        id = getpid() & 0xffff;
        seq = 0;
        fd = sockfd;
        sockaddr = (struct sockaddr *)&to;

        if (signal(SIGALRM, sig_alrm) == SIG_ERR)
                err_sys("signal SIGALRM error");

        if (kill(getpid(), SIGALRM) < 0)
                err_sys("kill error");

        for (;;) {
                proc_icmp(recvfd, id);
        }

        return 0;
}

static void proc_icmp(int recvfd, uint16_t id)
{
        char buf[MAXLINE];
        struct ip *ip;
        struct icmp *icmp;
        u_char *ptr;
        int len, n;
        struct sockaddr_in from;
        struct timeval *tv, now;
        float delta;

        len = sizeof(struct sockaddr_in);
        n = icmp_recv(recvfd, (u_char *) buf, sizeof(buf), (struct sockaddr *)&from, (socklen_t *) & len, &ptr);

        icmp = (struct icmp *)ptr;
        if (icmp->icmp_id != htons(id) || icmp->icmp_type != 0 || n < 8+sizeof(struct timeval))
                return;
        ip = (struct ip *)buf;
        ipopt_print((u_char *)buf);
        tv = (struct timeval *)(ptr + 8);
        if (gettimeofday(&now, NULL) < 0)
                err_sys("gettimeofday error");
        delta = (now.tv_sec*1000.0 + now.tv_usec/1000.0) - (tv->tv_sec*1000.0 + tv->tv_usec/1000.0);

        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                        n, inet_ntoa(from.sin_addr), ntohs(icmp->icmp_seq), ip->ip_ttl, delta);
        return;
}

