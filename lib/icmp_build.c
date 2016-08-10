#include "tcpi.h"

void icmp_build_mask(u_char *buf, int len, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, uint32_t mask)
{
    struct icmp *icmp;

    icmp = (struct icmp *)buf;
    memset(icmp, 0, sizeof(struct icmp));

    icmp->icmp_type = type;
    icmp->icmp_code = code;
    icmp->icmp_id = id;
    icmp->icmp_seq = seq;
    memmove(icmp->icmp_data, &mask, len);

    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = checksum((u_short *)icmp, 8+len);

    return;
}
