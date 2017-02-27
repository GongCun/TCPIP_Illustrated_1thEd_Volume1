#include "rdt.h"

/* A packet has arrived, process it with
 * connection management scheme by parent. 
 * If the cstate is in the "ESTABLISHED" state,
 * the packet will handed over to the child.
 * Return:
 *   0 - Can't process;
 *   1 - Have processed.
 */

#define CONN_RET_LEN sizeof(struct conn_ret)

int pkt_arrive(struct conn *cptr, const u_char *pkt, int len)
{
        const struct ip *ip;
        const struct rdthdr *rdthdr;
        int size_ip;
        ssize_t n;
        struct conn_addr conn_addr;

        ip = (struct ip *)pkt;
        size_ip = ip->ip_hl * 4;
        if (len - size_ip < RDT_LEN)
                return (0);
        rdthdr = (struct rdthdr *)(pkt + size_ip);

        fprintf(stderr, "debug before checksum():\n");
        pkt_debug(rdthdr);

        
	if (!chk_chksum((u_short *)(pkt + size_ip), ntohs(rdthdr->rdt_len))) {
	/*if (!chk_chksum((u_short *)(pkt + size_ip), len - size_ip)) { */
                fprintf(stderr, "checksum wrong\n");
		return (0);
        }

        /* Fill the struct conn_addr and pass to child,
         * cause the LISTEN status don't have the partner info.
         */
        bzero(&conn_addr, sizeof(struct conn_addr));
        memcpy(&conn_addr.src, &ip->ip_src, sizeof(ip->ip_src));
        memcpy(&conn_addr.dst, &ip->ip_dst, sizeof(ip->ip_dst));
        conn_addr.scid = rdthdr->rdt_scid;
        conn_addr.dcid = rdthdr->rdt_dcid;

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
                        /* If child process don't read the pipe
                         * will cause an EPIPE error, or other
                         * write() error will be handed over
                         * to the RDT mechanism.
                         */
			n = write(cptr->pfd, (u_char *) (pkt + size_ip), len - size_ip);
			fprintf(stderr, "ESTABLISHED: pass %zd bytes to child\n", n);
                        if (rdthdr->rdt_flags == RDT_FIN) {
                                cptr->cstate = CLOSED;
                                fprintf(stderr, "parent: ESTABLISHED -> CLOSED\n");
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

                        /* Pass the connection info to child */
                        n = pass_pkt(cptr->pfd, &conn_addr, (u_char *)(pkt + size_ip), len - size_ip);
			fprintf(stderr, "LISTEN: pass %zd bytes to child, "
                                        "buf = %d, conn_addr = %zd\n", n, len-size_ip, sizeof(conn_addr));
                        cptr->cstate = ESTABLISHED;

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
			write(cptr->pfd, (u_char *) (pkt + size_ip), len - size_ip);
                        cptr->cstate = ESTABLISHED;
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
                        cptr->cstate = CLOSED;
			fprintf(stderr, "DISCONN: pass %zd bytes to child\n", n);
                        return (1);
                }
                break;
        } /* end of DISCONN */

        default:;

        } /* end of switch() */

        return (0);
}
