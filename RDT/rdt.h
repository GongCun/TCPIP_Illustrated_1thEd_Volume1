#ifndef _RDT_H
#define _RDT_H

#include "tcpi.h"

#define MAX_CONN 32
#define IP_LEN 20
#define RDT_LEN sizeof(struct rdthdr)
#define GUESS_MTU 1500 
#undef  IPPROTO_RDT
#define IPPROTO_RDT 143 /* 143-252 (0x8F-0xFC) UNASSIGNED */

#define ranport() ((getpid() & 0xfff) | 0x8000)
#define FILTER "ip[9:1] == 143"
#define PCAP_TIMEOUT 500 /* 500 milliseconds */

#define RDT_UX_SOCK "/tmp/RDTUXSock" /* Unix Domain Socket for User IPC */
#define RDT_FIFO "/tmp/RDTPipe" /* FIFO pipe for User IPC */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef enum {CLOSED, LISTEN, WAITING, ESTABLISHED, DISCONN} cstate;

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
#define RDT_SEQ0 0x00
#define RDT_SEQ1 0x01
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
        pid_t pid;
        int xfd, pfd, sfd, scid, dcid;
        struct in_addr src, dst;
        cstate cstate;
	unsigned char *pkt;	/* pkt last sent include IP header */
	int pktlen;		/* length of pkt last sent */
        unsigned char *sndbuf;
        unsigned char *rcvbuf;
        int sndlen, rcvlen;
        unsigned long sndbyte; /* have sent bytes */
        unsigned long rcvbyte; /* have recv bytes */
        /* Have dropped bytes:
         * 1. Received from network layer (IP),
         *    but can't delivery to transport layer (RDT);
         * 2. Have deliveried to RDT, but the data is corrupt.
         */
        unsigned long drpbyte;
} conn[MAX_CONN];

typedef enum {ACTIVE, PASSIVE} cact;
struct conn_info {
        pid_t pid; /* user pid */
        cact cact;
        struct in_addr src, dst;
        int scid, dcid;
};

struct conn_ret {
        int ret;
        int err;
};

extern char dev[IFNAMSIZ];
extern int linktype;
extern int mtu;

struct in_addr get_addr(struct in_addr dst);
ssize_t make_pkt(struct in_addr src, struct in_addr dst, int scid, int dcid, int seq, int flags, void *data, size_t nbyte, void *buf);
ssize_t to_net(int fd, const void *buf, size_t nbyte, struct in_addr dst);
void from_net(void);
int get_dev(struct in_addr addr, char *dev);
int get_mtu(char *dev);
int chk_chksum(u_short *ptr, int len);
pid_t make_child(int, pid_t);
void xmit_pkt(int i);
int krdt_listen(struct in_addr src, int scid);
int krdt_connect(struct in_addr dst, int scid, int dcid);
int pkt_arrive(struct conn *cptr, const u_char *pkt, int len);
int make_sock(void);
ssize_t rexmt_pkt(int i);
void pkt_debug(const struct rdthdr *);
int rdt_connect(struct in_addr dst, int scid, int dcid);
int rdt_listen(struct in_addr src, int scid);
void conn_info_debug(struct conn_info *);

#endif

