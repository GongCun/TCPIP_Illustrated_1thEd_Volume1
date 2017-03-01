#include "rdt.h"
#include "rtt.h"

static void sig_alrm(int);
static sigjmp_buf jmpbuf;

ssize_t rexmt_pkt(struct conn_user *cptr)
{
        int n, ret;
        static int init = 0;

        struct rtt_info *rptr;

        rptr = &rtt_info;

        if (init++ == 0)
                rtt_init(rptr);

        if (signal(SIGALRM, sig_alrm) == SIG_ERR)
                err_sys("signal() of SIGALRM error");

        rtt_newpack(rptr);

        /* conn_user_debug(cptr); */

rexmt:
        n = make_pkt(cptr->src, cptr->dst, cptr->scid, cptr->dcid,
                        0, RDT_REQ, NULL, 0, cptr->pkt);
        if ((ret = to_net(cptr->sfd, cptr->pkt, n, cptr->dst)) < 0)
                return(ret);
        alarm(rtt_start(rptr));

        if (sigsetjmp(jmpbuf, 1) != 0) {
                if (rtt_timeout(rptr) < 0) {
                        errno = ETIMEDOUT;
                        return(-1);
                }
                rtt_debug(rptr);
                goto rexmt;
        }

        do {
                n = read(cptr->pfd, cptr->pkt, cptr->mss);
        } while (n < sizeof(struct rdthdr));

        alarm(0);
        rtt_stop(rptr);
        rtt_debug(rptr);
        return(n);
}


static void sig_alrm(int signo)
{
        siglongjmp(jmpbuf, 1);
}

