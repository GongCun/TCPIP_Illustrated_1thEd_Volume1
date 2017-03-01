#include "rdt.h"

char dev[IFNAMSIZ];
int mtu;

int main(int argc, char *argv[])
{
        int n;
        int scid;
	int buflen;
        struct in_addr src;
        char *buf;

        if (argc != 3)
                err_quit("usage: %s <IPaddress> <#CID>", basename(argv[0]));
        if (inet_aton(argv[1], &src) != 1) {
                errno = EINVAL;
                err_sys("inet_aton() error");
        }

        scid = atoi(argv[2]);
        rdt_listen(src, scid);
        sleep(1);

	buflen = conn_user.mss - IP_LEN - RDT_LEN;
	fprintf(stderr, "buflen = %d\n", buflen);
	if ((buf = malloc(buflen)) == NULL)
		err_sys("malloc() error");

	while ((n = rdt_recv(buf, buflen)) > 0)
		write(1, buf, n);
        pause();

        return(0);
}

