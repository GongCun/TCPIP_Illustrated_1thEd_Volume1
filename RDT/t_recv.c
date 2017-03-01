#include "rdt.h"

char dev[IFNAMSIZ];
int mtu;

int main(int argc, char *argv[])
{
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
        rdt_listen(src, scid);
        pause();

        return(0);
}

