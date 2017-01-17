#include "mysock.h"

void sockopts(int sockfd, int doall)
{
        struct linger ling;
	int option;
        socklen_t optlen;

        if (debug) {
                option = 1;
                if (setsockopt(sockfd, SOL_SOCKET, SO_DEBUG, &option, sizeof(option)) < 0)
                        err_sys("setsockopt() for SO_DEBUG error");
                option = 0;
                optlen = sizeof(option);
                if (getsockopt(sockfd, SOL_SOCKET, SO_DEBUG, &option, &optlen) < 0)
                        err_sys("getsockopt() for SO_DEBUG error");
                if (option == 0)
                        err_quit("SO_DEBUG not set %d\n", option);
                if (verbose)
                        fprintf(stderr, "SO_DEBUG set\n");
        }

        if (linger >= 0 && doall) {
                ling.l_onoff = 1;
                ling.l_linger = linger;
                if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) < 0)
                        err_sys("setsockopt() for SO_LINGER error");

                ling.l_onoff = 0;
                ling.l_linger = -1;
                optlen = sizeof(struct linger);
                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, &ling, &optlen) < 0)
                        err_sys("getsockopt() for SO_LINGER error");
                if (ling.l_onoff == 0 || ling.l_linger != linger)
                        err_quit("SO_LINGER not set (%d, %d)", ling.l_onoff, ling.l_linger);
                if (verbose)
                        fprintf(stderr, "linger %s, time = %d\n",
                                        ling.l_onoff ? "on" : "off", ling.l_linger);
        }

	if (nodelay && doall) {
		option = 1;
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)) < 0)
			err_sys("setsockopt() for TCP_NODELAY error");
		if (option == 0)
			err_quit("TCP_NODELAY not set (%d)", option);
		if (verbose)
			fprintf(stderr, "TCP_NODELAY set\n");
	}

	if (mss) {
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, sizeof(mss)) < 0)
			err_sys("setsockopt() for TCP_MAXSEG error");
	}

	if (doall) { /* just print MSS if verbose */
		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &option, &optlen) < 0)
			err_sys("getsockopt() for TCP_MAXSEG error");
                if (verbose)
                        fprintf(stderr, "TCP_MAXSEG = %d\n", option);
	}

        return;
}

