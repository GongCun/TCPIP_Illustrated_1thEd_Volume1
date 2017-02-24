#include "rdt.h"

int rdt_listen(struct in_addr src, int scid)
{
        int n, fd, listenfd, connfd;
        char buf[PATH_MAX];
        struct sockaddr_un un;
        struct in_addr dst;
        struct conn_info conn_info;
        struct conn_ret conn_ret;
        int dcid = -1;

        bzero(&dst, sizeof(dst));
        conn_info.pid = getpid();
        conn_info.cact = PASSIVE;
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

        sprintf(buf, "%s.%ld", RDT_UX_SOCK, getpid());
        if ((listenfd = ux_listen(buf)) < 0)
                err_sys("ux_listen() error");
        if ((connfd = ux_accept(listenfd)) < 0)
                err_sys("ux_accept() error");
        n = sizeof(struct conn_ret);
        if (read(connfd, &conn_ret, n) != n)
                err_sys("read() error");
        fprintf(stderr, "ret = %d, err = %d\n", conn_ret.ret, conn_ret.err);
        return(connfd);
}
