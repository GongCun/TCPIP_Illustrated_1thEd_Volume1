#include "rdt.h"

char dev[IFNAMSIZ];
int mtu;

int main(int argc, char *argv[])
{
        int fd;
        int scid;
        struct in_addr src;
        char buf[MAXLINE];
        int n;

        if (argc != 3)
                err_quit("usage: %s <IPaddress> <#CID>", basename(argv[0]));
        if (inet_aton(argv[1], &src) != 1) {
                errno = EINVAL;
                err_sys("inet_aton() error");
        }

        scid = atoi(argv[2]);
        fd = rdt_listen(src, scid);
        while ((n = read(fd, buf, MAXLINE)) > 0)
                if (write(1, buf, n) != n)
                        err_sys("write() error");
        pause();

        return(0);
}

