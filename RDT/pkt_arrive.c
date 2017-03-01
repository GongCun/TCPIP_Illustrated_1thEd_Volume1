#include "rdt.h"


/*
 * When a packet has arrived, delivery it to user process.
 *
 * Return:
 *   0 - Can't process;
 *   1 - Have processed.
 *
 */


int pkt_arrive(struct conn *cptr, const u_char *pkt, int len)
{
        const struct ip *ip;
        const struct rdthdr *rdthdr;
        int size_ip;
        ssize_t n;
        struct conn_info conn_info;

        ip = (struct ip *)pkt;
        size_ip = ip->ip_hl * 4;
        if (len - size_ip < RDT_LEN)
                return (0);
        rdthdr = (struct rdthdr *)(pkt + size_ip);

	if (!chk_chksum((uint16_t *)(pkt + size_ip), ntohs(rdthdr->rdt_len))) {
                fprintf(stderr, "pkt_arrive() checksum wrong\n");
		return (0);
        } else 
		fprintf(stderr, "pkt_arrive() checksum right\n");

        /* Fill the struct conn_info and pass to user,
         * cause the LISTEN status don't have the partner info.
         */
        bzero(&conn_info, sizeof(struct conn_info));
        memcpy(&conn_info.src, &ip->ip_src, sizeof(ip->ip_src));
        memcpy(&conn_info.dst, &ip->ip_dst, sizeof(ip->ip_dst));
        conn_info.scid = rdthdr->rdt_scid;
        conn_info.dcid = rdthdr->rdt_dcid;

	switch (cptr->cstate) {
	case ESTABLISHED:
	{
                switch (rdthdr->rdt_flags) {
                        case RDT_CONF:
                        case RDT_REQ:
                        case RDT_ACC:
                                return (0);
                        default:;
                }
		if (ip->ip_src.s_addr == cptr->dst.s_addr &&
		    ip->ip_dst.s_addr == cptr->src.s_addr &&
		    rdthdr->rdt_scid == cptr->dcid &&
		    rdthdr->rdt_dcid == cptr->scid)
                {
                        /* If user process don't read the pipe
                         * will cause an EPIPE error, or other
                         * write() error will be handed over
                         * to the RDT mechanism.
                         */
			n = write(cptr->pfd, (u_char *) (pkt + size_ip), len - size_ip);
			fprintf(stderr, "ESTABLISHED: pass %zd bytes to user\n", n);
                        if (rdthdr->rdt_flags == RDT_FIN) {
                                cptr->cstate = CLOSED;
                                fprintf(stderr, "pkt_arrive(): ESTABLISHED -> CLOSED\n");
                        }
                        return (1);
		}
                break;
	}
        case LISTEN:
	{
                if (rdthdr->rdt_flags != RDT_REQ)
                {
                        return (0);
                }

		if (ip->ip_dst.s_addr == cptr->src.s_addr &&
		    rdthdr->rdt_dcid == cptr->scid &&
                    rdthdr->rdt_flags == RDT_REQ) {
                        pkt_debug(rdthdr);
                        memcpy(&cptr->dst, &ip->ip_src, sizeof(ip->ip_src));
                        cptr->dcid = rdthdr->rdt_scid;

                        /* Pass the connection info to user */
                        n = pass_pkt(cptr->pfd, &conn_info, (u_char *)(pkt + size_ip), len - size_ip);
			fprintf(stderr, "LISTEN: pass %zd bytes to user, "
                                        "buf = %d, conn_info = %zd\n", n, len-size_ip, sizeof(conn_info));
                        cptr->cstate = ESTABLISHED;
			fprintf(stderr, "pkt_arrive(): LISTEN -> ESTABLISHED\n");

			return (1);
		}
                break;
	} /* end of LISTEN */
        case WAITING:
        {
                if (rdthdr->rdt_flags != RDT_ACC)
                        return (0);
		if (ip->ip_src.s_addr == cptr->dst.s_addr &&
		    ip->ip_dst.s_addr == cptr->src.s_addr &&
		    rdthdr->rdt_scid == cptr->dcid &&
		    rdthdr->rdt_dcid == cptr->scid)
                {
			n = write(cptr->pfd, (u_char *) (pkt + size_ip), len - size_ip);
                        cptr->cstate = ESTABLISHED;
			fprintf(stderr, "pkt_arrive(): WAITING -> ESTABLISHED\n");
                        return (1);
                }
                break;
        }
        case DISCONN:
        {
                /* In theory it should still receive Data or Ack in the half-closed state,
                 * but if user's socket was closed, we can't delivery the data to above,
                 * it will be improved in future.
                 */
                if (rdthdr->rdt_flags != RDT_CONF)
                {
                        return(0);
                }
		if (ip->ip_src.s_addr == cptr->dst.s_addr &&
		    ip->ip_dst.s_addr == cptr->src.s_addr &&
		    rdthdr->rdt_scid == cptr->dcid &&
		    rdthdr->rdt_dcid == cptr->scid)
                {
			n = write(cptr->pfd, (u_char *) (pkt + size_ip), len - size_ip);
			fprintf(stderr, "DISCONN: pass %zd bytes to user\n", n);
                        cptr->cstate = CLOSED;
			fprintf(stderr, "pkt_arrive(): DISCONN -> ESTABLISHED\n");
                        return (1);
                }
                break;
        } /* end of DISCONN */

        default:;

        } /* end of switch() */

        return (0);
}
