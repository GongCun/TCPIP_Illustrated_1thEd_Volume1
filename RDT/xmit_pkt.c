#include "tcpi.h"
#include "rdt.h"

void xmit_pkt(int i)
{
        ssize_t n;
        struct conn *cptr;
        struct rdthdr *rdthdr;

        cptr = &conn[i];
#define rxbuf cptr->rcvbuf
#define rxlen cptr->rcvlen
        while ((n = read(cptr->pfd, rxbuf, rxlen)) > 0) {
                rdthdr = (struct rdthdr *)rxbuf;
                if (!chk_chksum((u_short *)rxbuf, rdthdr->rdt_len))
                        err_msg("RDT packet is corrupt");
                fprintf(stderr, "%d %d %02x %02x %d\n",
                                rdthdr->rdt_scid,
                                rdthdr->rdt_dcid,
                                rdthdr->rdt_seq,
                                rdthdr->rdt_flags,
                                ntohs(rdthdr->rdt_len));
        }
        return;
}

