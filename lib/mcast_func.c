#include "tcpi.h"

int mcast_join(int sockfd, const char *dev, char *maddr)
{
        struct ip_mreq mreq;
        struct ifreq ifreq;

        strncpy(ifreq.ifr_name, dev, IFNAMSIZ);
        if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
                return -1;
        if (inet_pton(AF_INET, maddr, &mreq.imr_multiaddr) != 1) {
                errno = EINVAL;
                return -1;
        }
        memcpy(&mreq.imr_interface, &((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr,
                        sizeof(struct in_addr));
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                return -1;

        return 0;
}

