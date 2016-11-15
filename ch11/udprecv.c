#include "tcpi.h"

#define OPTSTR "f:vBb:"

static ssize_t recvdst(int sockfd, char *buf, size_t buflen, int *flags, struct sockaddr *sa, socklen_t *socklen, struct in_addr *dst);

static void usage(const char *s)
{
        err_quit("Usage: %s -v -B -f ForeignIP.Port -b LocalIP <Port>", s);
}

int
main(int argc, char **argv)
{
	int sockfd;
        char buf[MAXLEN+1];
        int n = 1;
        int flags;
        size_t len = sizeof(n);
        socklen_t socklen;
	struct sockaddr_in servaddr, cliaddr;
        struct in_addr dstaddr;
        const int on = 1;
        int c;
        char *ptr;

        int servport;
        int foreignport;
        char foreignip[32];
        static int verbose = 0;
        static int bindif = 0;
        char bindip[32];

        if (argc < 2)
                usage(basename(argv[0]));

        bzero(foreignip, sizeof(foreignip));
        bzero(bindip, sizeof(bindip));

        opterr = 0;
        optind = 1;
        while ((c = getopt(argc, argv, OPTSTR)) != -1)
                switch (c) {
                        case 'f':
                                if ((ptr = strrchr(optarg, '.')) == NULL)
                                        err_quit("Invalid -f option");
                                *ptr++ = 0;
                                foreignport = atoi(ptr);
                                strcpy(foreignip, optarg);
                                break;
                        case 'v':
                                verbose = 1;
                                break;
                        case 'B':
                                bindif = 1;
                                break;
                        case 'b':
                                strcpy(bindip, optarg);
                                break;
                        case '?':
                                err_quit("Unrecognized option");
                }

        if (optind != argc-1)
                usage(basename(argv[0]));

        argc -= optind;
        argv += optind;

	servport = atoi(argv[0]);

        if (verbose && foreignip[0] != 0)
                printf("Foreign IP: %s, Port: %d\n", foreignip, foreignport);

        if (bindif) {
		pid_t pid;

#if defined(HAVE_GETIFADDRS) && defined(HAVE_IFADDRS_STRUCT)
                struct ifaddrs *ifap, *ifa;
                struct sockaddr_in *sa;
                struct ifreq ifr;

                if (getifaddrs(&ifap) < 0)
                        err_sys("getifaddrs error");
                for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
                        if (ifa->ifa_addr->sa_family != AF_INET || (ifa->ifa_flags & IFF_UP) == 0)
                                continue;
			if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
				err_sys("socket error");
                        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                                err_sys("setsockopt SO_REUSEADDR error");
#ifdef IP_RECVDSTADDR
                        if (setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)) < 0)
                                err_sys("setsockopt IP_RECVDSTADDR error");
#elif defined IP_PKTINFO
                        if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0)
                                err_sys("setsockopt IP_PKTINFO error");
#endif
                        sa = (struct sockaddr_in *)(ifa->ifa_addr);
			sa->sin_family = AF_INET;
			sa->sin_port = htons(servport);
			if (bind(sockfd, (struct sockaddr *)sa, sizeof(*sa)) < 0)
				err_sys("bind error");
			if (verbose)
				printf("Bound IP: %s\n", inet_ntoa(sa -> sin_addr));
			if ((pid = fork()) < 0) 
				err_sys("fork() error");
			else if (pid == 0) {	/* child */
				for ( ; ; ) {
					socklen = sizeof(struct sockaddr_in);
					if ((n = recvdst(sockfd, buf, sizeof(buf), &flags, (struct sockaddr *) & cliaddr, &socklen, &dstaddr)) < 0)
						err_sys("recvfrom error");
					printf("Listen on %s ", inet_ntoa(sa->sin_addr));
					printf("Recv from %s:%d %d byte, ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), n);
					printf("destination: %s\n", inet_ntoa(dstaddr));
					if (n > 0 && write(1, buf, n) != n)
						err_sys("write error");
				}
				exit(0);
			}
			/* parent continue */
#ifdef SIOCGIFBRDADDR
			if (ifa -> ifa_flags & IFF_BROADCAST) {
				memcpy(ifr.ifr_name, ifa -> ifa_name, IFNAMSIZ);
				if (ioctl(sockfd, SIOCGIFBRDADDR, &ifr) < 0)
					err_sys("ioctl SIOCGIFGBRADDR error");
				sa = (struct sockaddr_in *) & ifr.ifr_broadaddr;
				if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
					err_sys("socket error");
				if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
					err_sys("setsockopt error");
#ifdef IP_RECVDSTADDR
                                if (setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)) < 0)
                                        err_sys("setsockopt IP_RECVDSTADDR error");
