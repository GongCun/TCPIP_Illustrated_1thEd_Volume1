#include "tcpi.h"

void
ospf_write(int fd, char *buf, int ospflen, struct in_addr src, struct in_addr dst, struct sockaddr *to, socklen_t tolen)
{
        struct ip *ip;
        int total;

        ip = (struct ip *)buf;
        bzero(ip, sizeof(ip));
        ip->ip_v = 4;
        ip->ip_hl = sizeof(struct ip) >> 2;
        ip->ip_tos = 0;
        total = ospflen + sizeof(struct ip);
#if defined(_LINUX) || defined(_OPENBSD)
        ip->ip_len = htons(total);
#else
        ip->ip_len = total;
#endif

        ip->ip_id = 0;

#ifdef _LINUX
        ip->ip_off = htons((0 << 15) + (0 << 14) + (0 << 13) + 0);
#else
        ip->ip_off = IP_DF;     /* Don't Fragment */
#endif
        ip->ip_ttl = (u_char)1; /* By default,
                                   all OSPF packets are sent with an IP TTL of 1 */
        ip->ip_p = 89; /* OSPF protocol */
        ip->ip_src.s_addr = src.s_addr;
        ip->ip_dst.s_addr = dst.s_addr;
        if (do_checksum((u_char *)buf, IPPROTO_IP , ip->ip_hl*4) < 0)
                err_quit("IP do_checksum error");

        if (sendto(fd, buf, total, 0, to, tolen) != total)
                err_sys("sendto error");

        return;

}

