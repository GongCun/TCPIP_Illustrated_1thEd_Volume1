#include "tcpi.h"

void
udp_write(int fd, char *buf, int userlen, struct in_addr src, struct in_addr dst, u_short sport, u_short dport, struct sockaddr *to, socklen_t tolen)
{
        struct ip *ip;
        struct udphdr *udp;
        int total;

        ip = (struct ip *)buf;
        bzero(ip, sizeof(ip));
        ip->ip_v = 4;
        ip->ip_hl = sizeof(struct ip) >> 2;
        ip->ip_tos = 0;
        total = userlen + sizeof(struct ip) + sizeof(struct udphdr);
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
        ip->ip_ttl = (u_char)64;
        ip->ip_p = IPPROTO_UDP;
        ip->ip_src.s_addr = src.s_addr;
        ip->ip_dst.s_addr = dst.s_addr;
        if (do_checksum((u_char *)buf, IPPROTO_IP , ip->ip_hl*4) < 0)
                err_quit("IP do_checksum error");

        udp = (struct udphdr *)(buf + ip->ip_hl * 4);
        bzero(udp, sizeof(struct udphdr));
        udp->uh_sport = sport;
        udp->uh_dport = dport;
        udp->uh_ulen = htons((uint16_t)(sizeof(struct udphdr) + userlen));
        if (do_checksum((u_char *)buf, IPPROTO_UDP, sizeof(struct udphdr) + userlen) < 0)
                err_quit("UDP do_checksum error");

        if (sendto(fd, buf, total, 0, to, tolen) != total)
                err_sys("send UDP error");

        return;

}

