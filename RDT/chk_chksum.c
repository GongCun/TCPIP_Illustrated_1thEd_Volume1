#include "rdt.h"

int chk_chksum(u_short *ptr, int len)
{
        return ((checksum(ptr, len) == (u_short)0) ? 1 : 0);
}
