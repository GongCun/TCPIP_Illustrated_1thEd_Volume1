#include "rdt.h"

void conn_debug(struct conn *cptr)
{
        fprintf(stderr, "pfd = %d, cstate = %d, ", cptr->pfd, cptr->cstate);
        fprintf(stderr, "src = %s, ", inet_ntoa(cptr->src));
        fprintf(stderr, "dst = %s, ", inet_ntoa(cptr->dst));
        fprintf(stderr, "scid = %d, dcid = %d\n", cptr->scid, cptr->dcid);
        fflush(stderr);
        return;
}
