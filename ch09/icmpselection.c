#include "tcpi.h"

int 
main(int argc, char *argv[])
{
	if (argc != 2)
		err_quit("Usage: %s <IPaddr>", basename(argv[0]));

	char buf[MAXLINE]; /* ICMP packet */
	int sockfd, len;
        const int on = 1;
	struct sockaddr_in to;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_sys("socket error");
	setuid(getuid());	/* don't need special permissions any more */

	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
                err_sys("setsockopt error");

	bzero(&to, sizeof(struct sockaddr_in));
	to.sin_family = AF_INET;
	if (inet_pton(AF_INET, argv[1], &to.sin_addr) != 1)
		err_sys("inet_pton error");

        bzero(buf, sizeof(buf));
        icmp_build_selection((u_char *)buf, 10, 0);


        len = 8;
	if (sendto(sockfd, buf, len, 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in)) != len)
		err_sys("sendto error");

        return 0;
}
