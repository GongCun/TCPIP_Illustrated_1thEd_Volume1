#include "tcpi.h"
#include "drp.h"

#define CMD "dst host 224.0.0.5 and ip[9:1] == 89" /* The OSPF protocol is 89,
                                                      class D addr is 224.0.0.5 */



int linktype;
ospf_stat_t ospfstat = DOWN;
int sendfd;
struct sockaddr_in sasend;
struct in_addr recvaddr;

static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);
static char *praddr(const uint32_t *buf, char *str, int len);
static void ospf_init(const u_char *, int);

extern void ospf_write(int fd, char *buf, int ospflen, struct in_addr src, struct in_addr dst, struct sockaddr *to, socklen_t tolen);

int main(int argc, char *argv[])
{
        pcap_t *pt;
        struct bpf_program bp;
        int recvfd;
        const int on = 1;
        struct ifreq ifreq;

        if (argc != 2)
                err_quit("Usage: %s <interface>", basename(argv[0]));

        /* Join the multicast group of 224.0.0.5 for receiving OSPF packet */
        if ((recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_IP)) < 0)
                err_sys("socket recvfd error");
        if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                err_sys("setsockopt SO_REUSEADDR error");
        if (mcast_join(recvfd, argv[1], OSPF_MCAST) < 0)
                err_sys("mcast_join error");
        /*-+- END of Join mcast group -+-*/

        /* Get the IP of interface */
        strncpy(ifreq.ifr_name, argv[1], IFNAMSIZ);
        if (ioctl(recvfd, SIOCGIFADDR, &ifreq) < 0)
                err_sys("ioctl error");
        memcpy(&recvaddr, &((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr, sizeof(struct in_addr));

        /* Create RAW socket for sending to 224.0.0.6 */
        if ((sendfd = socket(AF_INET, SOCK_RAW, IPPROTO_IP)) < 0)
                err_sys("socket sendfd error");
        if (setsockopt(sendfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
                err_sys("setsockopt IP_HDRINCL error");

        bzero(&sasend, sizeof(struct sockaddr_in));
        sasend.sin_family = AF_INET;
        if (inet_pton(AF_INET, OSPF_MCAST_RTR, &sasend.sin_addr) != 1) {
                errno = EINVAL;
                err_sys("inet_pton OSPF_MCAST_RTR error");
        }
        /*-+- END of Creating RAW socket -+-*/

        pt = open_pcap(argv[1], 500, CMD, &linktype, &bp); /* Capture wait 500 milliseconds */

        loop_pcap(pt, &bp, callback, 0); /* Capture unlimited packets */

        return 0;
}

static void callback(u_char *user, const struct pcap_pkthdr *header, const u_char *packet)
{
        const struct ip *ip;
        const struct ospfhdr *ospfhdr;
        int size_eth, size_ip;
        int size_ospfhdr = sizeof(struct ospfhdr);
        uint8_t type;

        if (linktype != 0 && linktype != 1)
                err_quit("Unknown link-layer header type: %d", linktype);
        size_eth = (linktype == 0) ? 4 : 14;

        ip = (struct ip *)(packet + size_eth);
        if ((size_ip = ip->ip_hl * 4) < 20)
                err_quit("Invalid IP header length: %d bytes\n", size_ip);

        if (header->caplen - size_eth - size_ip < size_ospfhdr)
                err_quit("Invalid OSPF header length: %d bytes\n", header->caplen - size_eth - size_ip);

        ospfhdr = (struct ospfhdr *)(packet + size_eth + size_ip);
        type = ospfhdr->ospf_type;
        printf("Capture packet From %s to ", inet_ntoa(ip->ip_src));
        printf("%s\n", inet_ntoa(ip->ip_dst));
        printf("Packet type = %zd, length = %d\n", type, ntohs(ospfhdr->ospf_len));

        switch (ospfstat) {
                case DOWN:
                        if (type != 1) {
                                fprintf(stderr, "The state is DOWN now, but the packet is not HELLO packet\n");
                        } else {
                                ospf_init(packet + size_eth + size_ip + size_ospfhdr,
                                                header->caplen - size_eth - size_ip - size_ospfhdr);
                        }
                        break;
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

static void ospf_init(const u_char *pkt, int length)
{
        const struct ospfhello *ospfhello;
        char sendbuf[MAXLINE];
        char buf[INET_ADDRSTRLEN];
        struct ospfhdr *sendhdr;
        struct ospfhello *sendhello;
        struct in_addr *neighbor;
        int sum = 0;
        int size_ip = sizeof(struct ip);
        int size_ospfhdr = sizeof(struct ospfhdr);
        int size_ospfhello = sizeof(struct ospfhello);
        uint32_t *ptr;

        if (length < sizeof(struct ospfhello)) {
                fprintf(stderr, "The HELLO packet is incomplete");
                return;
        }

        ospfhello = (struct ospfhello *)pkt;
        if (length - sizeof(struct ospfhello) >= 4) {
                ptr = (uint32_t *)(pkt + sizeof(struct ospfhello));
                if (*ptr != (uint32_t)0) {
                        fprintf(stderr, "Neighbor seen is %s, but not 0\n",
                                        praddr(ptr, buf, sizeof(buf)));
                        return;
                }
        }

        bzero(sendbuf, sizeof(sendbuf));
        sendhdr = (struct ospfhdr *)(sendbuf + size_ip);
        sendhdr->ospf_ver = 2;
        sendhdr->ospf_type = 1;
        sendhdr->ospf_len = htons(size_ospfhdr + size_ospfhello + sizeof(struct in_addr));
        sendhdr->ospf_rid = *((uint32_t *)&recvaddr);
        sendhdr->ospf_aid = htonl(0);
        sendhdr->ospf_sum = htons(0); /* calculate later */

        printf("Reply Router ID is %s\n", praddr(&sendhdr->ospf_rid, buf, sizeof(buf)));

        sendhello = (struct ospfhello *)(sendbuf + size_ip + size_ospfhdr);
        sendhello->hello_mask = ospfhello->hello_mask;
        sendhello->hello_intr = ospfhello->hello_intr;
        sendhello->hello_opt = ospfhello->hello_opt;
        sendhello->hello_pri = 0; /* Will be ineligible to
                                     become (Backup) Designated Router */
        sendhello->hello_rdi = ospfhello->hello_rdi;
        sendhello->hello_dr = ospfhello->hello_dr;
        sendhello->hello_bdr = ospfhello->hello_bdr;

        neighbor = (struct in_addr *)(sendbuf + size_ip + size_ospfhdr + size_ospfhello);
        memcpy(neighbor, &recvaddr, sizeof(struct in_addr));
        printf("Reply Neighbor seen is %s\n", inet_ntoa(*neighbor));

        /*
         * The checksum starting with the OSPF packet header but
         * excluding the 64-bit authentication field.
         */
        sum = in_checksum((uint16_t *)(sendbuf + size_ip), sizeof(struct ospfhdr) - 8);
        sum += in_checksum((uint16_t *)(sendbuf + size_ip + size_ospfhdr), size_ospfhello + sizeof(struct in_addr));
        sendhdr->ospf_sum = CKSUM_CARRY(sum);

        ospf_write(sendfd, sendbuf, size_ospfhdr + size_ospfhello + sizeof(struct in_addr),
                        recvaddr, sasend.sin_addr, (struct sockaddr *)&sasend, sizeof(sasend));

        /* ospfstat = INIT; */
        
        return;
}
