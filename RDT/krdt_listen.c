#include "tcpi.h"
#include "rdt.h"

/* listen() for a connection. If RDT_REQ has already arrived,
 * and reply a RDT_ACK, a passive connection will be established.
 * The variables will be initialized:
 * src          source address
 * scid         source conn id
 * sndbuf       send buff of user data
 * sndlen       send buff len of user data
 * rcvbuf       recv buff of RDT data
 * rcvlen       recv buff len of RDT header plus user data
 * timer        unuse
 * cstate       CLOSED->LISTEN
 */

int krdt_listen(struct in_addr src, int scid)
{
        int i;
        struct conn *cptr;

        for (i = 0; i < MAX_CONN; i++)
                if (conn[i].pid == 0 && conn[i].cstate == CLOSED)
                        break;
        if (i >= MAX_CONN)
                err_quit("Can't start listen() because the connection is full.");

        if (!mtu) {
                if (dev[0] == 0 && !get_dev(src, dev))
                        err_quit("can't get dev name");
                mtu = get_mtu(dev);
        }

        cptr = &conn[i];
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
        memcpy(&cptr->src, &src, sizeof(src));
        cptr->scid = scid;
        cptr->timer = 0; /* timer is not used */
        cptr->cstate = LISTEN;
        return (i);
}
