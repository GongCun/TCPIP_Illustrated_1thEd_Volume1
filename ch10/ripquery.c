#include "tcpi.h"
#include "drp.h"

#define RIPSERV 520

int main(int argc, char *argv[])
{
        char buf[MAXLINE];
        char *ptr;
        struct rip_data *rip;
        int fd;
        int userlen;
        int version;
        struct in_addr src;
        struct sockaddr_in to;
        const int on = 1;

        if (argc < 4)
                err_quit("Usage: %s <src> <dst> <version> [mask]", basename(argv[0]));
        version = atoi(argv[3]);
        if (version != 1 && version != 2)
                err_quit("Unknown RIP version: %d\n", version);

        bzero(buf, sizeof(buf));

        ptr = buf + sizeof(struct ip) + sizeof(struct udphdr);

        *((uint8_t *)ptr) = (uint8_t)1; /* request */
        *((uint8_t *)(ptr + 1)) = (uint8_t)version; /* version */
        rip = (struct rip_data *)(ptr + 4);
        rip->rip_metric = htonl((uint32_t)16);

        if (argc == 5)
                if (inet_pton(AF_INET, argv[4], &rip->rip_mask) != 1)
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
