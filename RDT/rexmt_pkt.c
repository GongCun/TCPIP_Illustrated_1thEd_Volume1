#include "rdt.h"
#include "rtt.h"

static void sig_alrm(int);
static sigjmp_buf jmpbuf;

ssize_t rexmt_pkt(int i)
{
        ssize_t n;
        struct conn *cptr;
        struct rtt_info *rptr;

        cptr = &conn[i];
        rptr = &rtt_info[i];

        if (signal(SIGALRM, sig_alrm) == SIG_ERR)
                err_sys("signal() of SIGALRM error");
        rtt_newpack(rptr);

rexmt:
        to_net(cptr->xfd, cptr->pkt, cptr->pktlen, cptr->dst);
        alarm(rtt_start(rptr));

        if (sigsetjmp(jmpbuf, 1) != 0) {
                if (rtt_timeout(rptr) < 0) {
                        errno = ETIMEDOUT;
                        return (-1);
                }
                rtt_debug(rptr);
                goto rexmt;
        }

        do {
                n = read(cptr->pfd, cptr->rcvbuf, cptr->rcvlen);
        } while (n < sizeof(struct rdthdr));

        alarm(0);
        rtt_stop(rptr);
        rtt_debug(rptr);
        return (n);
}


static void sig_alrm(int signo)
{
        siglongjmp(jmpbuf, 1);
}

