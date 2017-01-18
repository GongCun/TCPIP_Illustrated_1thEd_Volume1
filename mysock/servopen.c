#include "mysock.h"

int servopen(char *host, char *port)
{
        int newfd, fd, i, on;
        char *protocol;
        struct sockaddr_in cli_addr, serv_addr;
        struct servent *sp;
        in_addr_t inaddr;
	pid_t pid;

        protocol = udp ? "udp" : "tcp";

        bzero(&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;

        if (host == NULL)
                serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* wildcard */
        else {
                if ((inaddr = inet_addr(host)) == INADDR_NONE)
                        err_quit("invalid host name for %s", host);
                serv_addr.sin_addr.s_addr = inaddr;
        }

        if ((i = atoi(port)) == 0) {
                if ((sp = getservbyname(port, protocol)) == NULL)
                        err_quit("getservbyname() error for: %s/%s", port, protocol);
                serv_addr.sin_port = sp->s_port;
        } else
                serv_addr.sin_port = htons(i);

        if ((fd = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0)) < 0)
                err_sys("socket() error");
        if (reuseaddr) {
                on = 1;
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                        err_sys("setsockopt() of SO_REUSEADDR error");
        }

        if (udp && reuseport) {
#ifdef SO_REUSEPORT
                on = 1;
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0)
                        err_sys("setsockopt() of SO_REUSEPORT error");
#endif
        }

        if (bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                err_sys("bind() error");

	/* May set receive buffer size; must do here to get
 	 * correct window advertised on SYN
	 */
	buffers(fd);

        if (udp) {
                if (foreignip[0] != 0) { /* connect to foreignip/port# */
                        bzero((char *)&cli_addr, sizeof(cli_addr));
                        cli_addr.sin_family = AF_INET;
                        cli_addr.sin_addr.s_addr = inet_addr(foreignip);
                        cli_addr.sin_port = htons(foreignport);
                        if (connect(fd, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0)
                                err_sys("connect() error");
                }
                sockopts(fd, 1);
                return(fd); /* nothing else to do */
        }

        sockopts(fd, 0); /* only set some socket for fd */

        if (listen(fd, listenq) < 0)
                err_sys("listen() error");

        if (pauselisten)
                sleep(pauselisten);

	for (;;) {
		i = sizeof(cli_addr);
		if ((newfd = accept(fd, (struct sockaddr *) & cli_addr, (socklen_t *) & i)) < 0)
			err_sys("accept() error");
		if (verbose) {
			i = sizeof(serv_addr);
			if (getsockname(newfd, (struct sockaddr *) & serv_addr, (socklen_t *) & i) < 0)
				err_sys("getsockname() error");
			fprintf(stderr, "connection on %s.%d ",
				inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
			fprintf(stderr, "from %s.%d\n",
				inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
		}
		if (dofork) {
			if ((pid = fork()) < 0)
				err_sys("fork() error");
			else if (pid > 0) {
				if (verbose)
					fprintf(stderr, "new process %d for request\n", (int) pid);
				if (close(newfd) < 0) /* parent closes connected socket */
					err_sys("close() error");
				continue;
			} else {
				if (close(fd) < 0)
					err_sys("close() error"); /* child closes listening socket */
			}
		}

		buffers(newfd); /* setsockopt() again, in case it didn't propagate
				  from listening socket to connected socket */
		sockopts(newfd, 1);
		return newfd;
	}
}

