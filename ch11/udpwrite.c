#include "tcpi.h"

#define MAXSIZE 65535

int main(int argc, char *argv[])
{
        int nbyte, port, sport;
        struct in_addr src;
        struct sockaddr_in to;
        char buf[MAXSIZE];
        int fd;
        const int on = 1;

        if (argc != 5)
                err_quit("Usage: %s <src> <dst> <port> <bytes>", basename(argv[0]));
        if ((nbyte = atoi(argv[4])) >= sizeof(buf) - 28) /* 28 = IP header + UDP header */
                err_quit("Packet too large");
        port = atoi(argv[3]);
        sport = (getpid() & 0xffff) | 0x8000;

        if (inet_pton(AF_INET, argv[1], &src) != 1) {
                errno = EINVAL;
                err_sys("inet_pton error");
        }
        bzero(&to, sizeof(struct sockaddr_in));
        to.sin_family = AF_INET;
        to.sin_port = htons(port);
        if (inet_pton(AF_INET, argv[2], &to.sin_addr) != 1) {
                errno = EINVAL;
                err_sys("inet_pton error");
        }
        
        if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
                err_sys("socket SOCK_RAW error");
        if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
                err_sys("setsockopt IP_HDRINCL error");

        udp_write(fd, buf, nbyte, src, to.sin_addr,
                        htons(sport), htons(port),
                        (struct sockaddr *)&to, sizeof(to));

        return 0;
}
