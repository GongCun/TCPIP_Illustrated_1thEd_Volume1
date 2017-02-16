#include "tcpi.h"
#include "rdt.h"

int main(int argc, char *argv[])
{
        int rawfd, n;
        const int on = 1;
        struct in_addr src;
        struct in_addr dst;
        struct sockaddr_in sockaddr;
        char buf[1024];

        if (argc != 2)
                err_quit("usage: rdt <IPaddress>");
        if (inet_aton(argv[1], &dst) != 1) {
                errno = EINVAL;
                err_sys("inet_aton() error");
        }

        src = getlocaddr(dst);
        printf("local address %s\n", inet_ntoa(src));

        if ((rawfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                err_sys("socket() error");
        if (setsockopt(rawfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
                err_sys("setsockopt() of IP_HDRINCL error");
        sockaddr.sin_family = AF_INET;
        memcpy(&sockaddr.sin_addr, &dst, sizeof(dst));
        n = make_pkt(src, dst, 0, 0, 0, RDT_REQ, NULL, 0, buf);
        if (n != to_net(rawfd, buf, n, (struct sockaddr *)&sockaddr, sizeof(sockaddr)))
                err_sys("to_net() error");

        return(0);
}
