#include "rdt.h"

int chk_chksum(u_short *ptr, int len)
{
        /* _NOTE_: the length must use the host byte order */
        return ((checksum(ptr, len) == (u_short)0) ? 1 : 0);
}
