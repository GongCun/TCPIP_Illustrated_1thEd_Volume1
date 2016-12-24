#include "mysock.h"

void loop(FILE *fp, int sockfd)
{
	fd_set rset;
	int stdineof;
	int n;
	int maxfd;
	int fd;
        struct timeval tv, *ptv;

	stdineof = 0;
	FD_ZERO(&rset);

	for ( ; ; ) {
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfd = max(fileno(fp), sockfd);
                
                if (timeout && server) {
                        tv.tv_sec = timeout;
                        tv.tv_usec = 0;
                        ptv = &tv;
                } else
                        ptv = (struct timeval *)NULL;
		if ((n = select(maxfd+1, &rset, NULL, NULL, ptv)) < 0)
			err_sys("select() error");
                if (n == 0) /* timed out for server */
                        err_quit("recvmsg timed out");

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
				       err_quit("process %d: connection closed by peer abnormally", (int)getpid());
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
        fprintf(stderr, "process %d: connection closed normally\n", (int)getpid());

	return;
}
