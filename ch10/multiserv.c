#include "tcpi.h"

int main(int argc, char *argv[])
{
        int sockfd;
        int n;
        const int on = 1;
        struct sockaddr_in sarecv;
        struct sockaddr_in safrom;
        socklen_t salen;
        char line[MAXLINE];
        char buf[INET_ADDRSTRLEN];

        if (argc != 4)
                err_quit("Usage: %s <interface> <multicast addr> <#port>", basename(argv[0]));

        if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                err_sys("socket error");

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                err_sys("setsockopt error");

        bzero(&sarecv, sizeof(struct sockaddr_in));
        sarecv.sin_family = AF_INET;
        sarecv.sin_port = htons(atoi(argv[3]));
        if (inet_pton(AF_INET, argv[2], &sarecv.sin_addr) != 1) {
                errno = EINVAL;
                err_sys("inet_pton error");
        }
        if (bind(sockfd, (struct sockaddr *)&sarecv, sizeof(struct sockaddr_in)) < 0)
                err_sys("bind error");

        if (mcast_join(sockfd, argv[1], argv[2]) < 0)
                err_sys("mcast_join error");

        for (;;) {
                salen = sizeof(struct sockaddr_in);
                n = recvfrom(sockfd, line, MAXLINE-1, 0, (struct sockaddr *)&safrom, &salen);
                if (n <= 0) break;
                line[n] = 0;
                printf("from %s: %s", inet_ntop(AF_INET, &safrom.sin_addr, buf, sizeof(buf)), line);
        }


        return 0;
}
