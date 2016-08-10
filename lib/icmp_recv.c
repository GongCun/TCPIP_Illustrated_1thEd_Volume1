#include "tcpi.h"

ssize_t 
icmp_recv(int fd, u_char * buf, size_t buflen, struct sockaddr *address, socklen_t * address_len, u_char ** ptr)
{
	int len, iplen;
	const struct ip *ip;

	len = recvfrom(fd, buf, buflen, 0, address, address_len);
	if (len < 0)
		return (len);

	ip = (struct ip *)buf;
	iplen = ip->ip_hl * 4;
	if (ip->ip_p != IPPROTO_ICMP)
		return (0);

	*ptr = buf + iplen;

	return (len - iplen);
}
