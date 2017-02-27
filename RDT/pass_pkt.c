#include "rdt.h"

ssize_t pass_pkt(int fd, struct conn_addr *captr, u_char *buf, ssize_t buflen)
{
        ssize_t n;
        struct iovec iov[2];
        iov[0].iov_base = captr;
        iov[0].iov_len = sizeof(struct conn_addr);
        iov[1].iov_base = buf;
        iov[1].iov_len = buflen;

        if ((n = writev(fd, iov, 2)) != (sizeof(struct conn_addr) + buflen))
                err_sys("writev() error");
        return(n);
}
