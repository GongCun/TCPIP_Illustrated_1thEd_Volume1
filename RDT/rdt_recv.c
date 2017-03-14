#include "rdt.h"

static ssize_t nwrite(int fd, const void *buf, size_t nbyte);


/* Receive network frame from below */
void 
rdt_recv(int fd)
{
	int len, n;
	int base, nextack, ack;
	struct rdthdr *rptr;
	struct conn_user *cptr;
        struct rcvlist *plist, *rcvhead, *rcvlist;

        cptr = conn_user;
        base = cptr->ack;
        rcvlist = rcvhead = NULL;

	for (;;) {


                do {
                        /* Just wait, don't send any pkt */
                        n = read(cptr->rcvfd, cptr->rcvpkt, cptr->mss);
                        rptr = (struct rdthdr *)cptr->rcvpkt;

                } while ((n < sizeof(struct rdthdr)) ||
                         (!chk_chksum((uint16_t *) cptr->rcvpkt, ntohs(rptr->rdt_len))));

                /* Now we get the notcorrupt pkt */
		fprintf(stderr, "> rdt_recv() expect seq: %d\n", base);
		fprintf(stderr, "> recv pkt:\n");
		pkt_debug(rptr);

                
                /*
                 * If the seq is legal, respond the ack regardless
                 * whether the seq num is expected.
                 */
                len = n;
                ack = rptr->rdt_seq;
                if (ack < base || ack >= base + WINSIZE) {
                        continue;
                }
		n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid,
			     ack, RDT_ACK, NULL, 0, cptr->rcvpkt);

		if ((n = to_net(cptr->sfd, cptr->rcvpkt, n, cptr->dst)) < 0)
                        err_sys("rdt_recv(): to_net() Ack error");

                /*
                 * If the seq is exected, pop the list and delivery
                 * data the user.
                 */
                if (ack == base) {
                        len -= RDT_LEN;
                        nwrite(fd, (void *)cptr->rcvpkt+RDT_LEN, len);
                        ++base;
                        while (rcvhead) {
                                rcvlist = rcvhead;
                                if (rcvlist->rcvseq != base)
                                        break;
                                nwrite(fd, (void *)rcvlist->rcvbuf+RDT_LEN, rcvlist->rcvlen);
                                ++base;
                                /* pop the header */
                                rcvhead = rcvlist->rcvnext;
                                free(rcvlist->rcvbuf);
                                free(rcvlist);
                        }
                } else { /* insert to the list */
                        rcvlist = malloc(sizeof(struct rcvlist));
                        rcvlist->rcvbuf = malloc(cptr->mss);
                        memcpy(rcvlist->rcvbuf, cptr->rcvpkt, len);
                        rcvlist->rcvseq = ack;
                        rcvlist->rcvlen = len - RDT_LEN;
                        rcvlist->rcvnext = NULL;
                        for (plist = rcvhead; plist; plist = plist->rcvnext) {
                                if (rcvlist->rcvseq > plist->rcvseq)
                                        break;
                        }
                        if (plist == NULL) { /* header */
                                rcvlist->rcvnext = rcvhead;
                                rcvhead = rcvlist;
                        } else {
                                rcvlist->rcvnext = plist->rcvnext;
                                plist->rcvnext = rcvlist;
                        }
                }

		fprintf(stderr, "rdt_recv(): base = %d\n", base);
		pkt_debug((struct rdthdr *)(cptr->rcvpkt + IP_LEN));

		cptr->ack = base;;

		if (ret == 0) {
			close(readin);	/* notify send process do rdt_fin() */
			break;
		}
	}


}

static ssize_t 
nwrite(int fd, const void *buf, size_t nbyte)
{
        int len, ret;
        len = nbyte;
	while (len != 0 && (ret = write(fd, buf, len)) != 0) {
                if (ret == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                                continue;
                        else
                                err_sys("nwrite() %d error", ret);
                }
                buf += ret;
                len -= ret;
        }
        return nbyte;
}
