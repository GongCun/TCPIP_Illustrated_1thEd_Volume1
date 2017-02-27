#include "rdt.h"
#include "rtt.h"

/* connect() for connect to remote process by sending RDT_REQ packet,
 * If received RDT_ACC packet, a active connection would be established.
 * The variables will be initialized:
 * src          sour address
 * dst          dest address
 * scid         sour conn id - search for conn[] to find unused scid
 * dcid         dest conn id
 * sndbuf       send buff of user data
 * sndlen       send buff len of user data
 * rcvbuf       recv buff of RDT data
 * rcvlen       recv buff len of RDT header plus user data
 * cstate       CLOSED->WAITING
 */

int krdt_connect(struct in_addr dst, int scid, int dcid)
{
        int i, j, n;
        struct conn *cptr;
        struct in_addr src;
        struct rtt_info *rptr;
        
        /* Choose a scid internal if 
         * not in the legal range.
         */
	if (scid < 0 || scid >= MAX_CONN) {
		for (i = 0; i < MAX_CONN; i++) {
			for (j = 0; j < MAX_CONN; j++) {
				if (i == conn[j].scid && conn[j].cstate != CLOSED)
					break;
			}
			if (j >= MAX_CONN)	/* non-dup */
				break;
		}
		if (i >= MAX_CONN)
			err_quit("Can't start connect() because the conn-id is full.");
		scid = i;
	} else {		/* Check the scid if duplicate */
		for (i = 0; i < MAX_CONN; i++)
			if (conn[i].cstate != CLOSED && conn[i].scid == scid)
				err_quit("Can't start connect() because the SCID is in use.");
	}

        for (i = 0; i < MAX_CONN; i++)
                if (conn[i].cstate == CLOSED)
                        break;
        if (i >= MAX_CONN)
                err_quit("Can't start connect() because the connection is full.");

        cptr = &conn[i];
        cptr->scid = scid;
        cptr->dcid = dcid;
        src = get_addr(dst);
        memcpy(&cptr->src, &src, sizeof(src));
        memcpy(&cptr->dst, &dst, sizeof(dst));

        if (!mtu) {
                if (dev[0] == 0 && !get_dev(src, dev))
                        err_quit("can't get dev name");
                mtu = get_mtu(dev);
        }

        cptr->sndlen = mtu - IP_LEN - RDT_LEN;
        cptr->rcvlen = mtu - IP_LEN;
        if (cptr->sndbuf == NULL &&
                        (cptr->sndbuf = malloc(cptr->sndlen)) == NULL)
        {
                err_sys("malloc() of sndbuf error");
        }
        if (cptr->rcvbuf == NULL &&
                        (cptr->rcvbuf = malloc(cptr->rcvlen)) == NULL)
        {
                err_sys("malloc() of rcvbuf error");
        }
        if (cptr->pkt == NULL &&
                        (cptr->pkt = malloc(mtu)) == NULL)
        {
                err_sys("malloc() of pkt error");
        }
        cptr->xfd = make_sock();
        n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid, 0, RDT_REQ, NULL, 0, cptr->pkt);
        cptr->pktlen = n;
        if (n != to_net(cptr->xfd, cptr->pkt, n, cptr->dst))
                err_sys("to_net() error");

        /* Initialize the RTO for retransmission */
        rtt_alloc(MAX_CONN);
        rptr = rtt_info+i;
        rtt_init(rptr);
        rtt_newpack(rptr);
        /* rptr->rtt_ts = rtt_setts(rptr); */
        rtt_debug(rptr);

        cptr->cstate = WAITING;
        return (i);
}
