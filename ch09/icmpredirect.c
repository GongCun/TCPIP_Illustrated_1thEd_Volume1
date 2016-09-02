#include "tcpi.h"

static void ip_build_header_udp(u_char *buf, int userlen, struct in_addr src, struct in_addr dst,
                u_short sport, u_short dport)
{
        struct ip *ip;
        struct udphdr *udp;
        bzero(buf, 20+8); /* Hardcode, IP header + UDP header */
        
        ip = (struct ip *)buf;
        ip->ip_hl = 5;
        ip->ip_v = 4;
        ip->ip_tos = 0;

#if defined(_LINUX) || defined(_OPENBSD)
        ip->ip_len = htons(28 + userlen);
#else
        ip->ip_len = 28 + userlen;
#endif

        ip->ip_id = htons((u_short) time(0) & 0xffff);     /* id is 16 bits */

#ifdef _LINUX
        ip->ip_off = htons((0 << 15) + (0 << 14) + (0 << 13) + 0);
#else
        ip->ip_off = IP_DF;    /* Don't Fragment */
#endif

        ip->ip_ttl = 64;
        ip->ip_p = IPPROTO_UDP;
        ip->ip_src.s_addr = src.s_addr;
        ip->ip_dst.s_addr = dst.s_addr;
        if (do_checksum(buf, IPPROTO_IP, 20) < 0) /* only checksum the IP header */
                err_quit("do_checksum IP header error");

        udp = (struct udphdr *)(buf + 20);
        udp->uh_sport = sport;
        udp->uh_dport = dport;
        udp->uh_ulen = 8 + userlen;
        if (do_checksum(buf, IPPROTO_UDP, 8+userlen) < 0)
                err_quit("do_checksum UDP header error");

        return;
}



int 
main(int argc, char *argv[])
{
	if (argc != 4)
		err_quit("Usage: %s <sendto-IP> <redirect-IP> <route-IP>", basename(argv[0]));

	char buf[MAXLINE]; /* ICMP packet */
        char payload[MAXLINE]; /* IP header + UDP header */
	int len, size;
	int sockfd;
        int sport, dport;
	struct sockaddr_in to;
        struct in_addr redirect, gateway;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_sys("socket error");
	setuid(getuid());	/* don't need special permissions any more */

	size = 60 * 1024;	/* OK if setsockopt fails */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	bzero(&to, sizeof(struct sockaddr_in));
	to.sin_family = AF_INET;
	if (inet_pton(AF_INET, argv[1], &to.sin_addr) != 1)
		err_sys("inet_pton error");

        bzero(&redirect, sizeof(struct in_addr));
        if (inet_pton(AF_INET, argv[2], &redirect) != 1)
                err_sys("inet_pton error");

        bzero(&gateway, sizeof(struct in_addr));
        if (inet_pton(AF_INET, argv[3], &gateway) != 1)
                err_sys("inet_pton error");

	bzero(payload, sizeof(payload));
        sport = (getpid() & 0xffff) | 0x8000;
        dport = 32768+666;
        ip_build_header_udp((u_char *)payload, 0, to.sin_addr, redirect, htons(sport), htons(dport));

        icmp_build_redirect((u_char *)buf, 28, 5, 1, gateway, (u_char *)payload);

        len = 8+28;

	if (sendto(sockfd, buf, len, 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in)) != len)
		err_sys("sendto error");


        return 0;
}
