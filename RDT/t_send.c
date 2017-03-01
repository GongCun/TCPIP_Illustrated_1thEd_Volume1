#include "rdt.h"

char dev[IFNAMSIZ];
int mtu;

int main(int argc, char *argv[])
{
        int n, ret, buflen;
        struct in_addr dst;
        char *buf;

        if (argc != 3)
                err_quit("usage: %s <IPaddress> <#CID>", basename(argv[0]));
        if (inet_aton(argv[1], &dst) != 1) {
                errno = EINVAL;
                err_sys("inet_aton() error");
        }

        rdt_connect(dst, -1, atoi(argv[2]));
	buflen = conn_user.mss - IP_LEN - RDT_LEN;
	fprintf(stderr, "buflen = %d\n", buflen);
	if ((buf = malloc(buflen)) == NULL)
		err_sys("malloc() error");

	while ((n = read(0, buf, buflen)) > 0) {
		if ((ret = rdt_send(buf, n)) != n)
			err_quit("rdt_send() %d bytes, expect %d bytes", ret, n);
	}
        pause();

        return(0);
}