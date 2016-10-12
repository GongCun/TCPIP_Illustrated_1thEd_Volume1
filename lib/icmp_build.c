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

void icmp_build_redirect(u_char *buf, int len, uint8_t type, uint8_t code, struct in_addr gateway, u_char *data)
{
        struct icmp *icmp;

        icmp = (struct icmp *)buf;
        memset(icmp, 0, sizeof(struct icmp));

        icmp->icmp_type = type;
        icmp->icmp_code = code;
        icmp->icmp_cksum = 0;
        memmove(&icmp->icmp_gwaddr, &gateway, sizeof(struct in_addr));
        memmove(icmp->icmp_data, data, len);
        icmp->icmp_cksum = checksum((u_short *)icmp, 8+len);

        return;
}

void icmp_build_selection(u_char *buf, uint8_t type, uint8_t code)
{
        struct icmp *icmp;

        icmp = (struct icmp *)buf;
        memset(icmp, 0, sizeof(struct icmp));

        icmp->icmp_type = type;
        icmp->icmp_code = code;
        icmp->icmp_cksum = 0;
        icmp->icmp_id = 0;      /* must be 0 */
        icmp->icmp_seq = 0;     /* must be 0 */
        icmp->icmp_cksum = checksum((u_short *)icmp, 8);

        return;
}

void icmp_build_advertisment(u_char *buf, uint8_t type, uint8_t code, 
                u_char naddr, struct in_addr *addrlist)
{
        struct icmp *icmp;
        u_char i;
        u_char *ptr;
        ssize_t len = sizeof(struct in_addr);

        icmp = (struct icmp *)buf;
        memset(icmp, 0, sizeof(struct icmp));

        icmp->icmp_type = 9;
        icmp->icmp_code = 0;
        icmp->icmp_cksum = 0;
        icmp->icmp_num_addrs = naddr;
        icmp->icmp_wpa = 2;                     /* Address Entry Size: fixed at 2 */
        icmp->icmp_lifetime = htons(30*60);     /* Number of Seconds */

        ptr = (u_char *)icmp->icmp_data;
        for (i = 0; i < naddr; i++) {
                memmove(ptr, addrlist, len);    /* Router Address */
                *(uint32_t *)(ptr + len) = 0;   /* Preference Level */
                ptr += 2 * len;
                addrlist += len;
        }
	icmp->icmp_cksum = checksum((u_short *) icmp, 8 + len*2*naddr);

        return;
}
