#include "rdt.h"
#include "rtt.h"
#include <sys/mman.h>

struct rtt_info *rptr;

void rtt_alloc(int n)
{
        if (rptr != NULL)
                return;
#ifdef MAP_ANON
	if ((rptr = mmap(0, sizeof(struct rtt_info) * n, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0)) == MAP_FAILED)
        {
		err_sys("mmap() error");
        }
#else
        int fd;
        if ((fd = open("/dev/zero", O_RDWR, 0)) < 0)
                err_sys("open() /dev/zero error");
        if ((rptr = mmap(0, sizeof(struct rtt_info) * n, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        {
                err_sys("mmap() error");
        }
#endif
        return;
}


