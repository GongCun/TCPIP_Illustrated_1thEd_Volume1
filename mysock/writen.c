#include "mysock.h"

ssize_t  writen(int fd, const void *buf, size_t len)
{
	size_t ret;
	size_t n = len;

	while (len != 0 && (ret = write(fd, buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			return ret;
		}
		len -= ret;
		buf += ret;
	}
	return n;
}

