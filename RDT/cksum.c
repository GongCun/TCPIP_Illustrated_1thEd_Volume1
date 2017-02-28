#include "rdt.h"

u_short 
cksum(const u_char *ptr, int len)
{
	long sum = 0;		/* assume 32 bit long, 16 bit short */
        u_short *w;

        int i;
        const u_char *p;

        fprintf(stderr, "cksum() len = %d\n", len);
        for (i = 0, p = ptr; i < len; i++)
                fprintf(stderr, "0x%02x ", *(p+i));
        putc('\n', stderr);

        w = (u_short *)ptr;
	while (len > 1) {
		sum += *w++;
		if (sum & 0x80000000)	/* if high order bit set, fold */
			sum = (sum & 0xFFFF) + (sum >> 16);
		len -= 2;
	}

	if (len)		/* take care of left over byte */
		sum += (u_short)*(u_char *)w;

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return ~sum;
}
