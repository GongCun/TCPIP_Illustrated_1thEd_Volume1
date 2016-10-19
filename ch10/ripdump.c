#include "tcpi.h"
#include "drp.h"

#define CMD "udp and port 520" /* The source and destination ports
                                  of RIP are all 520 */

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
        const struct udphdr *udp;
        const struct rip_data *rip, *ptr;
        u_char *payload;
        int size_eth, size_ip;
        int size_udp = sizeof(struct udphdr);
        int size_payload;
        uint8_t cmd, ver;
        uint16_t domain;
        int cnt = 0;
        char addrbuf[INET_ADDRSTRLEN];
        char maskbuf[INET_ADDRSTRLEN];
        char nextbuf[INET_ADDRSTRLEN];

        if (linktype != 0 && linktype != 1)
                err_quit("Unknown link-layer header type: %d", linktype);
        size_eth = (linktype == 0) ? 4 : 14;

        ip = (struct ip *)(packet + size_eth);
        if ((size_ip = ip->ip_hl * 4) < 20)
                err_quit("Invalid IP header length: %d bytes\n", size_ip);

        if (header->caplen - size_eth - size_ip < size_udp)
                err_quit("Invalid UDP header length: %d bytes\n", header->caplen - size_eth - size_ip);

        udp = (struct udphdr *)(packet + size_eth + size_ip);
        assert(udp->uh_sport == htons(520));

        payload = (u_char *)(packet + size_eth + size_ip + size_udp);

        cmd = *(uint8_t *)payload;
        ver = *(uint8_t *)(payload + 1);
        domain = *(uint16_t *)(payload + 2);
        printf("RIP cmd = %zd, version = %zd, domain = %zd\n", cmd, ver, htons(domain));

        rip = (struct rip_data *)(payload + 4);

        if (cmd == 1 && rip->rip_af == (uint16_t)0 && ntohl(rip->rip_metric) == 16)
                printf("Caught RIP request\n");

        ptr = rip;
        size_payload = header->caplen - size_eth - size_ip - size_udp - 4;
        for (; size_payload > 0; ptr += 1, size_payload -= 20) {
                printf("%d: IP address: %s, mask: %s, next: %s, metric: %d\n", cnt++,
                                praddr(&ptr->rip_addr, addrbuf, sizeof(addrbuf)),
                                praddr(&ptr->rip_mask, maskbuf, sizeof(maskbuf)),
                                praddr(&ptr->rip_next_hop, nextbuf, sizeof(nextbuf)),
                                ntohl(ptr->rip_metric));
        }

        return;
}

static char *praddr(const uint32_t *buf, char *str, int len)
{
        u_char *ptr;

        ptr = (u_char *)buf;
        snprintf(str, len, "%d.%d.%d.%d", *ptr, *(ptr+1), *(ptr+2), *(ptr+3));

        return str;
}
