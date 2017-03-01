#include "rdt.h"

static void sig_hand(int signo)
{
        if (signo == SIGINT || signo == SIGHUP || signo == SIGQUIT)
                exit(1);
}


int rdt_listen(struct in_addr src, int scid)
{
        int n, fd;
        pid_t pid;
        struct sockaddr_un un;
        struct conn_info conn_info;

        if (signal(SIGINT, sig_hand) == SIG_ERR ||
            signal(SIGHUP, sig_hand) == SIG_ERR ||
            signal(SIGQUIT, sig_hand) == SIG_ERR)
        {
                err_sys("signal() error");
        }

        pid = getpid();
        bzero(&conn_info, sizeof(conn_info));
        conn_info.cact = PASSIVE;
        conn_info.pid = pid;
        conn_info.src = conn_user.src = src;
        conn_info.scid = conn_user.scid = scid;
        conn_info.dcid = conn_user.dcid = -1;
        conn_user.pfd = make_fifo(pid);

        if (!mtu) {
                if (dev[0] == 0 && !get_dev(src, dev))
                        err_quit("can't get dev name");
                mtu = get_mtu(dev);
        }
        n = min(mtu, 1500); /* not exceed the capture length */
        conn_user.mss = n;
        if ((conn_user.pkt = malloc(n)) == NULL)
                err_sys("malloc() sndbuf error");

        if ((fd = ux_cli(RDT_UX_SOCK, &un)) < 0)
                err_sys("ux_cli() error");
        n = sizeof(struct conn_info);
        if (sendto(fd, &conn_info, n, 0, (struct sockaddr *)&un,
                                sizeof(struct sockaddr_un)) != n)
        {
                err_sys("sendto() error");
        }

        fprintf(stderr, "sendto() %d bytes\n", n);

        /* Get partner info from FIFO */
        get_pkt(conn_user.pfd, &conn_info, conn_user.pkt, conn_user.mss);
        conn_user.dst = conn_info.dst;
        conn_user.dcid = conn_info.dcid;
        conn_user.sfd = make_sock();
        conn_user_debug(&conn_user);
        pkt_debug((struct rdthdr *)conn_user.pkt);

        return(0);
}
