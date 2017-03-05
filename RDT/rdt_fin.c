#include "rdt.h"

void rdt_fin(void)
{
	int n;
	struct conn_user *cptr;
        struct rdthdr *rptr;
	cptr = conn_user;

        cptr->fin = 1;

        /* Send Fin and wait Ack */
	if ((n = rexmt_pkt(cptr, RDT_FIN, NULL, 0)) < 0)
		err_sys("rexmt_pkt() error");

        /* Notify RDT process to cleanup */
        /* ... */

}


