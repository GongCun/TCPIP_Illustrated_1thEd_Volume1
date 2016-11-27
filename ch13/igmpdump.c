#include "tcpi.h"
#include <time.h>
#include <netinet/igmp.h>

#define FILTER "ip[9:1] == 2"

int linktype;

static void igmpdump(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char *argv[])
{
        pcap_t *pt;
        struct bpf_program bp;

        if (argc != 2)
                err_quit("Usage: igmpdump <interface>");


        pt = open_pcap(argv[1], 500, FILTER, &linktype, &bp);
        loop_pcap(pt, &bp, igmpdump, 0);

        return 0;
}

static void igmpdump(u_char *user, const struct pcap_pkthdr *header, const u_char *packet)
{
        const struct ip *ip;
        const struct igmp *igmp;
        int size_eth;
        int size_ip;
        char tmbuf[64];
        struct tm *tm;
        time_t time;

        if (linktype != 0 && linktype != 1)
                err_quit("Unknown link-layer header type: %d", linktype);

        size_eth = (linktype == 0) ? 4 : 14;

        ip = (struct ip *)(packet + size_eth);
        if ((size_ip = ip->ip_hl * 4) < sizeof(struct ip)) {
                err_msg("Invalid IP header length: %d bytes\n", size_ip);
                return;
        }

        if (ip->ip_p != IPPROTO_IGMP) {
                err_msg("Only check the IGMP packet");
                return;
        }

        /* get daytime */
        time = (time_t)header->ts.tv_sec;
        tm = localtime(&time);
        strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", tm);
        printf("%s.%06d ", tmbuf, header->ts.tv_usec);


        int size_igmp = header->caplen - size_eth - size_ip;
        if (size_igmp < sizeof(struct igmp))
                err_quit("IGMP too short message");
        igmp = (struct igmp *)(packet + size_eth + size_ip);
        int len = ntohs(ip->ip_len) - (ip->ip_hl * 4);

        printf("IP %s > ", inet_ntoa(ip->ip_src));
        printf("%s: IGMP ", inet_ntoa(ip->ip_dst));

        switch (igmp->igmp_type) {
                case 0x11:
                        if (len >= 12) {
                                printf("v3");
                        } else {
                                if (igmp->igmp_code)
                                        printf("v2");
                                else
                                        printf("v1");
                        }
                        printf(" query ");
                        /* 
                         * The group address field is set to zero
                         * when sending a General Query,
                         * and set to the group address being queried
                         * when sending a Group-Specific Query.
                         */
                        if (ntohl(*((const uint32_t *)&igmp->igmp_group)))
                                printf("[gattr %s]", inet_ntoa(igmp->igmp_group));
                                        
                        if (igmp->igmp_code)
                                printf(" [max resp time %d]", igmp->igmp_code);
                        break;
                case 0x16:
                        printf("v2 report %s", inet_ntoa(igmp->igmp_group));
                        break;
                case 0x17: /* v1 or v2 */
                        printf("leave %s", inet_ntoa(igmp->igmp_group));
                        break;
                case 0x12:
                        printf("v1 report %s", inet_ntoa(igmp->igmp_group));
                        break;
                default:
                        printf("type 0x%x", igmp->igmp_type);
        }
        printf(" [ttl %d]\n", ip->ip_ttl);

        return;
}

