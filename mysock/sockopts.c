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

        if (linger >= 0 && doall && !udp) {
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

	if (nodelay && doall && !udp) {
		option = 1;
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)) < 0)
			err_sys("setsockopt() for TCP_NODELAY error");
		if (option == 0)
			err_quit("TCP_NODELAY not set (%d)", option);
		if (verbose)
			fprintf(stderr, "TCP_NODELAY set\n");
	}

	if (mss && !udp) {
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, sizeof(mss)) < 0)
			err_sys("setsockopt() for TCP_MAXSEG error");
	}

	if (doall && !udp) { /* just print MSS if verbose */
		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &option, &optlen) < 0)
			err_sys("getsockopt() for TCP_MAXSEG error");
                if (verbose)
                        fprintf(stderr, "TCP_MAXSEG = %d\n", option);
	}

	if (recvdstaddr && udp) {
#ifdef IP_RECVDSTADDR
		option = 1;
		if (setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &option, sizeof(option)) < 0)
			err_sys("setsockopt() for IP_RECVDSTADDR error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &option, &optlen) < 0)
			err_sys("getsockopt() of IP_RECVDSTADDR error");
		if (option == 0)
			err_quit("IP_RECVDSTADDR not set (%d)", option);
		if (verbose)
			fprintf(stderr, "IP_RECVDSTADDR set\n");
#elif defined(IP_PKTINFO)
		option = 1;
		if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &option, sizeof(option)) < 0)
			err_sys("setsockopt() for IP_PKTINFO error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &option, &optlen) < 0)
			err_sys("getsockopt() of IP_PKTINFO error");
		if (option == 0)
			err_quit("IP_PKTINFO not set (%d)", option);
		if (verbose)
			fprintf(stderr, "IP_PKTINFO set\n");
#endif
	}

	if (broadcast && doall) {
		option = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &option, sizeof(option)) < 0)
			err_sys("SO_BROADCAST setsockopt error");
		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &option, &optlen) < 0)
			err_sys("SO_BROADCAST getsockopt error");
		if (option == 0)
			err_quit("SO_BROADCAST not set (%d)", option);
		if (verbose)
			fprintf(stderr, "SO_BROADCAST set\n");
	}

	if (dev[0] != 0 && doall) {
		int i = 0;

		while(1) {
			option = 0;
			optlen = sizeof(option);
			if (getsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &option, &optlen) < 0)
				err_sys("IP_MULTICAST_LOOP getsockopt error");
			if (option == 0) {
				if (i++ > 0)
					err_quit("IP_MULTICAST_LOOP not set (%d)", option);
				option = 1;
				if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &option, sizeof(option)) < 0)
					err_sys("IP_MULTICAST_LOOP setsockopt error");
			} else
				break;
		}
		if (verbose)
			fprintf(stderr, "IP_MULTICAST_LOOP set\n");
	}

        if (keepalive && !udp) {
                option = 1;
                if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &option, sizeof(option)) < 0)
                        err_sys("SO_KEEPALIVE setsockopt error");
                option = 0;
                optlen = sizeof(option);
                if (getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &option, &optlen) < 0)
                        err_sys("SO_KEEPALIVE getsockopt error");
                if (optlen == 0)
                        err_quit("SO_KEEPALIVE not set (%d)", option);
                if (verbose)
                        fprintf(stderr, "SO_KEEPALIVE set\n");
        }

        return;
}

