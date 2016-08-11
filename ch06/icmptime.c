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
        struct timeval tvorig, tvrecv;
        uint32_t tsorig, tsrecv, tsxmit;
        float rtt;
	int sockfd, n;
	struct sockaddr_in to, from;
	const struct icmp *icmp;
	u_char *ptr;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_sys("socket error");
	setuid(getuid());	/* don't need special permissions any more */

	size = 60 * 1024;	/* OK if setsockopt fails */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	bzero(&to, sizeof(struct sockaddr_in));
	to.sin_family = AF_INET;
	if (inet_pton(AF_INET, argv[1], &to.sin_addr) != 1)
		err_sys("inet_pton error");

        if (gettimeofday(&tvorig, (struct timezone *)NULL) < 0)
                err_sys("gettimeofday error");
        tsorig = tvorig.tv_sec % (24*60*60) * 1000 + tvorig.tv_usec / 1000;
        tsrecv = 0;
        tsxmit = 0;

	bzero(buf, sizeof(buf));
	len = sizeof(tsorig) + sizeof(tsrecv) + sizeof(tsxmit);

        icmp_build_time((u_char *)buf, len, 13, 0, id, seq, tsorig, 0, 0); /* ICMP time request */

	len += 8;		/* ICMP header + data */
	if (sendto(sockfd, buf, len, 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in)) != len)
		err_sys("sendto error");

        if (signal(SIGALRM, sig_alrm) == SIG_ERR)
                err_sys("signal error");
	alarm(WAITTIME);
	for (;;) {
		len = sizeof(from);
		n = icmp_recv(sockfd, (u_char *) buf, sizeof(buf), (struct sockaddr *)&from, (socklen_t *) & len, &ptr);
                if (n < 20)	/* not enough data */
			continue;
		icmp = (struct icmp *)ptr;
		if (icmp->icmp_type == 14 && icmp->icmp_id == id) { /* ICMP time reply */
                        if (gettimeofday(&tvrecv, (struct timezone *)NULL) < 0)
                                err_sys("gettimeofday error");
                        rtt = (tvrecv.tv_sec*1000.0 + tvrecv.tv_usec/1000.0) - (tvorig.tv_sec*1000.0 + tvorig.tv_usec/1000.0);
                        if (ntohl(icmp->icmp_otime) != tsorig)
                                printf("orignate timestamp not echoed: sent %ld, received %ld\n",
                                                (long)tsorig, (long)ntohl(icmp->icmp_otime));
                        tsrecv = ntohl(icmp->icmp_rtime);
                        tsxmit = ntohl(icmp->icmp_ttime);
                        printf("orig = %ld, recv = %ld, xmit = %ld, rtt = %.2f ms\ndifference = %ld ms\n",
                                        (long)tsorig, (long)tsrecv, (long)tsxmit, rtt, (long)(tsrecv-tsorig));
			break;
		}
	}
	alarm(0);


	return 0;
}
