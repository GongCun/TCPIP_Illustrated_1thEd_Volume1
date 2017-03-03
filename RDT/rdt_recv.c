#include "rdt.h"

/* Receive network frame from below */
ssize_t rdt_recv(void *buf, size_t nbyte)
{
	int ret, n, seq;
	struct rdthdr *rptr;
	struct conn_user *cptr;

	cptr = &conn_user;
	seq = cptr->rseq;

	n = read(cptr->rcvfd, cptr->rcvpkt, cptr->mss);
	rptr = (struct rdthdr *)cptr->rcvpkt;

        fprintf(stderr, "> rdt_recv() expect seq: %d\n", seq);
        fprintf(stderr, "> recv pkt:\n");
        pkt_debug(rptr);

	while((n < sizeof(struct rdthdr)) || ((rptr->rdt_seq) != seq) ||
			(!chk_chksum((uint16_t *)cptr->rcvpkt, ntohs(rptr->rdt_len))))
	{
		/* Send _NoAck_ packet */
		n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid,
                        seq, RDT_ACK, NULL, 0, cptr->rcvpkt);
		if ((n = to_net(cptr->sfd, cptr->rcvpkt, n, cptr->dst)) < 0)
			return(n);

		/* Wait packet again */
		n = read(cptr->rcvfd, cptr->rcvpkt, cptr->mss);
		rptr = (struct rdthdr *)cptr->rcvpkt;

                fprintf(stderr, "> recv pkt:\n");
                pkt_debug(rptr);
	}

	/* Now get the correct pkt, ack the partner, and delivery 
 	 * data to user.
	 */
	n -= RDT_LEN;
	if (n > nbyte || n < 0)
		err_quit("recv %d bytes exceed the buf size %d\n", n, nbyte);

	if (n > 0) {
		memcpy(buf, (void *)cptr->rcvpkt + RDT_LEN, n);
		ret = n;
	} else {
		ret = 0;
	}


	n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid,
			seq, RDT_ACK, NULL, 0, cptr->rcvpkt);
	cptr->rseq = (cptr->rseq + 1) % 2;

	if ((n = to_net(cptr->sfd, cptr->rcvpkt, n, cptr->dst)) < 0)
		return(n);

        if (ret == 0) {
                close(cptr->sfd);
                close(cptr->pfd);
                close(cptr->sndfd);
                close(cptr->rcvfd);
                bzero(cptr, sizeof(struct conn_user));
        }

	return(ret);

}


