#include "tcpi.h"
#include "rdt.h"

ssize_t to_net(int fd, const void *buf, size_t nbyte, struct sockaddr *dst, socklen_t len)
{
        return(sendto(fd, buf, nbyte, 0, dst, len));
}
