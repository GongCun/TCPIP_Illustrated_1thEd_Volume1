#include "rdt.h"

char dev[IFNAMSIZ];
int mtu;

int main(int argc, char *argv[])
{
        int i;
        struct in_addr dst;

        if (argc != 2)
                err_quit("usage: %s <IPaddress>", basename(argv[0]));
        if (inet_aton(argv[1], &dst) != 1) {
                errno = EINVAL;
                err_sys("inet_aton() error");
        }

        i = krdt_connect(dst, 1, 0);
        make_child(i);
        from_net();

        return(0);
}
