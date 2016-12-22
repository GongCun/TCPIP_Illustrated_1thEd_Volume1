#include "mysock.h"

void sockopts(int sockfd, int doall)
{
        struct linger ling;
        socklen_t optlen;

        if (linger >= 0 && doall) {
                ling.l_onoff = 1;
                ling.l_linger = linger;
                if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &linger, sizeof(ling)) < 0)
                        err_sys("setsockopt() for SO_LINGER error");

                ling.l_onoff = 0;
                ling.l_linger = -1;
                optlen = sizeof(struct linger);
                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, &linger, &optlen) < 0)
                        err_sys("getsockopt() for SO_LINGER error");
                if (ling.l_onoff == 0 || ling.l_linger != linger)
                        err_quit("SO_LINGER not set (%d, %d)", ling.l_onoff, ling.l_linger);
                if (verbose)
                        fprintf(stderr, "linger %s, time = %d\n",
                                        ling.l_onoff ? "on" : "off", ling.l_linger);
        }

        return;
}

