#include "rdt.h"

int chk_chksum(const u_char *ptr, int len)
{
        /* _NOTE_: the length must use the host byte order */
        u_short sum = cksum(ptr, len);
        fprintf(stderr, "cksum = %d\n", sum);
        return (sum ? 0 : 1);
        /* return (cksum(ptr, len) ? 0 : 1); */
}
