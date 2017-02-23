#include "rdt.h"
#include "rtt.h"

void 
xmit_pkt(int i)
{
	ssize_t n;
	struct conn *cptr;
	struct rdthdr *rdthdr;
	struct rtt_info *rptr;

	cptr = &conn[i];
	rptr = &rtt_info[i];

	if (cptr->cstate == WAITING) {
		rexmt_pkt(i);
	        rdthdr = (struct rdthdr *)cptr->rcvbuf;
	        if (!chk_chksum((u_short *) cptr->rcvbuf, rdthdr->rdt_len))
	        	err_msg("RDT packet is corrupt");
                pkt_debug(rdthdr);
	}
	pause();
	return;
}
