#include "tcpi.h"
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

void prmac(void)
{
#ifndef HAVE_SYSCTL
    err_msg("Sorry, this system not support sysctl()");
    return;
#endif
    int i, mib[6];
    size_t needed;
    char *buf, *next, *lim;
    u_char *ptr;

    struct rt_msghdr *rtm;
    struct sockaddr_inarp *sin;
    struct sockaddr_dl *sdl;

    /* Return ARP cache */
    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_INET;
    mib[4] = NET_RT_FLAGS;  
    mib[5] = RTF_LLINFO;

    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
        err_ret("sysctl error");
        return;
    }

    buf = xmalloc(needed);

    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
        free(buf);
        err_ret("sysctl error");
        return;
    }

    lim = buf + needed;

    for (next = buf; next < lim; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr *)next;
        sin = (struct sockaddr_inarp *)(rtm + 1);
        printf("(%s) at", inet_ntoa(sin->sin_addr));

#ifdef HAVE_SOCKADDR_DL_STRUCT
        sdl = (struct sockaddr_dl *)(sin + 1);

        if ((i = sdl->sdl_alen) > 0) {
            ptr = (u_char *)LLADDR(sdl);
            do {
                printf("%s%x", (i == sdl->sdl_alen) ? " " : ":", *ptr++);
            } while(--i > 0);
            printf("\n");
        }
#else
        err_msg("not have struct sockaddr_dl");
        return;
#endif
    }
}
