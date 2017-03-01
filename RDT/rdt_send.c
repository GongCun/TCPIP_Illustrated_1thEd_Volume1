#include "rdt.h"

/* Send data from above */
ssize_t rdt_send(void *buf, size_t nbyte)
{
	int n;
	static int seq = 0;
	struct conn_user *cptr;
	cptr = &conn_user;

	n = cptr->mss - IP_LEN - RDT_LEN;
	if (nbyte > n)
		err_quit("data %d bytes exceed the pkt mss %d bytes", nbyte, n);

	if ((n = rexmt_pkt(cptr, seq, RDT_DATA, buf, nbyte)) < 0)
		err_sys("rexmt_pkt() error");
	seq = ++seq % 2;

	/* Return sent user data length */
	return(n - IP_LEN - RDT_LEN);

}


