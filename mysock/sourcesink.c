#include "mysock.h"
#include <ctype.h>

static void pattern(char *ptr, int len);
static void sig_urg(int signo);
static int fd = -1;
static void 
distime(void)
{
	char tmbuf[64], buf[64];
	struct tm *tm;
	struct timeval tv;

	if (gettimeofday(&tv, NULL) < 0)
		err_sys("gettimeofday() error");
	tm = localtime(&tv.tv_sec);
	if (strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", tm) == 0)
		err_quit("strftime() error");
	snprintf(buf, sizeof(buf), "%s.%06d", tmbuf, (int) tv.tv_usec);
	fprintf(stderr, "%s", buf);

	return;
}

void sink(int sockfd)
{
        int i, n;
        char oob;

        if (client) {
                pattern(wbuf, writelen); /* fill send buffer with a pattern */

		if (timestamp) {
			distime();
			fprintf(stderr, "\tstarting\n");
		}

                if (pauseinit)
                        sleep(pauseinit);

                for (i = 1; i <= nbuf; i++) {
			if (timestamp) {
				distime();
				fprintf(stderr, "\t");
			}
                        if (urgwrite == i) {
                                oob = (char)urgwrite;

                                if ((n = send(sockfd, &oob, 1, MSG_OOB)) != 1)
                                        err_sys("send() MSG_OOB error");
                                if (verbose)
                                        fprintf(stderr, "wrote %d byte of urgent data\n", n);
                        }
                        if ((n = writen(sockfd, wbuf, writelen)) != writelen)
                                err_sys("writen() error");
                        if (verbose)
                                fprintf(stderr, "wrote %d bytes\n", n);
                        if (pauserw)
                                sleep(pauserw);
                }
        } else {
                /*
                 * Server process
                 */
		if (timestamp) {
			distime();
			fprintf(stderr, " \tstarting\n");
		}

                if (pauseinit)
                        sleep(pauseinit);

                fd = sockfd;
                if (xsignal(SIGURG, sig_urg) == SIG_ERR)
                        err_sys("signal() of SIGURG error");
                if (fcntl(sockfd, F_SETOWN, getpid()) < 0)
                        err_sys("fcntl() of F_SETOWN error");


                for ( ; ; ) {
			if (timestamp) {
				distime();
				fprintf(stderr, "\t");
			}
                        if ((n = read(sockfd, rbuf, readlen)) < 0)
                                err_sys("read() error");
                        else if (n == 0) {
                                fprintf(stderr, "connection closed by peer\n");
                                break;
                        }

                        if (verbose)
                                fprintf(stderr, "received %d bytes\n", n);

                        if (pauserw)
                                sleep(pauserw);
                }
        }

        if (pauseclose) {
                if (verbose)
                        fprintf(stderr, "pausing before close\n");
                sleep(pauseclose);
        }

        if (close(sockfd) < 0)
                err_sys("close() error");
}

static void pattern(char *ptr, int len)
{
        char c;
        c = 0;
        while (len-- > 0) {
                while (isprint(c & 0x7f) == 0) /* skip over nonprinting characters */
                        c++;
                *ptr++ = (c++ & 0x7f);
        }
}

static void sig_urg(int signo)
{
        int n;
        char c;

        printf("SIGURG received\n");
        if ((n = recv(fd, &c, 1, MSG_OOB)) != 1)
                err_sys("recv() error");
        printf("read OOB byte: 0x%02x\n", c);
}
