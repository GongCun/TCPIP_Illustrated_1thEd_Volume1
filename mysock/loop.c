#include "mysock.h"

void loop(FILE *fp, int sockfd)
{
	fd_set rset;
	int stdineof;
	int n;
	int maxfd;
	int fd;

	stdineof = 0;
	FD_ZERO(&rset);

	for ( ; ; ) {
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfd = max(fileno(fp), sockfd);
		if (select(maxfd+1, &rset, NULL, NULL, NULL) < 0)
			err_sys("select() error");

		if (FD_ISSET(fileno(fp), &rset)) { /* data to read on fp */
			if ((n = read(fileno(fp), rbuf, readlen)) < 0)
				err_sys("read() error from fp");
			else if (n == 0) {
				stdineof = 1;
				if (shutdown(sockfd, 1) < 0) /* default half close */
					err_sys("shutdown() error");
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			if (writen(sockfd, rbuf, n) != n)
				err_sys("writen() error");
		}

		if (FD_ISSET(sockfd, &rset)) { /* data to read from socket */
			if ((n = read(sockfd, rbuf, readlen)) < 0) 
				err_sys("read() sockfd error");
			else if (n == 0) {
			       if (stdineof == 1 || server)
				       break; /* normal termination */
			       else
				       err_quit("connection closed by peer abnormally");
			}
			if (client)
				fd = fileno(stdout);
			else
				fd = echo ? sockfd : fileno(stdout);
			if (writen(fd, rbuf, n) != n)
				err_sys("writen() error");
		}
	}

	if (close(sockfd) < 0)
		err_sys("close() error");
        fprintf(stderr, "connection closed by peer normally\n");

	return;
}
