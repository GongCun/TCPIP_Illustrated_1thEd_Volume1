#include "mysock.h"

static void sig_catch(int signo)
{
	exit(0); /* exit handler will reset tty state */
}

void loop(FILE *fp, int sockfd)
{
	fd_set rset, xset;
	int stdineof;
	int n;
	int maxfd;
	int fd;
        int justoob = 0;
        struct timeval tv, *ptv;

        /* For UDP verbose output */
        int flags;
        socklen_t socklen;
	struct sockaddr_in cliaddr;
        struct in_addr dstaddr;

	stdineof = 0;
	FD_ZERO(&rset);
	FD_ZERO(&xset);

	if (cbreak && isatty(STDIN_FILENO)) {
		if (tty_cbreak(STDIN_FILENO) < 0)
			err_sys("tty_cbreak() error");
		if (atexit(tty_atexit) != 0)
			err_sys("tty_atexit() error");
		if (signal(SIGINT, sig_catch) == SIG_ERR)
			err_sys("signal of SIGINT error");
		if (signal(SIGQUIT, sig_catch) == SIG_ERR)
			err_sys("signal of SIGQUIT error");
		if (signal(SIGTERM, sig_catch) == SIG_ERR)
			err_sys("signal of SIGTERM error");
	}

	for ( ; ; ) {
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
                if (!justoob)
                        FD_SET(sockfd, &xset);
		maxfd = max(fileno(fp), sockfd);
                
                if (timeout && server) {
                        tv.tv_sec = timeout;
                        tv.tv_usec = 0;
                        ptv = &tv;
                } else
                        ptv = (struct timeval *)NULL;
		if ((n = select(maxfd+1, &rset, NULL, &xset, ptv)) < 0)
			err_sys("select() error");
                if (n == 0) /* timed out for server */
                        err_quit("process %d exited: recvmsg timed out", (int)getpid());

		if (FD_ISSET(fileno(fp), &rset)) { /* data to read on fp */
			if ((n = read(fileno(fp), rbuf, readlen)) < 0)
				err_sys("read() error from fp");
			else if (n == 0) {
				if (udp)
					break;
				stdineof = 1;
				if (shutdown(sockfd, 1) < 0) /* default half close */
					err_sys("shutdown() error");
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			if (client) {
				if (multicast) {
					if (sendto(sockfd, rbuf, n, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
						err_sys("sendto() error");

				} else {
					if (writen(sockfd, rbuf, n) != n)
						err_sys("writen() error");
				}
			}
		}

                if (FD_ISSET(sockfd, &xset)) {
                        if ((n = recv(sockfd, rbuf, readlen-1, MSG_OOB)) < 0)
                                err_sys("recv() error");
                        rbuf[n] = 0;
                        printf("read %d OOB byte: %s\n", n, rbuf);
                        justoob = 1;
                        FD_CLR(sockfd, &xset);
                }

		if (FD_ISSET(sockfd, &rset)) {	/* data to read from socket */
			if (udp && server) {
                                socklen = sizeof(struct sockaddr_in);
                                if ((n = recvdst(sockfd, rbuf, readlen, &flags, (struct sockaddr *)&cliaddr,
                                                                &socklen, &dstaddr)) < 0)
                                {
                                        err_sys("recvfrom() error");
                                }
				if (verbose) {
					fprintf(stderr, "from %s.%d", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
#if defined(HAVE_MSGHDR_MSG_CONTROL) 
#if defined(IP_RECVDSTADDR) || defined(IP_PKTINFO)
					if (recvdstaddr) {
						fprintf(stderr, " to %s", inet_ntoa(dstaddr));
					}
#endif /* HAVE_MSGHDR_MSG_CONTROL */
#endif /* IP_RECVDSTADDR */
					putc('\n', stderr);

				} /* end of verbose */
                                if (echo) {
                                        if (sendto(sockfd, rbuf, n, 0, (struct sockaddr *)&cliaddr, socklen) < 0)
                                                err_sys("sendto() error");
                                } else {
                                        if (writen(fileno(stdout), rbuf, n) != n)
                                                err_sys("writen() error");
                                }
			} else {
				if ((n = read(sockfd, rbuf, readlen)) < 0)
					err_sys("read() sockfd error");
				else if (n == 0) {
					if (stdineof == 1 || server)
						break;	/* normal termination */
					else
						err_quit("process %d: connection closed by peer", (int)getpid());
				}
				if (client)
					fd = fileno(stdout);
				else
					fd = echo ? sockfd : fileno(stdout);
				if (writen(fd, rbuf, n) != n)
					err_sys("writen() error");
				justoob = 0;
			}
		}
	}

	if (close(sockfd) < 0)
		err_sys("close() error");
        fprintf(stderr, "process %d: connection closed normally\n", (int)getpid());

	return;
}
