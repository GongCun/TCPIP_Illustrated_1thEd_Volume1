#include "tcpi.h"

void *xmalloc(size_t size)
{
    void *ptr;

    if ((ptr = malloc(size)) == NULL)
        err_sys("malloc error");
    return ptr;
}

int xsocket(int family, int type, int protocol)
{
    int n;

    if ((n = socket(family, type, protocol)) < 0)
        err_sys("socket error");
    return n;
}

int xioctl(int fd, unsigned long request, void *arg)
{
    int n;
    if ((n = ioctl(fd, request, arg)) == -1)
        err_sys("ioctl error");
    return n;
}

void *xcalloc(size_t count, size_t size)
{
    void *ptr;
    if ((ptr = calloc(count, size)) == NULL)
        err_sys("calloc error");
    return ptr;
}

