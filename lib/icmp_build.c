#include "tcpi.h"

void 
icmp_build_mask(u_char * buf, int len, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, uint32_t mask)
{
	struct icmp *icmp;

	icmp = (struct icmp *)buf;
	memset(icmp, 0, sizeof(struct icmp));

	icmp->icmp_type = type;
	icmp->icmp_code = code;
	icmp->icmp_id = htons(id);
	icmp->icmp_seq = htons(seq);
	memmove(icmp->icmp_data, &mask, len);

	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = checksum((u_short *) icmp, 8 + len);

	return;
}

void
icmp_build_time(u_char * buf, int len, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, uint32_t origtime, uint32_t recvtime, uint32_t xmittime)
{
        struct icmp *icmp;
        icmp = (struct icmp *)buf;
        memset(icmp, 0, sizeof(struct icmp));

        icmp->icmp_type = type;
        icmp->icmp_code = code;
        icmp->icmp_id = htons(id);
        icmp->icmp_seq = htons(seq);
        icmp->icmp_otime = htonl(origtime);
        icmp->icmp_rtime = htonl(recvtime);
        icmp->icmp_ttime = htonl(xmittime);

        icmp->icmp_cksum = 0;
        icmp->icmp_cksum = checksum((u_short *) icmp, 8 + len);

        return;
}

void icmp_build_echo(u_char * buf, int len, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, u_char *data)
{
        struct icmp *icmp;
        icmp = (struct icmp *)buf;
        memset(icmp, 0, sizeof(struct icmp));

        icmp->icmp_type = type;
        icmp->icmp_code = code;
        icmp->icmp_cksum = 0; /* calculate later */
        icmp->icmp_id = htons(id);
        icmp->icmp_seq = htons(seq);
        memmove(icmp->icmp_data, data, len);

        icmp->icmp_cksum = checksum((u_short *)icmp, 8+len);

        return;
}
