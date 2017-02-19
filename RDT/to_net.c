#include "tcpi.h"
#include "rdt.h"

ssize_t to_net(int fd, const void *buf, size_t nbyte, struct in_addr dst)
{
        struct sockaddr_in addr;

        addr.sin_family = AF_INET;
        memcpy(&addr.sin_addr, &dst, sizeof(dst));
        return (sendto(fd, buf, nbyte, 0,
                (struct sockaddr *)&addr, sizeof(addr)));
}
