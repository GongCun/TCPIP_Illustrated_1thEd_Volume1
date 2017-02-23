#include "rdt.h"
#include "rtt.h"
#include <sys/mman.h>

struct rtt_info *rtt_info;

void rtt_alloc(int n)
{
        if (rtt_info != NULL)
                return;
#ifdef MAP_ANON
	if ((rtt_info = mmap(0, sizeof(struct rtt_info) * n, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0)) == MAP_FAILED)
        {
		err_sys("mmap() error");
        }
#else
        int fd;
        if ((fd = open("/dev/zero", O_RDWR, 0)) < 0)
                err_sys("open() /dev/zero error");
        if ((rtt_info = mmap(0, sizeof(struct rtt_info) * n, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        {
                err_sys("mmap() error");
        }
#endif
        return;
}


