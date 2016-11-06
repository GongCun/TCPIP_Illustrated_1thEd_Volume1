#include "tcpi.h"
#include <time.h>

int linktype;

static void udpfrag(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char *argv[])
{
        pcap_t *pt;
        struct bpf_program bp;

        if (argc != 3)
                err_quit("Usage: %s <interface> <command>", basename(argv[0]));


        pt = open_pcap(argv[1], 500, argv[2], &linktype, &bp);
        loop_pcap(pt, &bp, udpfrag, 0);

        return 0;
}

static void udpfrag(u_char *user, const struct pcap_pkthdr *header, const u_char *packet)
{
        const struct ip *ip, *orgip;
        const struct udphdr *udp, *orgudp;
        const struct icmp *icmp;
        int size_eth;
        int size_ip, size_orgip;
        int size_udp = sizeof(struct udphdr);
        int size_icmp;
        int udplen;
        char tmbuf[64];
        struct tm *tm;
        int offset = 0;

        if (linktype != 0 && linktype != 1)
                err_quit("Unknown link-layer header type: %d", linktype);

        size_eth = (linktype == 0) ? 4 : 14;

        ip = (struct ip *)(packet + size_eth);
        if ((size_ip = ip->ip_hl * 4) < sizeof(struct ip)) {
                err_msg("Invalid IP header length: %d bytes\n", size_ip);
                return;
        }

        if (ip->ip_p != IPPROTO_UDP && ip->ip_p != IPPROTO_ICMP) {
                err_msg("Only check the UDP or ICMP packet");
                return;
        }

        /* get daytime */
        tm = localtime(&(header->ts.tv_sec));
        strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", tm);
        printf("%s.%06d ", tmbuf, header->ts.tv_usec);


        /* Judge if fragmentation */
        offset = (ntohs(ip->ip_off) & IP_OFFMASK) * 8;

        if (ip->ip_p == IPPROTO_ICMP)
                goto ICMP;

        if ((udplen = header->caplen - size_eth - size_ip) < size_udp || offset) {
                udp = (struct udphdr *)NULL;
        } else
                udp = (struct udphdr *)(packet + size_eth + size_ip);

        if (udp) {
                printf("%s.%d > ", inet_ntoa(ip->ip_src), ntohs(udp->uh_sport));
                printf("%s.%d: ", inet_ntoa(ip->ip_dst), ntohs(udp->uh_dport));
                printf("udp %d ", htons(udp->uh_ulen));
        } else {
                printf("%s > ", inet_ntoa(ip->ip_src));
                printf("%s: ", inet_ntoa(ip->ip_dst));
        }
        if ((ntohs(ip->ip_off) & IP_MF) || offset)
                printf("(frag %d:%d@%d%s)", ntohs(ip->ip_id),
                                ntohs(ip->ip_len) - size_ip,
                                offset, (ntohs(ip->ip_off) & IP_MF) ? "+" : "");
        printf("\n");
        return;

ICMP:
        printf("ICMP ");
        if (!offset)
                icmp = (struct icmp *)(packet + size_eth + size_ip);
        else
                icmp = (struct icmp *)NULL;

        if (icmp == (struct icmp *)NULL) goto END;

        size_icmp = header->caplen - size_eth - size_ip;

        if (!(icmp->icmp_type == ICMP_UNREACH && icmp->icmp_code == ICMP_UNREACH_NEEDFRAG)) {
                printf("type %d code %d ", icmp->icmp_type, icmp->icmp_code);
                goto END;
        }

        if (size_icmp < 8 + sizeof(struct ip) + 8) {
                err_msg("Invalid ICMP packet length: %d bytes\n", size_icmp);
                return;
        }

        orgip = (struct ip *)(packet + size_eth + size_ip + 8);
        size_orgip = orgip->ip_hl * 4;
        printf("%s ", inet_ntoa(orgip->ip_src));

        if (orgip->ip_p == IPPROTO_UDP) {
                if (size_icmp - 8 - size_orgip < 8) {
                        err_msg("Invalid ICMP packet length: %d bytes\n", size_icmp);
                        return;
                }
                orgudp = (struct udphdr *)(packet + size_eth + size_ip + 8 + size_orgip);
                printf("udp port %d unreachable ", ntohs(orgudp->uh_sport));
        }

        printf("next MTU %d ", ntohs(icmp->icmp_nextmtu));
END:
        if ((ntohs(ip->ip_off) & IP_MF) || offset) {
                printf("(frag %d:%d@%d%s)", ntohs(ip->ip_id), ntohs(ip->ip_len) - size_ip,
                                offset, (ntohs(ip->ip_off) & IP_MF) ? "+" : "");
        }
        printf("\n");

        return;
}



