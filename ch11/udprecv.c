#include "tcpi.h"

int
main(int argc, char **argv)
{
	int sockfd, fd;
        char buf[MAXLEN+1];
        int n = 1;
        size_t len = sizeof(n);
        socklen_t socklen;
	struct sockaddr_in servaddr, cliaddr;

        if (argc != 2)
                err_quit("Usage: %s <Port>", basename(argv[0]));

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                err_sys("socket error");

        if ((fd = open("/dev/null", O_RDWR)) < 0)
                err_sys("open error");

        for (n += 128; n <= MAXLEN; n += 128) {
                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, len) < 0) {
                        if (errno == ENOBUFS)
                                break;
                        err_sys("setsockopt SO_RCVBUF error");
                }
        }
        printf("RCVBUF = %d\n", n - 128);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
                err_sys("bind error");

        for (;;) {
                socklen = sizeof(struct sockaddr_in);
                if ((n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &socklen)) < 0) 
                        err_sys("recvfrom error");
                printf("Recv from %s:%d %d byte\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), n);
                sleep(1);
                if (n > 0 && write(fd, buf, n) != n)
                        err_sys("write error");
        }

        exit(0);
}
