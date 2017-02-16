#ifndef _RDT_H
#define _RDT_H

#define MAX_CONN 32
#define IP_LEN 20
#define RDT_LEN 8
#undef  IPPROTO_RDT
#define IPPROTO_RDT 143 /* 143-252 (0x8F-0xFC) UNASSIGNED */

#define ranport() ((getpid() & 0xfff) | 0x8000)

typedef enum {IDLE, WAITING, ESTABLISHED, SENDING, RECEIVING, DISCONN} cstate;

/*
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   SRC CID#    |    DST CID#   |   Seq(0/1)    |     Flags     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Packet length         |            Checksum           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                             Data...                           |
 *
 */

struct rdthdr {
        uint8_t rdt_scid;
        uint8_t rdt_dcid;
        uint8_t rdt_seq;
        uint8_t rdt_flags;
#define RDT_FIN  0x01 /* finish request */
#define RDT_CONF 0x02 /* finish confirm */
#define RDT_REQ  0x04 /* connection request */
#define RDT_ACC  0x08 /* connection accept */
#define RDT_DATA 0x10
#define RDT_ACK  0x20
        uint16_t rdt_len;
        uint16_t rdt_sum;
};

struct conn {
        int fd, scid, dcid;
        cstate state;
        unsigned char *sndbuf;
        unsigned char *rcvbuf;
        int mss;
        int timer;
} conn[MAX_CONN];


struct in_addr getlocaddr(struct in_addr dst);
ssize_t make_pkt(struct in_addr src, struct in_addr dst, int scid, int dcid, int seq, int flags, void *data, size_t nbyte, void *buf);
ssize_t to_net(int fd, const void *buf, size_t nbyte, struct sockaddr *dst, socklen_t len);
ssize_t from_net(int fd, void *buf, size_t nbyte, int *scid, int *dcid, int *seq, int *flags);

#endif

