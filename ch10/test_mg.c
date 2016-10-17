#include "tcpi.h"

int main(int argc, char *argv[])
{
        int sockfd;
        struct ip_mreq mreq;
        struct ifreq ifreq;

        if (argc != 3)
                err_quit("Usage: %s <interface> <multicast addr>", basename(argv[0]));

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
                err_sys("socket error");

        strncpy(ifreq.ifr_name, argv[1], IFNAMSIZ);
        if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
                err_sys("ioctl error");

        if (inet_pton(AF_INET, argv[2], &mreq.imr_multiaddr) != 1)
                err_quit("inet_pton error");
        memcpy(&mreq.imr_interface, &((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr,
                        sizeof(struct in_addr));
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                err_sys("setsockopt error");

        pause();

        return 0;
}