#elif defined IP_PKTINFO
                                if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0)
                                        err_sys("setsockopt IP_PKTINFO error");
#endif
				sa->sin_family = AF_INET;
				sa->sin_port = htons(servport);
				if (bind(sockfd, (struct sockaddr *)sa, sizeof(*sa)) < 0) {
					if (errno == EADDRINUSE) {
                                                err_msg("Can't bind: address %s in use", inet_ntoa(sa->sin_addr));
						if (close(sockfd) < 0)
							err_sys("close() error");
						continue;
					} else
						err_sys("bind error");
				}
				if (verbose) printf("Bound IP: %s\n", inet_ntoa(sa -> sin_addr));
				if ((pid = fork()) < 0) 
					err_sys("fork() error");
				else if (pid == 0) {
					for (;;) {
						socklen = sizeof(struct sockaddr_in);
						if ((n = recvdst(sockfd, buf, sizeof(buf), &flags, (struct sockaddr *) & cliaddr, &socklen, &dstaddr)) < 0)
							err_sys("recvfrom error");
                                                printf("Listen on %s ", inet_ntoa(sa->sin_addr));
						printf("Recv from %s:%d %d byte, ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), n);
						printf("destination: %s\n", inet_ntoa(dstaddr));
						if (n > 0 && write(1, buf, n) != n)
							err_sys("write error");
					}
					exit(0);
				}
			}
#endif /* SIOCGIFBRDADDR */
                }
#else
		struct ifconf ifc;
		struct ifreq *ifr, ifrcopy;
		struct sockaddr_in *sa;
		int xlen, xfd, lastlen = 0;
		char *xbuf;
		if ((xfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			err_sys("socket xfd error");
		for (xlen = 100 * sizeof(struct ifreq); ;) {
			xbuf = xmalloc(xlen);
			ifc.ifc_len = xlen;
			ifc.ifc_buf = xbuf;
			if (ioctl(xfd, SIOCGIFCONF, &ifc) < 0) {
				if (!(errno == EINVAL && lastlen == 0))
					err_sys("ioctl error");
			} else {
				if (ifc.ifc_len == lastlen)
					break;
				else
					lastlen = ifc.ifc_len;
			}
			xlen += 10 * sizeof(struct ifreq);
			free(xbuf);
		}
		for (ptr = xbuf; ptr < xbuf + ifc.ifc_len; ) {
			ifr = (struct ifreq *)ptr;
#ifdef HAVE_SOCKADDR_SA_LEN
			xlen = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
#else
			switch (ifr->ifr_addr.sa_family) {
#ifdef IPV6
				case AF_INET6:
					xlen = sizeof(struct sockaddr_in6);
					break;
#endif
				case AF_INET: default:
					xlen = sizeof(struct sockaddr);
					break;
			}
#endif /* HAVE_SOCKADDR_SA_LEN */
			ptr += sizeof(ifr->ifr_name) + xlen;
			if (ifr->ifr_addr.sa_family != AF_INET)
				continue;
			ifrcopy = *ifr;
			if (ioctl(xfd, SIOCGIFFLAGS, &ifrcopy) < 0)
				err_sys("ioctl SIOCGIFFLAGS error");
			if ((ifrcopy.ifr_flags & IFF_UP) == 0)
				continue;

			if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
				err_sys("socket() error");
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
				err_sys("setsockopt SO_REUSEADDR error");
#ifdef IP_RECVDSTADDR
                        if (setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)) < 0)
                                err_sys("setsockopt IP_RECVDSTADDR error");
#elif defined IP_PKTINFO
                        if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0)
                                err_sys("setsockopt IP_PKTINFO error");
