#include "rdt.h"

char dev[IFNAMSIZ];
int mtu;

int main(int argc, char *argv[])
{
        struct in_addr dst;
        char buf[MAXLINE];
        int n;

        if (argc != 3)
                err_quit("usage: %s <IPaddress> <#CID>", basename(argv[0]));
        if (inet_aton(argv[1], &dst) != 1) {
                errno = EINVAL;
                err_sys("inet_aton() error");
        }

        rdt_connect(dst, 1, atoi(argv[2]));
        pause();

        return(0);
}
