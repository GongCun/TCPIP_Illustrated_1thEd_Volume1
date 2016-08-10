#include "tcpi.h"

#define WAITTIME 5

static void 
sig_alrm(int signo)
{
	printf("timeout\n");
        exit(1);
}


int 
main(int argc, char *argv[])
{
	if (argc != 2)
		err_quit("Usage: %s <IPaddr>", basename(argv[0]));

	char buf[MAXLINE];
	int len, size;
	uint16_t id = (uint16_t) (time(0) & 0xffff);
	uint16_t seq = 0;
	uint32_t mask = 0xffffffff;
	int sockfd, n;
	struct sockaddr_in to, from;
	const struct icmp *icmp;
	u_char *ptr, *p;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_sys("socket error");
	setuid(getuid());	/* don't need special permissions any more */

	size = 60 * 1024;	/* OK if setsockopt fails */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	bzero(&to, sizeof(struct sockaddr_in));
	to.sin_family = AF_INET;
	if (inet_pton(AF_INET, argv[1], &to.sin_addr) != 1)
		err_sys("inet_pton error");

	bzero(buf, sizeof(buf));
	len = sizeof(mask);

	icmp_build_mask((u_char *) buf, len, 17, 0, id, seq, mask);	/* Address mask request */

	len += 8;		/* ICMP header + data */
	if (sendto(sockfd, buf, len, 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in)) != len)
		err_sys("sendto error");

        if (signal(SIGALRM, sig_alrm) == SIG_ERR)
                err_sys("signal error");
	alarm(WAITTIME);
	for (;;) {
		len = sizeof(from);
		n = icmp_recv(sockfd, (u_char *) buf, sizeof(buf), (struct sockaddr *)&from, (socklen_t *) & len, &ptr);
                if (n < 12)	/* not enough data */
			continue;
		icmp = (struct icmp *)ptr;
		if (icmp->icmp_type == 18 && icmp->icmp_id == id) {
			p = (u_char *) icmp->icmp_data;
			printf("received mask = %d.%d.%d.%d, "
			       "from %s\n",
			       *p, *(p + 1), *(p + 2), *(p + 3), inet_ntoa(from.sin_addr));
			break;
		}
	}
	alarm(0);


	return 0;
}