#endif
			sa = (struct sockaddr_in *)&ifr->ifr_addr;
			sa->sin_family = AF_INET;
			sa->sin_port = htons(servport);
			if (bind(sockfd, (struct sockaddr *)sa, sizeof(*sa)) < 0)
				err_sys("bind error");
			if (verbose) printf("Bound IP: %s\n", inet_ntoa(sa -> sin_addr));

			if ((pid = fork()) < 0) 
				err_sys("fork() error");
			else if (pid == 0) { /* child */
				for ( ; ; ) {
					socklen = sizeof(struct sockaddr_in);
					if ((n = recvdst(sockfd, buf, sizeof(buf), &flags, (struct sockaddr *) & cliaddr, &socklen, &dstaddr)) < 0)
						err_sys("recvfrom error"); 
                                        printf("Listen on %s ", inet_ntoa(sa->sin_addr));
					printf("Recv from %s:%d %d byte, ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), n);
					printf("destination: %s\n", inet_ntoa(dstaddr));
					if (n > 0 && write(1, buf, n) != n)
						err_sys("write error");
				}
				exit(0);
			}

#ifdef SIOCGIFBRDADDR
                        if (ifrcopy.ifr_flags & IFF_BROADCAST) {
                                if (ioctl(xfd, SIOCGIFBRDADDR, &ifrcopy) < 0)
                                        err_sys("ioctl SIOCGIFGBRADDR error");
				if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
					err_sys("socket() error");
				if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
					err_sys("setsockopt SO_REUSEADDR error");
#ifdef IP_RECVDSTADDR
                                if (setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)) < 0)
                                        err_sys("setsockopt IP_RECVDSTADDR error");
#elif defined IP_PKTINFO
                                if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0)
                                        err_sys("setsockopt IP_PKTINFO error");
#endif
                                sa = (struct sockaddr_in *)&ifrcopy.ifr_broadaddr;
				sa->sin_family = AF_INET;
				sa->sin_port = htons(servport);
				if (bind(sockfd, (struct sockaddr *)sa, sizeof(*sa)) < 0) {
					if (errno == EADDRINUSE) {
						if (close(sockfd) < 0)
							err_sys("close() error");
						continue;
					} else
						err_sys("bind() error");
				}
				if (verbose) printf("Bound IP: %s\n", inet_ntoa(sa -> sin_addr));
				if ((pid = fork()) < 0)
					err_sys("fork() error");
				else if (pid == 0) { /* child */
					for (;;) {
						socklen = sizeof(struct sockaddr_in);
						if ((n = recvdst(sockfd, buf, sizeof(buf), &flags, (struct sockaddr *) & cliaddr, &socklen, &dstaddr)) < 0)
							err_sys("recvfrom error");
                                                printf("Listen on %s ", inet_ntoa(sa->sin_addr));
						printf("Recv from %s:%d %d byte, ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), n);
						printf("destination: %s\n", inet_ntoa(dstaddr));
						if (n > 0 && write(1, buf, n) != n)
							err_sys("write error");
					}
					exit(0);
				}
			}
#endif /* SIOCGIFBRDADDR */
		}
#endif /* HAVE_GETIFADDRS */
        }

        /* bind wildcard or explicit address */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err_sys("socket() error");

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		err_sys("setsockopt error");

	for (n = 1; n <= MAXLEN; n += 128) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, len) < 0) {
			if (errno == ENOBUFS)
				break;
			err_sys("setsockopt SO_RCVBUF error");
		}
	}
	if (verbose)
		printf("RCVBUF = %d\n", n - 128);


#ifdef IP_RECVDSTADDR
        if (setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)) < 0)
                err_sys("setsockopt IP_RECVDSTADDR error");
#elif defined IP_PKTINFO
        if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0)
                err_sys("setsockopt IP_PKTINFO error");
