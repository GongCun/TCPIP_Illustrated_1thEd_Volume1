#include "traceroute.h"

char *device;
int Sport;
int Dport = 32768 + 666;
int probe = 3;
sigjmp_buf jumpbuf;
struct sockaddr_in from;
struct sockaddr_in to;

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
                        !memcmp(&originip->ip_dst, &to.sin_addr, sizeof(struct in_addr)) &&
                        udp->uh_sport == htons(Sport) &&
                        udp->uh_dport == htons(Dport))
                /* Don't check the source IP address, because it doesn't bind an address */
        {
                if (icmp->icmp_type == 11 &&
                                icmp->icmp_code == 0) {
                        return 1;
                } else if (icmp->icmp_type == 3 &&
                                icmp->icmp_code == 3) {
                        return 2;
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

	if (argc != 2) {
		err_quit("Usage: %s <IPaddr>", basename(argv[0]));
	}

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
