#include "traceroute.h"

#define buflen 24

char *device;
uint16_t id, seq = 0;
int probe = 3;
sigjmp_buf jumpbuf;
struct sockaddr_in from;
struct sockaddr_in to;

static void send_icmp(int fd, uint16_t id, uint16_t seq, struct sockaddr *dest_addr, socklen_t dest_len)
{
        char buf[MAXLINE], data[buflen];

        memset(data, 0x80, buflen);

        icmp_build_echo((u_char *)buf, buflen, 8, 0, id, seq, (u_char *)data);

        if (sendto(fd, buf, 8+buflen, 0, dest_addr, dest_len) != 8+buflen)
                err_sys("sendto ICMP echo request error");

        return;
}


static void tvsub(struct timeval *out, struct timeval *in)
{
        if ((out->tv_usec -= in->tv_usec) < 0) {
                out->tv_sec--;
                out->tv_usec += 1000000;
        }
        out->tv_sec -= in->tv_sec;
}

static float deltaT(struct timeval *tp)
{
        struct timeval tv;
        if (gettimeofday(&tv, (struct timezone *)NULL) < 0)
                err_sys("gettimeofday error");
        tvsub(&tv, tp);
        return (tv.tv_sec*1000.0 + (tv.tv_usec+500.0)/1000.0);
}


static void 
sig_alrm(int signo)
{
	siglongjmp(jumpbuf, 3);
#if defined(_AIX) || defined(_AIX64)
	if (signal(SIGALRM, sig_alrm) == SIG_ERR)
		err_sys("signal error");
#endif
}

static int icmp_icmp_check(u_char *buf, int len)
{
        /*
         * return code:
         *   0) Not expect ICMP packet or error;
         *   1) ICMP Time Exceeded
         *   2) ICMP Echo Reply
         *
         */
        const struct ip *ip;
        const struct ip *origin_ip;
        const struct icmp *icmp;
        const struct icmp *origin_icmp;
        int ip_hl;
        int icmplen;
        int origin_ip_hl;

        ip = (struct ip *)buf;
        if (ip->ip_p != IPPROTO_ICMP) {
                return 0;
        }

        ip_hl = ip->ip_hl << 2;
        if (ip_hl < 20) {
                return 0;
        }
        
        icmp = (struct icmp *)(buf + ip_hl);
        if ((icmplen = len - ip_hl) < 8 + 8) {
                /* The option data must > 8 bytes */
                return 0;
        }

        if (icmp->icmp_type == 11 && icmp->icmp_code == 0) {
                origin_ip = (struct ip *)(buf + ip_hl + 8);
                origin_ip_hl = origin_ip->ip_hl << 2;
                if (icmplen < 8 + origin_ip_hl + 8)
                        return 0;

                origin_icmp = (struct icmp *)(buf + ip_hl + 8 + origin_ip_hl);
                if (origin_ip->ip_p == IPPROTO_ICMP &&
                        !memcmp(&origin_ip->ip_dst, &to.sin_addr, sizeof(struct in_addr)) &&
                        /* Don't check the source IP address, because it doesn't bind an address */
                        origin_icmp->icmp_id == htons(id) &&
                        origin_icmp->icmp_seq == htons(seq))
                {
                        return 1;
                }
        } else if (icmp->icmp_type == 0 &&
                        !memcmp(&ip->ip_src, &to.sin_addr, sizeof(struct in_addr)) &&
                        icmp->icmp_id == htons(id) &&
                        icmp->icmp_seq == htons(seq))
        {
               return 2;
        } 
        return 0;
}


int 
main(int argc, char *argv[])
{
	char recvbuf[MAXLINE];
	struct in_addr lastrecv;
        struct sockaddr_in recv;
	int i, sendfd, rawfd, ttl, n, ret;
        const int size = 60 * 1024;
        socklen_t len;
        struct timeval sendtime;

	if (argc != 2) {
		err_quit("Usage: %s <IPaddr>", basename(argv[0]));
	}

	if ((rawfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_sys("socket error");

        if ((sendfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
                err_sys("socket SOCK_DGRAM error");

        setuid(getuid()); /* don't need special permissions anymore */
        setsockopt(rawfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	id = getpid() & 0xffff;

        /* Prepare destination sockaddr */
        bzero(&to, sizeof(struct sockaddr_in));
        to.sin_family = AF_INET;
        xgethostbyname(argv[1], &to.sin_addr);

	if (signal(SIGALRM, sig_alrm) == SIG_ERR)
		err_sys("signal error");

	setvbuf(stdout, NULL, _IONBF, 0);

	bzero(&lastrecv, sizeof(struct in_addr));

        printf("traceroute to %s (%s), 30 hops max, 52 bytes packets\n",
                        argv[1], inet_ntoa(to.sin_addr));

	for (ttl = 1; ttl <= 30; ttl++) {
		printf("%2d", ttl);
                if (setsockopt(sendfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int)) < 0)
                        err_sys("setsockopt IP_TTL error");
		for (i = 0; i < probe; i++, seq++) {
                        if (gettimeofday(&sendtime, (struct timezone *)NULL) < 0)
                                err_sys("gettimeofday sendtime error");
			send_icmp(sendfd, id, seq, (struct sockaddr *)&to, sizeof(to));

			alarm(WAITTIME);
			if (sigsetjmp(jumpbuf, 1) != 0) {
				printf(" *");
				goto endloop;
			}

			for (;;) {
                                len = sizeof(recv);
                                if ((n = recvfrom(rawfd, recvbuf, sizeof(recvbuf), 0,
                                                        (struct sockaddr *)&recv, &len)) < 0) {
                                        if (errno == EINTR)
                                                continue;
                                        else
                                                err_sys("recvfrom error");
                                }
                                if ((ret = icmp_icmp_check((u_char *)recvbuf, n)) == 1) {
                                        if (memcmp(&recv.sin_addr, &lastrecv, sizeof(struct in_addr))) {
                                                printf(" %s", inet_ntoa(recv.sin_addr));
                                                memmove(&lastrecv, &recv.sin_addr, sizeof(struct in_addr));
                                        }
                                        printf(" (%.3f ms)", deltaT(&sendtime));
                                        break;
                                } else if (ret == 2) {
                                        printf(" %s (%.3f ms)\n", inet_ntoa(recv.sin_addr), deltaT(&sendtime));
                                        goto end;
                                }
			}
	endloop:
			alarm(0);
		}
		printf("\n");
	}

end:
	return 0;
}
