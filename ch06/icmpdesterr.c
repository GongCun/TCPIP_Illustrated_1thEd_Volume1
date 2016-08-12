#include "tcpi.h"

#define WAITTIME 5

static void 
sig_alrm(int signo)
{
	printf("timeout\n");
        exit(1);
}

static pid_t make_icmp_recv(int, int);

int 
main(int argc, char *argv[])
{
	if (argc != 3)
		err_quit("Usage: %s <IPaddr> <Port>", basename(argv[0]));

	int udpfd, sport, dport;
	struct sockaddr_in to, from;

        if ((udpfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                err_sys("socket UDP error");


        sport = (getpid() & 0xffff) | 0x8000;
        dport = atoi(argv[2]);

        make_icmp_recv(sport, dport);

        /* Bind source UDP port */
        bzero(&from, sizeof(struct sockaddr_in));
        from.sin_family = AF_INET;
        from.sin_port = htons(sport);
        if (bind(udpfd, (struct sockaddr *)&from, sizeof(from)) < 0)
                err_sys("bind source UDP port error");
        
        /* Send the UDP packet */
	bzero(&to, sizeof(struct sockaddr_in));
	to.sin_family = AF_INET;
	if (inet_pton(AF_INET, argv[1], &to.sin_addr) != 1)
		err_quit("inet_pton error");
        to.sin_port = htons(dport);

        if (sendto(udpfd, "a", 1, 0, (struct sockaddr *)&to, sizeof(to)) != 1)
               err_sys("send UDP packet error"); 

        wait(NULL);

        return 0;
}

static pid_t make_icmp_recv(int sport, int dport)
{
        char buf[MAXLINE];
        pid_t pid;
        int sockfd, n, len, iplen, size;
	u_char *ptr;
        struct sockaddr_in sockaddr;
        struct icmp *icmp;
        struct ip *ip;
        struct udphdr *udp;

        if ((pid = fork()) > 0)
                return pid;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_sys("socket RAW error");
	setuid(getuid());	/* don't need special permissions any more */

	size = 60 * 1024;	/* OK if setsockopt fails */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

        if (signal(SIGALRM, sig_alrm) == SIG_ERR)
                err_sys("signal error");
	alarm(WAITTIME);

        printf("sport = %d, dport = %d\n", sport, dport);
	for (;;) {
		len = sizeof(sockaddr);
		n = icmp_recv(sockfd, (u_char *) buf, sizeof(buf), (struct sockaddr *)&sockaddr, (socklen_t *) & len, &ptr);
                printf("n = %d\n", n);
                if (n < 8)	/* not enough ICMP data */
			continue;
		icmp = (struct icmp *)ptr;
                printf("%d %d\n", icmp->icmp_type, icmp->icmp_code);
		if (icmp->icmp_type == 3 && icmp->icmp_code == 3) { /* ICMP port unreachableness */
                        ip = (struct ip *)(ptr + 8);

                        /* Check if not enough IP or UDP data */
                        if ((iplen = ip->ip_hl * 4) < 20 || ip->ip_p != IPPROTO_UDP)
                                continue;
                        if (n - 8 - iplen < 8)
                                continue;
                        udp = (struct udphdr *)(ptr + 8 + iplen);
                        if (udp->uh_sport == htons(sport) &&
                                        udp->uh_dport == htons(dport))
                        {
                                printf("Caught ICMP Port Unreachableness error: from %d to %d\n",
                                                sport, dport);
                                break;
                        }
		}
	}
	alarm(0);

	return 0;
}
