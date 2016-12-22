#include "mysock.h"

void buffers(int sockfd)
{
	int n, optlen;

	if (rbuf == NULL && (rbuf = malloc(readlen)) == NULL)
		err_sys("malloc() error for read buffer");
	if (wbuf == NULL && (wbuf = malloc(writelen)) == NULL)
		err_sys("malloc() error for write buffer");

	/* Set the socket send and receive buffer sizes (if specified).
 	 * The receive buffer size is tied to TCP's advertised window. */
	if (rcvbuflen) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuflen, sizeof(rcvbuflen)) < 0)
			err_sys("setsockopt() of SO_RCVBUF error");
		optlen = sizeof(n);
		if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, (socklen_t *)&optlen) < 0)
			err_sys("getsockopt() of SO_RCVBUF error");
		if (n != rcvbuflen)
			err_quit("rcvbuflen = %d, SO_RCVBUF = %d", rcvbuflen, n);
		if (verbose)
			fprintf(stderr, "SO_RCVBUF = %d\n", n);
	}

	if (sndbuflen) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuflen, sizeof(sndbuflen)) < 0)
			err_sys("setsockopt() of SO_SNDBUF error");
		optlen = sizeof(n);
		if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &n, (socklen_t *)&optlen) < 0)
			err_sys("getsockopt() of SO_SNDBUF error");
		if (n != sndbuflen)
			err_quit("sndbuflen = %d, SO_SNDBUF = %d", sndbuflen, n);
		if (verbose)
			fprintf(stderr, "SO_SNDBUF = %d\n", n);
	}
}
