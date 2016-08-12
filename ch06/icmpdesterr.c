#include "tcpi.h"

#define WAITTIME 5

static void
sig_alrm(int signo)
{
	printf("timeout\n");
	exit(1);
}

static void make_icmp_recv(int, int);

int
main(int argc, char *argv[])
{
	if (argc != 2)
		err_quit("Usage: %s <IPaddr>", basename(argv[0]));

	int udpfd, sport, dport;
	struct sockaddr_in to, from;
	pid_t pid;

	if ((udpfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_sys("socket UDP error");


	sport = (getpid() & 0xffff) | 0x8000;
	dport = (time(0) & 0xffff) | 0x8000;

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid == 0) {	/* child process */
		if (signal(SIGALRM, sig_alrm) == SIG_ERR)
			err_sys("signal error");
		alarm(WAITTIME);
		make_icmp_recv(sport, dport);
		alarm(0);
		exit(0);
	}
	/*
	 * parent continue...
	 */
        sleep(1); /* parent must wait for child ready */

	/* Bind source UDP port */
	bzero(&from, sizeof(struct sockaddr_in));
	from.sin_family = AF_INET;
	from.sin_addr.s_addr = htonl(0);
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
/* Child process continue, never returns */

static void 
make_icmp_recv(int sport, int dport)
{
	char buf[MAXLINE], Src[32], Dst[32];
	int sockfd, n, len, iplen, size;
	u_char *ptr;
	struct sockaddr_in sockaddr;
	struct icmp *icmp;
	struct ip *ip;
	struct udphdr *udp;


	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_sys("socket RAW error");
	setuid(getuid());	/* don't need special permissions any more */

	size = 60 * 1024;	/* OK if setsockopt fails */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	for (;;) {
		len = sizeof(sockaddr);
		n = icmp_recv(sockfd, (u_char *) buf, sizeof(buf), (struct sockaddr *)&sockaddr, (socklen_t *) & len, &ptr);
		if (n < 8)	/* not enough ICMP data */
			continue;
		icmp = (struct icmp *)ptr;
		if (icmp->icmp_type == 3 && icmp->icmp_code == 3) {	/* ICMP port
									 * unreachableness */
			ip = (struct ip *)(ptr + 8);

			/* Check if not enough IP or UDP data */
			if ((iplen = ip->ip_hl * 4) < 20 || ip->ip_p != IPPROTO_UDP)
				continue;
			if (n - 8 - iplen < 8)
				continue;
			udp = (struct udphdr *)(ptr + 8 + iplen);
			if (udp->uh_sport == htons(sport) &&
			    udp->uh_dport == htons(dport)) {
                                /* Never use inet_ntoa() */
				printf("Caught ICMP Port Unreachableness error\nFrom %s:%d to %s:%d\n",
				       inet_ntop(AF_INET, &ip->ip_src, Src, sizeof(Src)), sport,
				       inet_ntop(AF_INET, &ip->ip_dst, Dst, sizeof(Dst)), dport);

				break;
			}
		}
	}
	return;
}
