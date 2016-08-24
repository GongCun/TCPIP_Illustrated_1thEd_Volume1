#include "traceroute.h"

#define OPTLEN 44
#define MAXSRR 9
#define LSRR 0x83
#define SSRR 0x89

char *device;
int Sport;
int Dport = 32768 + 666;
int probe = 3;
sigjmp_buf jumpbuf;
struct sockaddr_in from;
struct sockaddr_in to;
u_char *optr = NULL;

static void icmp_srr_print(u_char *buf)
{
	struct ip *ip;
	struct ip *originip;
	int ip_hl;
	int len;
	u_char *ptr;

	ip = (struct ip *)buf;
	ip_hl = ip->ip_hl * 4;

	originip = (struct ip *)(buf + ip_hl + 8);
	if (originip->ip_hl * 4 <= 20)
		return;

	ptr = buf + ip_hl + 8 + 20;
	while (*ptr == 1) {
		ptr++;
	}
        if (*ptr != 0x83 || *ptr == 0x89)
                return;
	printf("0x%02x ", *ptr++);
	len = *ptr - 3;
	for (ptr+=2 ; len>0; ptr+=4, len-=4)
		printf("%s ", inet_ntoa(*(struct in_addr *)ptr));
}

static void srr_init(u_char *buf, u_char type)
{
        optr = buf;

        *optr++ = 1;
        *optr++ = type;
        *optr++ = 3;
        *optr++ = 4;

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

static int icmp_udp_check(u_char *buf, int len)
{
        /*
         * return code:
         *   0) Not expect ICMP packet or error;
         *   1) ICMP Time Exceeded
         *   2) ICMP Port Unreachable
         *   3) ICMP Source Route failed
         */
        const struct ip *ip;
        const struct ip *originip;
        const struct icmp *icmp;
        const struct udphdr *udp;
        int ip_hl;
        int icmplen;
        int originip_hl;

        ip = (struct ip *)buf;
        if (ip->ip_p != IPPROTO_ICMP) {
                return 0;
        }

        ip_hl = ip->ip_hl << 2;
        if (ip_hl < 20) {
                return 0;
        }
        
        icmp = (struct icmp *)(buf + ip_hl);
        if ((icmplen = len - ip_hl) < 8 + sizeof(struct ip)) {
                return 0;
        }
        originip = (struct ip *)(buf + ip_hl + 8);
        originip_hl = originip->ip_hl << 2;
        if (icmplen < 8 + originip_hl + 8)
                return 0;
        udp = (struct udphdr *)(buf + ip_hl + 8 + originip_hl);
        if (originip->ip_p == IPPROTO_UDP &&
                        /*
                         * - Don't check the destination IP address, because it maybe the gateway address
                         * if set the source and record routes option
                         * - Don't check the source IP address, because it doesn't bind an address
                         */
                        udp->uh_sport == htons(Sport) &&
                        udp->uh_dport == htons(Dport))
        {
                if (icmp->icmp_type == 11 &&
                                icmp->icmp_code == 0) {
                        return 1;
                } else if (icmp->icmp_type == 3 &&
                                icmp->icmp_code == 3) {
                        return 2;
                } else if (icmp->icmp_type == 3 &&
				icmp->icmp_code == 5) {
			return 3;
		} else {
                        return 0;
                }
        }
        return 0;
}


int 
main(int argc, char *argv[])
{
	char buf[24], recvbuf[MAXLINE];
	struct in_addr lastrecv;
        struct sockaddr_in recv;
	int i, sendfd, rawfd, ttl, n, ret;
        socklen_t len;
        struct timeval sendtime;
        int ch;
        int srr = 0;
        int optlen = 0;
        u_char optspace[OPTLEN], *ptr;

        bzero(optspace, sizeof(optspace));

        opterr = 0;
        optind = 1;

        while ((ch = getopt(argc, argv, "g:G:")) != -1)
                switch(ch) {
                        case 'G':
                        case 'g':
                        {
                                if (optr == NULL) {
                                        if (ch == 'g')
                                                srr_init(optspace, LSRR);
                                        else
                                                srr_init(optspace, SSRR);
                                }
                                if (++srr > MAXSRR) {
                                        err_quit("too many source routes");
                                }
                                xgethostbyname(optarg, (struct in_addr *)optr);
                                optr += sizeof(struct in_addr);
                                break;
                        }
                        default:
                        {
                                err_quit("Usage: %s [-g|-G] gateway host", basename(argv[0]));
                        }
                }

        if (optind != argc-1)
                err_quit("Usage: %s [-g|-G] gateway host", basename(argv[0]));

	if ((rawfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_sys("socket error");
        setuid(getuid()); /* don't need special permissions anymore */

        if ((sendfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                err_sys("socket SOCK_DGRAM error");

	Sport = (getpid() & 0xffff) | 0x8000; /* our source UDP port */

        /* Bind source port */
        bzero(&from, sizeof(struct sockaddr_in));
        from.sin_family = AF_INET;
        from.sin_port = htons(Sport);
        from.sin_addr.s_addr = htons(0);
        if (bind(sendfd, (struct sockaddr *)&from, sizeof(from)) < 0)
                err_sys("bind error");

        /* Prepare destination sockaddr */
        bzero(&to, sizeof(struct sockaddr_in));
        to.sin_family = AF_INET;
        xgethostbyname(argv[argc-1], &to.sin_addr);

        if (srr > 0) {
                if (++srr > MAXSRR+1)
                        err_quit("too many source routes");
                memmove((struct in_addr *)optr, &to.sin_addr, sizeof(struct in_addr));
                optspace[2] = 3 + srr*4;
                optlen = optspace[2] + 1;
                if (setsockopt(sendfd, IPPROTO_IP, IP_OPTIONS, optspace, optlen) < 0)
                        err_sys("setsockopt error");
                printf("code = 0x%02x, len = %d, ptr = %d\n",
                                optspace[1], optspace[2], optspace[3]);
                for (ptr=optspace+4; ptr<=optr; ptr+=4) {
                        printf("%s ", inet_ntoa(*(struct in_addr *)ptr));
                }
                printf("\n");
        }

	if (signal(SIGALRM, sig_alrm) == SIG_ERR)
		err_sys("signal error");

	setvbuf(stdout, NULL, _IONBF, 0);

        printf("traceroute to %s (%s), 30 hops max, 52 bytes packets\n",
                        argv[argc-1], inet_ntoa(to.sin_addr));

	for (ttl = 1; ttl <= 30; ttl++) {
		bzero(&lastrecv, sizeof(struct in_addr));
		printf("%2d", ttl);
                if (setsockopt(sendfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int)) < 0)
                        err_sys("setsockopt IP_TTL error");
		for (i = 0; i < probe; i++, Dport++) {
                        if (gettimeofday(&sendtime, (struct timezone *)NULL) < 0)
                                err_sys("gettimeofday sendtime error");
                        to.sin_port = htons(Dport);
			if (sendto(sendfd, buf, sizeof(buf), 0, (struct sockaddr *)&to, sizeof(to)) != sizeof(buf))
				err_sys("sendto error");

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
                                if ((ret = icmp_udp_check((u_char *)recvbuf, n)) == 1) {
                                        if (memcmp(&recv.sin_addr, &lastrecv, sizeof(struct in_addr))) {
                                                printf(" %s", inet_ntoa(recv.sin_addr));
                                                memmove(&lastrecv, &recv.sin_addr, sizeof(struct in_addr));
                                        }
                                        printf(" (%.3f ms)", deltaT(&sendtime));
                                        break;
                                } else if (ret == 2) {
                                        printf(" %s (%.3f ms)\n", inet_ntoa(recv.sin_addr), deltaT(&sendtime));
                                        goto end;
                                } else if (ret == 3) {
                                        printf(" %s (%.3f ms) !S ", inet_ntoa(recv.sin_addr), deltaT(&sendtime));
                                        icmp_srr_print((u_char *)recvbuf);
                                        printf("\n");
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

