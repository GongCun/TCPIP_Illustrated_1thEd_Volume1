#include "tcpi.h"

void
dispatch_pcap(pcap_t *pt, struct bpf_program *bp, handler callback)
{
        int rt;

        if ((rt = pcap_dispatch(pt, 0, callback, NULL)) == -1)
                err_quit("pcap_dispatch: %s", pcap_dispatch);
        else if (rt == 0)
                fprintf(stderr, "pcap_dispatch captured no packets\n");

#ifdef HAVE_PCAP_FREECODE
        pcap_freecode(bp);
#endif
        pcap_close(pt);

        return;
}
