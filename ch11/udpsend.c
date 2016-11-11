#include "tcpi.h"

int
main(int argc, char **argv)
{
	int sockfd;
	int n = 1;
        size_t len = sizeof(n);
	struct sockaddr_in servaddr;
	char buf[65536];

	if (argc != 3)
		err_quit("Usage: %s <IPaddress> <Port>", basename(argv[0]));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1) {
		errno = EINVAL;
		err_sys("inet_pton error");
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err_sys("socket error");
        for (n += 128; n <= 65535; n += 128) {
                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &n, len) < 0) {
                        if (errno == ENOBUFS)
                                break;
                        err_sys("setsockopt SO_SNDBUF error");
                }
        }
        printf("SNDBUF = %d\n", n - 128);
	while ((n = read(0, buf, sizeof(buf))) != 0) {
		if (n == -1) {
			if (errno == EINTR)
				continue;
			else
				err_sys("read error");
		}
		if (sendto(sockfd, buf, n, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) != n)
			err_sys("write error");
	}

	exit(0);
}
