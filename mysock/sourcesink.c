#include "mysock.h"
#include <ctype.h>

static void pattern(char *ptr, int len);

void sink(int sockfd)
{
        int i, n;

        if (client) {
                pattern(wbuf, writelen); /* fill send buffer with a pattern */

                if (pauseinit)
                        sleep(pauseinit);

                for (i = 1; i <= nbuf; i++) {
                        if ((n = writen(sockfd, wbuf, writelen)) != writelen)
                                err_sys("writen() error");
                        if (verbose)
                                fprintf(stderr, "wrote %d bytes\n", n);
                        if (pauserw)
                                sleep(pauserw);
                }
        } else {
                if (pauseinit)
                        sleep(pauseinit);

                for ( ; ; ) {
                        if ((n = read(sockfd, rbuf, readlen)) < 0)
                                err_sys("read() error");
                        else if (n == 0) {
                                fprintf(stderr, "connection closed by peer\n");
                                break;
                        } else if (n != readlen)
                                err_quit("read returned %d, expected %d", n, readlen);

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
