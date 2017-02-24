#include "rdt.h"
#include "rtt.h"

#define CONN_RET_LEN sizeof(struct conn_ret)

void 
xmit_pkt(int i)
{
	int ret;
	struct conn *cptr;
	struct rdthdr *rdthdr;
	struct rtt_info *rptr;
        struct conn_ret conn_ret;

	cptr = &conn[i];
	rptr = &rtt_info[i];

	if (cptr->cstate == WAITING) {
		if ((ret = rexmt_pkt(i)) < 0) {
                        conn_ret.ret = ret;
                        conn_ret.err = errno; 
                        if (write(cptr->sfd, &conn_ret, CONN_RET_LEN) != CONN_RET_LEN)
                                err_sys("write() error");
                }
	        rdthdr = (struct rdthdr *)cptr->rcvbuf;
	        if (!chk_chksum((u_short *) cptr->rcvbuf, rdthdr->rdt_len)) {
                        conn_ret.ret = -1;
                        conn_ret.err = EINVAL; 
                        if (write(cptr->sfd, &conn_ret, CONN_RET_LEN) != CONN_RET_LEN)
                                err_sys("write() error");
                }
                conn_ret.ret = 0;
                conn_ret.err = 0;
                if (write(cptr->sfd, &conn_ret, CONN_RET_LEN) != CONN_RET_LEN)
                        err_sys("write() error");
                pkt_debug(rdthdr);
	}
	pause();
	return;
}
