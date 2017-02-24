#include "rdt.h"

/*
                   -+- Data Transfer Path -+-

   +---------+ pass connection info by     +-------+
   | user    | unix domain datagram socket |parent |
   | process | --------------------------> |process|
   +---+-----+                             +-------+
       ^                                       /\
       |                                SIGIO /  \
       |                                     /    \
       |                                    v      v
       |send/receive pkt by        +--------+      +--------+
       |unix domain stream socket  |sig_io()|      |packet  |
       |                           |        |      |capture |
       |                           +--------+      +--------+
       |                             |    pass pkt/      ^
       |                        fork |    by pipe/       |capture
       |                             v          /        |pkt
       |                           +--------+  /         |
       |                           |child   | <    +--------+
       +-------------------------->|process |----->|external|
                                   +--------+      +--------+
                                            send pkt

*/

char dev[IFNAMSIZ];
int mtu;
int fd;
static void sig_io(int);

int main(int argc, char *argv[])
{
        const int on = 1;

        if (argc != 2)
                err_quit("%s <dev>", basename(argv[0]));
        strcpy(dev, argv[1]);

        fd = ux_serv(RDT_UX_SOCK);
	if (signal(SIGIO, sig_io) == SIG_ERR)
		err_sys("signal() of SIGIO error");
	if (fcntl(fd, F_SETOWN, getpid()) < 0)
		err_sys("fcntl() of F_SETOWN error");
	if (ioctl(fd, FIOASYNC, &on) < 0)
		err_sys("ioctl() of FIOASYNC error");
	if (ioctl(fd, FIONBIO, &on) < 0)
		err_sys("ioctl() of FIONBIO error");

        from_net();

        return(0);
}

static void sig_io(int signo)
{
        int i, n;
        struct conn_info conn_info;
        struct conn_info *ciptr = &conn_info;

	fprintf(stderr, "caught SIGIO\n");

        /* TO be added rexmt mechanism */
        n = sizeof(struct conn_info);
        if (recvfrom(fd, ciptr, n, 0, NULL, 0) != n)
        {
                err_sys("recvmsg() error");
        }

        conn_info_debug(ciptr);

        switch (ciptr->cact) {
                case ACTIVE:
                        i = krdt_connect(ciptr->dst, ciptr->scid, ciptr->dcid);
                        break;
                case PASSIVE:
                        i = krdt_listen(ciptr->src, ciptr->scid);
                        break;
                default:
                        err_quit("unknown connection type: %d", ciptr->cact);
        }

        make_child(i, ciptr->pid);

        return;
}
