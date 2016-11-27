#include "tcpi.h"


int main(int argc, char *argv[])
{
        int sockfd;
        const int on = 0;

        if (argc != 3)
                err_quit("mjoin <Dev> <ClassD_IP>");

        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                err_sys("socket error");

        if (mcast_join(sockfd, argv[1], argv[2]) < 0)
                err_sys("mcast_join error");

        if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &on, sizeof(on)) < 0)
                err_sys("setsockopt IP_MULTICAST_LOOP error");

        pause();
        exit(0);
}
