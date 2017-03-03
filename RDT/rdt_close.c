#include "rdt.h"

/* Active close */
ssize_t rdt_close(void)
{
	int n;
	struct conn_user *cptr;

	cptr = &conn_user;

	/* send RDT_FIN and recv RDT_ACK */
	if ((n = rexmt_pkt(cptr, RDT_FIN, NULL, 0)) < 0)
		err_sys("rexmt_pkt() error");
	close(cptr->sfd);
	close(cptr->sndfd);
	close(cptr->rcvfd);
        bzero(cptr, sizeof(conn_user));

	/* Return sent user data length (should be 0) */
	return(n - IP_LEN - RDT_LEN);

}


