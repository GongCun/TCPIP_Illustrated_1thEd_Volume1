#include "tcpi.h"

int linktype;

static void udpcksum(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char *argv[])
{
        pcap_t *pt;
        struct bpf_program bp;

        if (argc != 3)
                err_quit("Usage: %s <interface> <command>", basename(argv[0]));


        pt = open_pcap(argv[1], 500, argv[2], &linktype, &bp);
        loop_pcap(pt, &bp, udpcksum, 0);

        return 0;
}

static void udpcksum(u_char *user, const struct pcap_pkthdr *header, const u_char *packet)
{
        const struct ip *ip;
        const struct udphdr *udp;
        int size_eth;
        int size_ip;
        int size_udp = sizeof(struct udphdr);
        int udplen;

        if (linktype != 0 && linktype != 1)
                err_quit("Unknown link-layer header type: %d", linktype);

        size_eth = (linktype == 0) ? 4 : 14;

        ip = (struct ip *)(packet + size_eth);
        if ((size_ip = ip->ip_hl * 4) < sizeof(struct ip))
                err_quit("Invalid IP header length: %d bytes\n", size_ip);

        if (ip->ip_p != IPPROTO_UDP)
                err_quit("Only check the UDP packet");

        if ((udplen = header->caplen - size_eth - size_ip) < size_udp)
                err_quit("Invalid UDP header length: %d bytes\n", udplen);

        udp = (struct udphdr *)(packet + size_eth + size_ip);

        /* get daytime */
        char tmbuf[64];
        time_t time = (time_t)header->ts.tv_sec;
        struct tm *tm = localtime(&time);
        strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", tm);
        printf("%s.%06d ", tmbuf, header->ts.tv_usec);
        printf("%s.%d > ", inet_ntoa(ip->ip_src), ntohs(udp->uh_sport));
        printf("%s.%d ", inet_ntoa(ip->ip_dst), ntohs(udp->uh_dport));
        printf("(UDP cksum=%x)\n", ntohs(udp->uh_sum));
}



