#include "rdt.h"

ssize_t get_pkt(int fd, struct conn_addr *captr, u_char *buf, ssize_t buflen)
{
        ssize_t n;
        struct iovec iov[2];
        iov[0].iov_base = captr;
        iov[0].iov_len = sizeof(struct conn_addr);
        iov[1].iov_base = buf;
        iov[1].iov_len = buflen;

        if ((n = readv(fd, iov, 2)) < sizeof(struct conn_addr))
                err_sys("readv() error");
        return(n);
}
