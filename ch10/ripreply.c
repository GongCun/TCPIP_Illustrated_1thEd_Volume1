#include "tcpi.h"
#include "rip.h"

#define RIPSERV 520

int main(int argc, char *argv[])
{
        char buf[MAXLINE];
        char *ptr;
        struct rip_data *rip;
        int fd;
        int userlen;
        int metric;
        struct in_addr src;
        struct sockaddr_in to;
        const int on = 1;

        if (argc != 5)
                err_quit("Usage: %s <src> <dst> <addr> <metric>", basename(argv[0]));
        metric = atoi(argv[4]);
        if (metric >= 16 || metric <= 0)
                err_quit("RIP metric out of range: %d\n", metric);

        bzero(buf, sizeof(buf));

        ptr = buf + sizeof(struct ip) + sizeof(struct udphdr);

        *((uint8_t *)ptr) = (uint8_t)2; /* reply */
        *((uint8_t *)(ptr + 1)) = (uint8_t)1; /* RIPv1 */
        rip = (struct rip_data *)(ptr + 4);
        rip->rip_af = htons((uint16_t)2); /* address family */
        rip->rip_metric = htonl(metric);

        if (inet_pton(AF_INET, argv[3], &rip->rip_addr) != 1)
                err_sys("inet_pton error");

        userlen = 4 + sizeof(struct rip_data);

        if (inet_pton(AF_INET, argv[1], &src) != 1)
                err_sys("inet_pton error");
        bzero(&to, sizeof(struct sockaddr_in));
        to.sin_family = AF_INET;
        to.sin_port = htons(RIPSERV);
        if (inet_pton(AF_INET, argv[2], &to.sin_addr) != 1)
                err_sys("inet_pton error");

        if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
                err_sys("socket error");
        if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
                err_sys("setsockopt error");

        udp_write(fd, buf, userlen,
                        src, to.sin_addr,
                        htons(RIPSERV), htons(RIPSERV),
                        (struct sockaddr *)&to, sizeof(to));

        return 0;
}
