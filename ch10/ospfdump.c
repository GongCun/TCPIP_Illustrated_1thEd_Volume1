#include "tcpi.h"
#include "drp.h"

#define CMD "host 224.0.0.5 and ip[9:1] == 89" /* The OSPF protocol is 89,
                                                  class D addr is 224.0.0.5 */


int linktype;

static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);
static char *praddr(const uint32_t *buf, char *str, int len);
static void pkt_hello(const u_char *, int);
static void pkt_dbd(const u_char *, int);
static void pkt_lsack(const u_char *, int);
static void pkt_lsahdr(const struct ospflsahdr *);

int main(int argc, char *argv[])
{
        pcap_t *pt;
        struct bpf_program bp;
        int to;
        int sockfd;
        const int on = 1;

        if (argc != 4)
                err_quit("Usage: %s <interface> <seconds> <#packets>", basename(argv[0]));

         if (setvbuf(stdout, NULL, _IONBF, 0) < 0)
                 err_sys("setvbuf stdout error");
         if (setvbuf(stderr, NULL, _IONBF, 0) < 0)
                 err_sys("setvbuf stderr error");

        /* Join the multicast group of 224.0.0.5 */
        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_IP)) < 0)
                err_sys("socket error");
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                err_sys("setsockopt SO_REUSEADDR error");
        if (mcast_join(sockfd, argv[1], "224.0.0.5") < 0)
                err_sys("mcast_join error");
        /* End join mcast group */

        to = (argv[2] < 0) ? -1 : (int)(atof(argv[2]) * 1000);
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
        printf("From %s to ", inet_ntoa(ip->ip_src));
        printf("%s\n", inet_ntoa(ip->ip_dst));

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

        switch (type) {
                case 1:
                        printf(">>> Hello Packet <<<\n");
                        pkt_hello(packet + size_eth + size_ip + size_ospf,
                                        header->caplen - size_eth - size_ip - size_ospf);
                        break;
                case 2:
                        printf(">>> DD Packet <<<\n");
                        pkt_dbd(packet + size_eth + size_ip + size_ospf,
                                        header->caplen - size_eth - size_ip - size_ospf);
                        break;
                case 5:
                        printf(">>> Link State Acknowledgement <<<\n");
                        pkt_lsack(packet + size_eth + size_ip + size_ospf,
                                        header->caplen - size_eth - size_ip - size_ospf);
                default:
                        ;
        }

        printf("\n");

        return;
}

static char *praddr(const uint32_t *buf, char *str, int len)
{
        u_char *ptr;

        ptr = (u_char *)buf;
        snprintf(str, len, "%d.%d.%d.%d", *ptr, *(ptr+1), *(ptr+2), *(ptr+3));

        return str;
}

static void pkt_hello(const u_char *pkt, int length)
{
        const struct ospfhello *ospfhello;
        char buf[INET_ADDRSTRLEN];
        int neighbor;
        const struct in_addr *ptr;

        if (length < sizeof(struct ospfhello)) {
                fprintf(stderr, "The HELLO packet is incomplete\n");
                return;
        }
        ospfhello = (struct ospfhello *)pkt;
        printf("Network Mask: %s\n"
                        "HelloInterval: %d\n"
                        "Options: %03o\n"
                        "Rtr Pri: %d\n"
                        "RouterDeadInterval: %d\n",
                        praddr(&ospfhello->hello_mask, buf, sizeof(buf)),
                        ntohs(ospfhello->hello_intr),
                        ospfhello->hello_opt,
                        ospfhello->hello_pri,
                        ntohl(ospfhello->hello_rdi));
        printf("Designated Router: %s\n", praddr(&ospfhello->hello_dr, buf, sizeof(buf)));
        printf("Backup Designated Router: %s\n", praddr(&ospfhello->hello_bdr, buf, sizeof(buf)));

        printf("Neighbor Seen: ");
        neighbor = length - sizeof(struct ospfhello);
        ptr = (struct in_addr *)(pkt + sizeof(struct ospfhello));
        if (neighbor == 0) {
                printf("0.0.0.0\n");
        } else {
                while (neighbor > 0) {
                        printf("%s ", inet_ntoa(*ptr));
                        neighbor -= sizeof(struct in_addr);
                        ptr += 1;
                }
                printf("\n");
        }
        
        return;
}

static void pkt_dbd(const u_char *pkt, int length)
{
        const struct ospfdbd *ospfdbd;

        if (length < sizeof(struct ospfdbd)) {
                fprintf(stderr, "The DB Description packet is incomplete\n");
                return;
        }
        ospfdbd = (struct ospfdbd *)pkt;
        printf("Interface MTU: %d, Options: %d\n", ntohs(ospfdbd->dbd_mtu), ospfdbd->dbd_opt);
        if ((ospfdbd->dbd_flag & 1) != 0) printf("MS-bit ");
        if ((ospfdbd->dbd_flag & 2) != 0) printf("M-bit ");
        if ((ospfdbd->dbd_flag & 4) != 0) printf("I-bit ");
        printf("\n");
        printf("DD sequence number: %d\n", ntohl(ospfdbd->dbd_seq));

        return;
}

static void pkt_lsack(const u_char *pkt, int length)
{
        const struct ospflsahdr *ospflsahdr;

        if (length < sizeof(struct ospflsahdr)) {
                fprintf(stderr, "The Link State Acknowledgment packet is incomplete\n");
                return;
        }

        for (ospflsahdr = (const struct ospflsahdr *)pkt; length > 0;
                        length -= sizeof(struct ospflsahdr), ospflsahdr += 1)
                pkt_lsahdr(ospflsahdr);
        printf("\n");

        return;
}

static void pkt_lsahdr(const struct ospflsahdr *lsahdr)
{
        printf("[LSA Header]\n");
        printf("LS Age: %d sec\n", ntohs(lsahdr->lsa_age));
        printf("Options: %03o\n", lsahdr->lsa_opt);
        printf("LS Type: %d\n", lsahdr->lsa_type);
        printf("Link State ID: %s\n", inet_ntoa(*((struct in_addr *)&lsahdr->lsa_id)));
        printf("Advertising Router: %s\n", inet_ntoa(*((struct in_addr *)&lsahdr->lsa_adv)));
        printf("LS Sequence Number: %zd\n", ntohl(lsahdr->lsa_seq));
        printf("LS Length: %d\n", ntohs(lsahdr->lsa_len));
        return;
}

