#include "tcpi.h"
#include "rdt.h"

/* A packet has arrived, process it with
 * connection management scheme by parent. 
 * If the cstate is in the "ESTABLISHED" state,
 * the packet will handed over to the child.
 * Return:
 *   0 - Can't process;
 *   1 - Have processed.
 */

int pkt_arrive(struct conn *cptr, const u_char *pkt, int len)
{
        const struct ip *ip;
        const struct rdthdr *rdthdr;
        int size_ip;
        ssize_t n;
        char buf[MAXLINE];

        ip = (struct ip *)pkt;
        size_ip = ip->ip_hl * 4;
        if (len - size_ip < RDT_LEN)
                return (0);
        rdthdr = (struct rdthdr *)(pkt + size_ip);
	switch (cptr->cstate) {
	case ESTABLISHED:
	{
                fprintf(stderr, "pkt arrived (ESTABLISHED)\n");
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
			fprintf(stderr, "deliveried %zd bytes\n", n);
                        return (1);
		}
                break;
	}
        case LISTEN:
	{
                fprintf(stderr, "pkt arrived (LISTEN)\n");
		if (ip->ip_dst.s_addr == cptr->src.s_addr &&
		    rdthdr->rdt_dcid == cptr->scid) {
                        memcpy(&cptr->dst, &ip->ip_src, sizeof(ip->ip_src));
                        cptr->dcid = rdthdr->rdt_scid;

                        /* Send the ACK to establish a connection */
                        cptr->xfd = make_sock(cptr->dst);
                        n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid, 0, RDT_ACC, NULL, 0, buf);
                        if (n != to_net(cptr->xfd, buf, n, cptr->dst))
                                err_sys("to_net() error");
                        cptr->cstate = ESTABLISHED;
                        fprintf(stderr, "LISTEN -> ESTABLISHED\n");
			return (1);
		}
                break;
	}
        default: break;
        }

        return (0);
}
