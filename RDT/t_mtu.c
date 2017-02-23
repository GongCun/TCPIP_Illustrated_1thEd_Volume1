#include "rdt.h"

int main(int argc, char *argv[])
{
        struct in_addr src;
        struct in_addr dst;
        char dev[IFNAMSIZ];

        if (argc != 2)
                err_quit("usage: %s <IPaddress>", basename(argv[0]));
        if (inet_aton(argv[1], &dst) != 1) {
                errno = EINVAL;
                err_sys("inet_aton() error");
        }

        src = get_addr(dst);
        if (!get_dev(src, dev))
                err_quit("can't get dev name");
        printf("Dev = %s\n", dev);
        printf("MTU = %d\n", get_mtu(dev));

        return(0);
}

