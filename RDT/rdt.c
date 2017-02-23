#include "rdt.h"

char dev[IFNAMSIZ];
int mtu;
int fd;
static void sig_io(int);

int main(int argc, char *argv[])
{
        int i;

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
        struct msghdr msg;
        struct iovec iov[1];
        struct conn_info *ciptr = &conn_info;

	fprintf(stderr, "caught SIGIO\n");

        bzero(&msg, sizeof(struct msghdr));
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        iov[0].iov_base = ciptr;
        iov[0].iob_len = sizeof(struct conn_info);

        /* TO be added rexmt mechanism */
        if ((n = recvmsg(fd, &msg, 0)) < sizeof(struct conn_info))
        {
                err_sys("recvmsg() error");
        }


        i = krdt_connect(ciptr->dst, ciptr->scid, ciptr->dcid);
        make_child(i);

        return;
}
