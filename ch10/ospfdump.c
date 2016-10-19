#include "tcpi.h"
#include "rip.h"

#define CMD "host 224.0.0.5 and ip[9:1] == 89" /* The OSPF protocol is 89,
                                                  class D addr is 224.0.0.5 */


int linktype;

static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);
static char *praddr(const uint32_t *buf, char *str, int len);

int main(int argc, char *argv[])
{
        pcap_t *pt;
        struct bpf_program bp;
        int to;

        if (argc != 4)
                err_quit("Usage: %s <interface> <seconds> <#packets>", basename(argv[0]));

        to = atoi(argv[2]);
        to = (to < 0) ? -1 : to*1000;
        pt = open_pcap(argv[1], to, CMD, &linktype, &bp);

        loop_pcap(pt, &bp, callback, atoi(argv[3]));

        return 0;
}


static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet)
{
        const struct ip *ip;
        const struct ospfhdr *ospfhdr;
        int size_eth, size_ip;
        int size_ospf = sizeof(struct ospfhdr);
        uint8_t ver;
        uint8_t type;
        u_short pktlen;
        char ridbuf[INET_ADDRSTRLEN];
        char aidbuf[INET_ADDRSTRLEN];

        if (linktype != 0 && linktype != 1)
                err_quit("Unknown link-layer header type: %d", linktype);
        size_eth = (linktype == 0) ? 4 : 14;

        ip = (struct ip *)(packet + size_eth);
        if ((size_ip = ip->ip_hl * 4) < 20)
                err_quit("Invalid IP header length: %d bytes\n", size_ip);

        if (header->caplen - size_eth - size_ip < size_ospf)
                err_quit("Invalid OSPF header length: %d bytes\n", header->caplen - size_eth - size_ip);

        ospfhdr = (struct ospfhdr *)(packet + size_eth + size_ip);

        ver = ospfhdr->ospf_ver;
        type = ospfhdr->ospf_type;
        pktlen = ntohs(ospfhdr->ospf_len);
        
        printf("OSPF version = %zd, type = %zd, length = %d\n"
                        "Router ID = %s; Area ID = %s\n",
                        ver, type, pktlen,
                        praddr(&ospfhdr->ospf_rid, ridbuf, sizeof(ridbuf)),
                        praddr(&ospfhdr->ospf_aid, aidbuf, sizeof(aidbuf)));

        return;
}

static char *praddr(const uint32_t *buf, char *str, int len)
{
        u_char *ptr;

        ptr = (u_char *)buf;
        snprintf(str, len, "%d.%d.%d.%d", *ptr, *(ptr+1), *(ptr+2), *(ptr+3));

        return str;
}
