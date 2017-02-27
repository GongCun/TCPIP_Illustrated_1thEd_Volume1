#include "rdt.h"
#include "rtt.h"

#define CONN_RET_LEN sizeof(struct conn_ret)
#define RDTHDR_LEN sizeof(struct rdthdr)

/* For end the data transmission */
static sigjmp_buf jmptime;
static void sig_alrm(int);

void 
xmit_pkt(int i)
{
	int n, ret;
	struct conn *cptr;
	struct rdthdr *rdthdr;
	struct rtt_info *rptr;
        struct conn_ret conn_ret;
        struct conn_addr conn_addr;
        fd_set rset;
        int maxfd;
        int flags, status, rcvseq, rcvlen;
        static int fsminit, seq;

	cptr = &conn[i];
	rptr = &rtt_info[i];

        if (signal(SIGALRM, sig_alrm) == SIG_ERR)
                err_dump("signal() SIGALRM error");

        while (1) {

	        switch (cptr->cstate) {
	
	        case LISTEN:
		{
			/* Fill partner info */
	                fprintf(stderr, "xmit_pkt() cptr->rcvlen = %d\n", cptr->rcvlen);
			n = get_pkt(cptr->pfd, &conn_addr, cptr->rcvbuf, cptr->rcvlen);
	                fprintf(stderr, "LISTEN: get %zd bytes from parent\n", n);
			memcpy(&cptr->dst, &conn_addr.src, sizeof(conn_addr.src));
			cptr->dcid = conn_addr.scid;
	
			/* Send the ACK to establish a connection */
			cptr->xfd = make_sock();
			if (cptr->pkt == NULL &&
			    (cptr->pkt = malloc(mtu)) == NULL) {
				err_dump("malloc() of pkt error");
			}
			n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid, 0, RDT_ACC, NULL, 0, cptr->pkt);
			cptr->pktlen = n;
			if (to_net(cptr->xfd, cptr->pkt, cptr->pktlen, cptr->dst) != n) {
				conn_ret.ret = -1;
				conn_ret.err = EINVAL;
				if (write(cptr->sfd, &conn_ret, CONN_RET_LEN) != CONN_RET_LEN) {
					err_dump("write() error");
				}
	                        return;
			}
			cptr->cstate = ESTABLISHED;
			fprintf(stderr, "LISTEN -> ESTABLISHED\n");
			conn_ret.ret = 0;
			conn_ret.err = 0;
			if (write(cptr->sfd, &conn_ret, CONN_RET_LEN) != CONN_RET_LEN) {
				err_dump("write() error");
			}
	                break;
		}
	
	        case WAITING:
	        {
			if ((ret = rexmt_pkt(i)) < 0) {
	                        conn_ret.ret = ret;
	                        conn_ret.err = errno; 
	                        printf("errno = %d\n", errno);
	                        if (write(cptr->sfd, &conn_ret, CONN_RET_LEN) != CONN_RET_LEN)
	                                err_dump("write() error");
	                        return;
	                }
	                conn_ret.ret = 0;
	                conn_ret.err = 0;
	                if (write(cptr->sfd, &conn_ret, CONN_RET_LEN) != CONN_RET_LEN)
	                        err_dump("write() error");
			cptr->cstate = ESTABLISHED;
	                fprintf(stderr, "WAITING -> ESTABLISHED\n");
		        rdthdr = (struct rdthdr *)cptr->rcvbuf;
	                fprintf(stderr, "the following pkt has been processed:\n");
	                pkt_debug(rdthdr);
	                break;
		} /* end of WAITING */
	
	        case ESTABLISHED:
		{
                        if (fsminit++ == 0) {
                                seq = 0;
                                cptr->sndstate = ABOVE0;
                                cptr->rcvstate = BELOW0;
                        }
	                FD_ZERO(&rset);
	                FD_SET(cptr->pfd, &rset); /* from user */
	                FD_SET(cptr->sfd, &rset); /* from network */
	                maxfd = max(cptr->pfd, cptr->sfd);
	                
	again:
			if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
				if (errno == EINTR)
					goto again;
				else
					err_dump("select() error");
			}
			if (FD_ISSET(cptr->pfd, &rset)) {
                                /* network frame arrival, suppose the checksum is right
                                 * after parent process checked */
				n = read(cptr->pfd, cptr->rcvbuf, cptr->rcvlen);
                                rcvlen = n;

	                        rdthdr = (struct rdthdr *)(cptr->rcvbuf);
	
	                        switch (flags = rdthdr->rdt_flags) {
	
		                case RDT_FIN:
		                {
					n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid, 0, RDT_CONF, NULL, 0, cptr->pkt);
					cptr->pktlen = n;
					/*
					 * TBC: Is neccessary to check whether to_net()
					 * succeed ?
					 */
					to_net(cptr->xfd, cptr->pkt, n, cptr->dst);
					cptr->cstate = CLOSED;
                                        goto close;
				}

                                /* AckPkt arrived event */
	                        case RDT_ACK:
	                        {
                                        status = cptr->sndstate;
                                        if (rdthdr->rdt_seq == RDT_SEQ0 && status == ACK0) {
                                                cptr->sndstate = ABOVE1;
                                                alarm(0);
                                        } else if (rdthdr->rdt_seq == RDT_SEQ1 && status == ACK1) {
                                                cptr->sndstate = ABOVE0;
                                                alarm(0);
                                        }
                                        
	                                break;
	                        }

                                /* DataPkt arrived event */
	                        case RDT_DATA:
	                        {
                                        rcvseq = rdthdr->rdt_seq;
                                        if ((cptr->rcvstate == BELOW0 && rcvseq == RDT_SEQ1) || 
                                            (cptr->rcvstate == BELOW1 && rcvseq == RDT_SEQ0))
                                        {
                                                n = make_pkt(cptr->src, cptr->dst, cptr->scid,
                                                                cptr->dcid, rcvseq, RDT_ACK, NULL, 0, cptr->pkt);
                                                if (to_net(cptr->xfd, cptr->pkt, cptr->pktlen, cptr->dst) != n) {
                                                        err_dump("to_net() error");
                                                }
                                        } else if ((cptr->rcvstate == BELOW0 && rcvseq == RDT_SEQ0) ||
                                            (cptr->rcvstate == BELOW1 && rcvseq == RDT_SEQ1)) {
                                                /* deliver data to user */
                                                n = rcvlen - RDTHDR_LEN; 
                                                if (write(cptr->sfd, (void *)(cptr->rcvbuf + RDTHDR_LEN), n) != n) {
                                                        err_dump("write() to user error"); 
                                                }

                                                /* reply ACK to sender */
                                                n = make_pkt(cptr->src, cptr->dst, cptr->scid,
                                                                cptr->dcid, rcvseq, RDT_ACK, NULL, 0, cptr->pkt);
                                                if (to_net(cptr->xfd, cptr->pkt, cptr->pktlen, cptr->dst) != n) {
                                                        err_dump("to_net() error");
                                                }
                                        }

                                        if (cptr->rcvstate == BELOW0)
                                                cptr->rcvstate = BELOW1;
                                        else
                                                cptr->rcvstate = BELOW0;

	                                break;
	                        }
	
	                        default:
	                        {
	                                err_quit("can't process flags: %d\n", flags);
	                        }
	                        } /* end of switch(flags) */
	                } /* end of process network frame */

                        /* SEND DataPkt event */
	                if (FD_ISSET(cptr->sfd, &rset)) {
	                        n = read(cptr->sfd, cptr->sndbuf, cptr->sndlen);
                                cptr->pktlen = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid, (seq++ % 2), RDT_DATA, cptr->sndbuf, n, cptr->pkt);
                                n = cptr->pktlen;
                                fprintf(stderr, "debug before to_net():\n");
                                pkt_debug((struct rdthdr *)(cptr->pkt + IP_LEN));
                                if (to_net(cptr->xfd, cptr->pkt, n, cptr->dst) != n)
                                {
                                        err_dump("to_net() error");
                                }
                                rtt_newpack(rptr);
                                fprintf(stderr, "to_net() RTT\n");
                                rtt_debug(rptr);
                                alarm(rtt_start(rptr));
                                status = cptr->sndstate;
                                switch (status) {
                                case ABOVE0:
                                        cptr->sndstate = ACK0;
                                        break;
                                case ABOVE1:
                                        cptr->sndstate = ACK1;
                                        break;
                                default:
                                        err_quit("unknown send status: %d\n", status);
                                }

	                } /* end of process user data */

                        /* TIMEOUT event */
                        if (sigsetjmp(jmptime, 1) != 0) {
                                if (rtt_timeout(rptr) < 0) {
                                        errno = ETIMEDOUT;
                                        err_dump("wait ACK timed out");
                                }
                                rtt_debug(rptr);
                                if ((n = to_net(cptr->xfd, cptr->pkt, cptr->pktlen, cptr->dst)) < 0)
                                {
                                        goto close;
                                }
                                alarm(rtt_start(rptr));
                                /* Don't change the waiting state */
                        }
                                        
	                break;
	        } /* end of ESTABLISHED */
	
	        default:
	        {
	                err_quit("Abnormal state when pkt arrived: %d\n", cptr->cstate);
	
	        }
	
	        } /* end of switch() */

        } /* end of loop */
close:
	return;
}

static void sig_alrm(int signo)
{
        siglongjmp(jmptime, 1);
}
