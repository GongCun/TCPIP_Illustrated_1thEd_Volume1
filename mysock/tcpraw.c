#include "mysock.h"

#define IP_TCP_LEN 40
#define IP_LEN 20
#define TCP_LEN 20

void tcpraw(unsigned char event, int id, unsigned int seq, unsigned int ack, char *addr, char *host, int localport, int foreignport)
{
        char buf[IP_TCP_LEN];
        struct ip *ip;
        in_addr_t inaddr;
        char **pptr;
        struct hostent *hp;
        int rawfd;
        const int on = 1;
        struct sockaddr_in to;
        
        if (verbose)
                fprintf(stderr, "Raw sock from %s.%d to %s.%d\n", addr, localport, host, foreignport);

        if ((rawfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                err_sys("socket() error");
        if (setsockopt(rawfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
                err_sys("setsockopt() of IP_HDRINCL error");

        bzero(buf, sizeof(buf));

        /*
         * IP header
         */
        ip = (struct ip *)buf;
        ip->ip_hl = 5;
        ip->ip_v = 4;
        ip->ip_tos = 0;
#if defined(_LINUX) || defined(_OPENBSD)
        ip->ip_len = htons(IP_TCP_LEN);
#else
        ip->ip_len = IP_TCP_LEN;
#endif
        ip->ip_id = id ? htons(id) : htons((u_short)(time(0) & 0xffff));
#ifdef _LINUX
        ip->ip_off = htons(IP_DF);
#else
        ip->ip_off = IP_DF;
#endif
        ip->ip_ttl = 64;
        ip->ip_p = IPPROTO_TCP;

        /* fill addr to ip_src */
        if ((inaddr = inet_addr(addr)) != INADDR_NONE) {
                /* it's dotted-decimal */
                memcpy(&ip->ip_src, &inaddr, sizeof(inaddr));
        } else {
                if ((hp = gethostbyname(addr)) == NULL)
                        err_quit("gethostbyname() error for: %s", addr);

                pptr = hp->h_addr_list;
                if (*pptr == NULL)
                        err_quit("can't ip address for: %s", addr);
                if (hp->h_addrtype != AF_INET)
                        err_quit("unknown address type");
                memcpy(&ip->ip_src, *pptr, hp->h_length);
        }
        
        /* fill host to ip_dst and struct sockaddr_in */
        bzero(&to, sizeof(to));
        to.sin_family = AF_INET;
        to.sin_port = htons(foreignport);
        if ((inaddr = inet_addr(host)) != INADDR_NONE) {
                /* it's dotted-decimal */
                memcpy(&ip->ip_dst, &inaddr, sizeof(inaddr));
                memcpy(&to.sin_addr, &inaddr, sizeof(inaddr));
        } else {
                if ((hp = gethostbyname(host)) == NULL)
                        err_quit("gethostbyname() error for: %s", host);

                pptr = hp->h_addr_list;
                if (*pptr == NULL)
                        err_quit("can't ip address for: %s", host);
                if (hp->h_addrtype != AF_INET)
                        err_quit("unknown address type");
                memcpy(&ip->ip_dst, *pptr, hp->h_length);
                memcpy(&to.sin_addr, *pptr, hp->h_length);
        }

        if (do_checksum((u_char *)buf, IPPROTO_IP, IP_LEN) < 0)
                err_quit("do_checksum() error");
        
        /*
         * TCP header
         */
        struct tcphdr *tcp;
        tcp = (struct tcphdr *)(buf + IP_LEN);
        tcp->th_sport = htons(localport);
        tcp->th_dport = htons(foreignport);
        tcp->th_seq = htonl(seq);
        tcp->th_ack = htonl(ack);
        tcp->th_x2 = 0;
        tcp->th_off = 5;
        tcp->th_flags = event;
        tcp->th_urp = 0; /* urgent pointer, fix me */
        tcp->th_win = htons(65535); /* default */
        tcp->th_sum = 0; /* calculate later */

        /* Now TCP checksum */
        if (do_checksum((u_char *)buf, IPPROTO_TCP, TCP_LEN) < 0)
                err_quit("do_checksum() error");

        if (sendto(rawfd, buf, IP_TCP_LEN, 0, (struct sockaddr *)&to, sizeof(to)) < 0)
                err_sys("sendto() error");

        return;

}