#endif

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	if (bindip[0] == 0)
                servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        else {
                if (inet_aton(bindip, &servaddr.sin_addr) == 0)
                           err_quit("Invalid IP address: %s", bindip);
        }
	servaddr.sin_port        = htons(servport);

	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
                if (errno == EADDRINUSE && bindif) {
                        err_msg("Can't bind: address %s in use", inet_ntoa(servaddr.sin_addr));
                        goto END;
                }
                else
                        err_sys("bind error");
        }

        if (foreignip[0] != 0) {
                bzero(&cliaddr, sizeof(cliaddr));
                cliaddr.sin_family = AF_INET;
                if (inet_aton(foreignip, &cliaddr.sin_addr) == 0)
                        err_quit("Invalid IP address: %s", foreignip);
                cliaddr.sin_port = htons(foreignport);

                if (connect(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
                        err_sys("connect() error");
        }

        for (;;) {
                socklen = sizeof(struct sockaddr_in);
                if ((n = recvdst(sockfd, buf, sizeof(buf), &flags, (struct sockaddr *)&cliaddr, &socklen, &dstaddr)) < 0) 
                        err_sys("recvfrom error");
                printf("Listen on *.* ");
                printf("Recv from %s:%d %d byte, ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), n);
                printf("destination: %s\n", inet_ntoa(dstaddr));
                if (n > 0 && write(1, buf, n) != n)
                        err_sys("write error");
        }
END:
        while (waitpid(-1, NULL, 0) > 0)
                ;
        exit(0);
}

ssize_t recvdst(int sockfd, char *buf, size_t buflen, int *flags, struct sockaddr *sa, socklen_t *socklen, struct in_addr *dst)
{
        struct msghdr msg;
        struct iovec iov[1];
        ssize_t n;

#ifdef HAVE_MSGHDR_MSG_CONTROL
        struct cmsghdr *cmptr;
#ifdef HAVE_IN_PKTINFO_STRUCT
        char ctrldata[CMSG_SPACE(sizeof(struct in_pktinfo))];
        struct in_pktinfo *pkt;
#else
        char ctrldata[CMSG_SPACE(sizeof(struct in_addr))];
#endif
        msg.msg_control = ctrldata;
        msg.msg_controllen = sizeof(ctrldata);
        msg.msg_flags = 0;
#else
        bzero(&msg, sizeof(msg));
#endif
        msg.msg_name = sa;
        msg.msg_namelen = *socklen;
        iov[0].iov_base = buf;
        iov[0].iov_len = buflen;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        if ((n = recvmsg(sockfd, &msg, *flags)) < 0)
                return n;

        *socklen = msg.msg_namelen; /* pass back results */

        *flags = msg.msg_flags;
#ifndef HAVE_MSGHDR_MSG_CONTROL
        return n;
#else
#ifdef IP_RECVDSTADDR
        if (msg.msg_controllen < CMSG_SPACE(sizeof(struct in_addr)) ||
                        (msg.msg_flags & MSG_CTRUNC) || dst == NULL)
                return n;
        for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr)) {
                if (cmptr->cmsg_level == IPPROTO_IP &&
                                cmptr->cmsg_type == IP_RECVDSTADDR)
                        memcpy(dst, CMSG_DATA(cmptr), sizeof(struct in_addr));
        }
#elif defined(IP_PKTINFO) && defined(HAVE_IN_PKTINFO_STRUCT)
        if (msg.msg_controllen < CMSG_SPACE(sizeof(struct in_pktinfo)) ||
                        (msg.msg_flags & MSG_CTRUNC) || dst == NULL)
                return n;
        for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr)) {
                if (cmptr->cmsg_level == IPPROTO_IP &&
                                cmptr->cmsg_type == IP_PKTINFO) {
                        pkt = (struct in_pktinfo *)(CMSG_DATA(cmptr));
                        memcpy(dst, &pkt->ipi_addr, sizeof(struct in_addr));
                }
        }
#else
        bzero(dst, sizeof(struct in_addr));
#endif
        return n;
#endif /* HAVE_MSGHDR_MSG_CONTROL */
}

