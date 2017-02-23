#include "rdt.h"

void 
pkt_debug(const struct rdthdr *rdthdr)
{
	fprintf(stderr, "%d %d %02x %02x %d\n",
		rdthdr->rdt_scid,
		rdthdr->rdt_dcid,
		rdthdr->rdt_seq,
		rdthdr->rdt_flags,
		ntohs(rdthdr->rdt_len));
	fflush(stderr);
}
