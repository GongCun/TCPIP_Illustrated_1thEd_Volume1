#include "tcpi.h"

int main(void)
{
    struct ifi_info *ifi, *ifihead;
    struct sockaddr *sa;
    u_char *ptr;
    int i;
    char buf[1024];

    if ((ifihead = ifi = get_ifi_info()) == NULL)
        err_quit("get_ifi_info error");

    for (; ifi != NULL; ifi = ifi->ifi_next) {
        printf("%s: ", ifi->ifi_name);
        if(ifi->ifi_index != 0)
            printf("(%d) ", ifi->ifi_index);

        bzero(buf, sizeof(buf));
        strcat(buf, "<");
        if(ifi->ifi_flags & IFF_UP) strcat(buf, "UP ");
        if(ifi->ifi_flags & IFF_BROADCAST) strcat(buf, "BROADCAST ");
        if(ifi->ifi_flags & IFF_MULTICAST) strcat(buf, "MULTICAST ");
        if(ifi->ifi_flags & IFF_LOOPBACK) strcat(buf, "LOOPBACK ");
        if(ifi->ifi_flags & IFF_POINTOPOINT) strcat(buf, "P2P ");
        buf[strlen(buf)-1] = 0;
        strcat(buf, ">");
        printf("%s\n", buf);

        if ((i = ifi->ifi_hlen) > 0) {
            ptr = ifi->ifi_haddr;
            do {
                printf("%s%x", (i == ifi->ifi_hlen) ? "  " : ":", *ptr++);
            } while(--i > 0);
            printf("\n");
        }
        if (ifi->ifi_mtu != 0)
            printf("  MTU: %d\n", ifi->ifi_mtu);
        if ((sa = ifi->ifi_addr) != NULL)
            printf("  IP addr: %s\n", inet_ntoa(((struct sockaddr_in *)sa)->sin_addr)); 
        if ((sa = ifi->ifi_brdaddr) != NULL)
            printf("  Broadcast addr: %s\n", inet_ntoa(((struct sockaddr_in *)sa)->sin_addr)); 
        if ((sa = ifi->ifi_dstaddr) != NULL)
            printf("  Destination addr: %s\n", inet_ntoa(((struct sockaddr_in *)sa)->sin_addr)); 
    }
    exit(0);
}
