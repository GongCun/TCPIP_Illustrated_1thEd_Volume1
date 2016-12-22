#include "mysock.h"

int servopen(char *host, char *port)
{
        int newfd, fd, i, on;
        char *protocol = "tcp";
        struct sockaddr_in cli_addr, serv_addr;
        struct servent *sp;
        in_addr_t inaddr;

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

        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                err_sys("socket() error");
        if (reuseaddr) {
                on = 1;
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                        err_sys("setsockopt() of SO_REUSEADDR error");
        }
        if (bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                err_sys("bind() error");
#if 0
        sockopts(fd, 0); /* only set some socket for fd */
#endif
        if (listen(fd, listenq) < 0)
                err_sys("listen() error");

        i = sizeof(cli_addr);
        if ((newfd = accept(fd, (struct sockaddr *)&cli_addr, (socklen_t *)&i)) < 0)
                err_sys("accept() error");
        if (verbose) {
                i = sizeof(serv_addr);
                if (getsockname(newfd, (struct sockaddr *)&serv_addr, (socklen_t *)&i) < 0)
                        err_sys("getsockname() error");
                fprintf(stderr, "connection on %s.%d ",
                                inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
                fprintf(stderr, "from %s.%d\n",
                                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        }
        return newfd;
}

