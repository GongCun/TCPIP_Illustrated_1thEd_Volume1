#include "rdt.h"

/* Receive network frame from below */
ssize_t rdt_recv(void *buf, size_t nbyte)
{
	int ret, n;
	static int seq = 0;
	struct rdthdr *rptr;
	struct conn_user *cptr;

	cptr = &conn_user;

	n = read(cptr->pfd, cptr->pkt, cptr->mss);
	rptr = (struct rdthdr *)cptr->pkt;

        fprintf(stderr, "> rdt_recv() expect seq: %d\n", seq);
        fprintf(stderr, "> recv pkt:\n");
        pkt_debug(rptr);

	while((n < sizeof(struct rdthdr)) || ((rptr->rdt_seq) != seq) ||
			(!chk_chksum((uint16_t *)cptr->pkt, ntohs(rptr->rdt_len))))
	{
		/* Send _NoAck_ packet */
		n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid,
                        seq, RDT_ACK, NULL, 0, cptr->pkt);
		if ((n = to_net(cptr->sfd, cptr->pkt, n, cptr->dst)) < 0)
			return(n);

		/* Wait packet again */
		n = read(cptr->pfd, cptr->pkt, cptr->mss);
		rptr = (struct rdthdr *)cptr->pkt;

                fprintf(stderr, "> recv pkt:\n");
                pkt_debug(rptr);
	}

	/* Now get the correct pkt, ack the partner, and delivery 
 	 * data to user.
	 */
	n -= RDT_LEN;
	if (n > nbyte)
		err_quit("recv %d bytes exceed the buf size %d\n", n, nbyte);
	memcpy(buf, (void *)cptr->pkt + RDT_LEN, n);
	ret = n;

	n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid,
			seq, RDT_ACK, NULL, 0, cptr->pkt);
	if ((n = to_net(cptr->sfd, cptr->pkt, n, cptr->dst)) < 0)
		return(n);
	seq = (seq + 1) % 2;

	return(ret);

}


