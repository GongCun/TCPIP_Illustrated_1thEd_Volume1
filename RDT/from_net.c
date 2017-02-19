#include "tcpi.h"
#include "rdt.h"

int linktype;
static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);

void from_net(void)
{
        pcap_t *pt;
        struct bpf_program bp;

        pt = open_pcap(dev, PCAP_TIMEOUT, FILTER, &linktype, &bp);

        /* Due to signal process, don't use loop_pcap().
         * Any exception is deal with RDT mechanism.
         */
        pcap_loop(pt, -1, callback, NULL);

        return;
}

static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet)
{
        int i;
        const struct ip *ip;
        int size_eth;
        int size_ip;

        /* DLT_NULL of loopback which head is 4-byte
         * DLT_EN10MB of IEEE 802.3 Ethernet
         * (10Mb, 100Mb, 1000Mb, and up) which head is 14-byte
         */
        if (linktype != 0 && linktype != 1)
                return;
        size_eth = (linktype == 0) ? 4 : 14;

        ip = (struct ip *)(packet + size_eth);
        if ((size_ip = ip->ip_hl * 4) < 20)
                return;

        /* Delive data to the child process */
        for (i = 0; i < MAX_CONN; i++) {
                if (pkt_arrive(&conn[i], packet + size_eth, header->caplen - size_eth))
                        return;
        }
        if (i >= MAX_CONN)
                fprintf(stderr, "can't delivery packet\n");
}

