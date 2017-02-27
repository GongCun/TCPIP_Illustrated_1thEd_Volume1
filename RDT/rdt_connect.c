#include "rdt.h"

static char sockname[PATH_MAX];

static void sockexit(void)
{
        unlink(sockname);
}

static void sig_hand(int signo)
{
        if (signo == SIGINT || signo == SIGHUP || signo == SIGQUIT)
                exit(1);
}

int rdt_connect(struct in_addr dst, int scid, int dcid)
{
        int n, fd, listenfd, connfd;
        struct sockaddr_un un;
        struct in_addr src;
        struct conn_info conn_info;
        struct conn_ret conn_ret;

        if (signal(SIGINT, sig_hand) == SIG_ERR ||
            signal(SIGHUP, sig_hand) == SIG_ERR ||
            signal(SIGQUIT, sig_hand) == SIG_ERR)
        {
                err_sys("signal() error");
        }

        if (atexit(sockexit) != 0)
                err_sys("can't register sockexit()");

        bzero(&src, sizeof(src));
        conn_info.pid = getpid();
        conn_info.cact = ACTIVE;
        conn_info.src = src;
        conn_info.dst = dst;
        conn_info.scid = scid;
        conn_info.dcid = dcid;

        if ((fd = ux_cli(RDT_UX_SOCK, &un)) < 0)
                err_sys("ux_cli() error");
        n = sizeof(struct conn_info);
        if (sendto(fd, &conn_info, n, 0, (struct sockaddr *)&un,
                                sizeof(struct sockaddr_un)) != n)
        {
                err_sys("sendto() error");
        }

        sprintf(sockname, "%s.%ld", RDT_UX_SOCK, (long)getpid());
        if ((listenfd = ux_listen(sockname)) < 0)
                err_sys("ux_listen() error");
        if ((connfd = ux_accept(listenfd)) < 0)
                err_sys("ux_accept() error");
        n = sizeof(struct conn_ret);
        if (read(connfd, &conn_ret, n) != n)
                err_sys("read() error");
        fprintf(stderr, "ret = %d, err = %d\n", conn_ret.ret, conn_ret.err);
        return(connfd);
}
