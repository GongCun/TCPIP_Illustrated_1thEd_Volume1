#include "tcpi.h"

uint16_t checksum(uint16_t * addr, int len)
{
	int nleft = len;
	uint16_t *w = addr;
	uint16_t answer = 0;
	uint32_t sum = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*((unsigned char *)&answer) = *((unsigned char *)w);	/* only 1 byte */
		sum += answer;
	}
	sum = (sum >> 16) + (sum & 0xffff);	/* hi 16 + low 16 */
	sum += (sum >> 16);
	answer = ~sum;		/* truncate to 16 bit */
	return answer;
}
