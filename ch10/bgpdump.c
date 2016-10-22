#include "tcpi.h"
#include "drp.h"

#define CMD "tcp and dst port 179 and tcp[13:1] & 8 != 0" /* BGP port 179 */

int linktype;

static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char *argv[])
{
        pcap_t *pt;
        struct bpf_program bp;
        int to;
        int listenfd;
        struct sockaddr_in servaddr;

        if (argc != 4)
                err_quit("Usage: %s <interface> <seconds> <#packets>", basename(argv[0]));

        if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                err_sys("socket error");
        bzero(&servaddr, sizeof(struct sockaddr_in));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(179);

        if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                err_sys("bind error");
        if (listen(listenfd, 1024) < 0)
                err_sys("listen error");

        to = atoi(argv[2]);
        to = (to < 0) ? -1 : to*1000;
        pt = open_pcap(argv[1], to, CMD, &linktype, &bp);

        loop_pcap(pt, &bp, callback, atoi(argv[3]));

        return 0;
}


static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet)
{
        const struct ip *ip;
        const struct tcphdr *tcp;
        const struct bgphdr *bgp;
        const struct bgpopenhdr *bgpopen;
        int i;
        int size_eth, size_ip;
        int size_tcp;
        int size_bgp = sizeof(struct bgphdr);
        uint8_t optlen, *ptr, paratype, paralen;

        if (linktype != 0 && linktype != 1)
                err_quit("Unknown link-layer header type: %d", linktype);
        size_eth = (linktype == 0) ? 4 : 14;

        ip = (struct ip *)(packet + size_eth);
        if ((size_ip = ip->ip_hl * 4) < 20)
                err_quit("Invalid IP header length: %d bytes\n", size_ip);

        if (header->caplen - size_eth - size_ip < sizeof(struct tcphdr))
                err_quit("Invalid TCP header length: %d bytes\n", header->caplen - size_eth - size_ip);

        tcp = (struct tcphdr *)(packet + size_eth + size_ip);
        assert(tcp->th_dport == htons(179));
        size_tcp = tcp->th_off * 4;

        bgp = (struct bgphdr *)(packet + size_eth + size_ip + size_tcp);

        printf("Capture BGP packet from %s to ", inet_ntoa(ip->ip_src));
        printf("%s\n", inet_ntoa(ip->ip_dst));
        printf("BGP Marker: ");
        for (i = 0; i < sizeof(bgp->bgp_marker); i++)
                printf("%x%s", bgp->bgp_marker[i], (i%2) == 1 ? " " : "");
        printf("\nBGP packet length = %d, type = %d\n", ntohs(bgp->bgp_len), bgp->bgp_type);

        if (bgp->bgp_type != 1)
                return;

        bgpopen = (struct bgpopenhdr *)(packet + size_eth + size_ip + size_tcp + size_bgp);
        printf("Verion = %d, AS id = %d\n", bgpopen->bgp_ver, ntohs(bgpopen->bgp_asid));
        printf("Hold timer = %d sec\n", ntohs(bgpopen->bgp_holdtime));
        printf("BGP id = %s\n", inet_ntoa(*((struct in_addr *)&bgpopen->bgp_bgpid)));

        optlen = bgpopen->bgp_optlen;
        printf("Option Paratemters length = %d\n", optlen);
        ptr = (uint8_t *)(packet + size_eth + size_ip + size_tcp + size_bgp + sizeof(struct bgpopenhdr));
        while (optlen > 0) {
                paratype = *ptr;
                paralen = *(ptr + 1);
                printf("Parameter Type = %zd, Length = %zd ", paratype, paralen);
                if (paratype == 1)
                        printf("(Authentication Optional Parameter)\n");
                else if (paratype == 2)
                        printf("(Capabilities Optional Parameter)\n");
                else
                        printf("(Unknown Optional Parameter)\n");
                ptr += paralen + 2;
                optlen -= paralen + 2;
        }

        printf("\n");

        return;
}

