#include "mysock.h"

static void sig_alrm(int signo)
{
        return; /* just interrupt the connect() */
}

int cliopen(char *host, char *port)
{
        int i, fd;
        int on;
        struct sockaddr_in cli_addr, serv_addr;
        struct servent *sp;
        struct hostent *hp;
        in_addr_t inaddr;
        char **pptr;
        char *protocol = "tcp";

        bzero(&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;

        if ((i = atoi(port)) == 0) {
                if ((sp = getservbyname(port, protocol)) == NULL)
                        err_quit("getservbyname() error for: %s/%s", port, protocol);
                serv_addr.sin_port = sp->s_port;
        } else
                serv_addr.sin_port = htons(i);

        /*
         * First try to convert the host name as a dotted-decimal number.
         * Only if that fails do we call gethostbyname().
         */

        if ((inaddr = inet_addr(host)) != INADDR_NONE) {
                /* it's dotted-decimal */
                serv_addr.sin_addr.s_addr = inaddr;
        } else {
                if ((hp = gethostbyname(host)) == NULL)
                        err_quit("gethostbyname() error for: %s", host);

                pptr = hp->h_addr_list;
                if (*pptr == NULL)
                        err_quit("can't ip address for: %s", host);
                if (hp->h_addrtype != AF_INET)
                        err_quit("unknown address type");
                memcpy(&serv_addr.sin_addr, *pptr, hp->h_length);
        }

        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                err_sys("socket() error");
        if (reuseaddr) {
                on = 1;
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                        err_sys("setsockopt() of SO_REUSEADDR error");
        }

        if (bindport) {
                bzero(&cli_addr, sizeof(cli_addr));
                cli_addr.sin_family = AF_INET;
                cli_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* wildcard */
                cli_addr.sin_port = htons(bindport);

                if (bind(fd, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0)
                        err_sys("bind() error");
        }


	/* Need to allocate buffers before connect(), since they can affect
 	 * TCP options (window scale, etc.)
	 */
	buffers(fd);
#if 0
        sockopts(fd, 0); /* may also want to set SO_DEBUG */
#endif

        signal_func_t sigfunc;
        if (timeout) {
                if ((sigfunc = xsignal(SIGALRM, sig_alrm)) == SIG_ERR)
                        err_sys("xsignal() error");
                if (alarm(timeout) != 0) /* Return 0 if no alarm is currently set */
                        err_quit("sig_alrm(): alarm was already set");
        }
        if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                if (errno == EINTR)
                        errno = ETIMEDOUT;
                err_sys("connect() error");
        }
        if (timeout) {
                alarm(0);
                if (xsignal(SIGALRM, sigfunc) == SIG_ERR)
                        err_sys("xsignal() error");
        }

        if (verbose) {
                i = sizeof(cli_addr);
                if (getsockname(fd, (struct sockaddr *)&cli_addr, (socklen_t *)&i) < 0)
                        err_sys("getsockname() error");
                fprintf(stderr, "connected on %s.%d ",
                                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                fprintf(stderr, "to %s.%d\n",
                                inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
        }

        sockopts(fd, 1); /* some options get set after connect() */

        return fd;
}
