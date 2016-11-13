#include "tcpi.h"

#define OPTSTR "f:v"

static ssize_t recvdst(int sockfd, char *buf, size_t buflen, int *flags, struct sockaddr *sa, socklen_t *socklen, struct in_addr *dst);

static void usage(const char *s)
{
        err_quit("Usage: %s -f ForeignIP.Port <Port>", s);
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

        int foreignport;
        char foreignip[32];
        static int verbose = 0;


        if (argc < 2)
                usage(basename(argv[0]));

        bzero(foreignip, sizeof(foreignip));

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
                        case '?':
                                err_quit("Unrecognized option");
                }

        if (optind != argc-1)
                usage(basename(argv[0]));

        argc -= optind;
        argv += optind;

        if (verbose && foreignip[0] != 0)
                printf("Foreign IP: %s, Port: %d\n", foreignip, foreignport);


	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                err_sys("socket error");

        for (n += 128; n <= MAXLEN; n += 128) {
                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, len) < 0) {
                        if (errno == ENOBUFS)
                                break;
                        err_sys("setsockopt SO_RCVBUF error");
                }
        }
        if (verbose) printf("RCVBUF = %d\n", n - 128);

#ifdef IP_RECVDSTADDR
        if (setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)) < 0)
                err_sys("setsockopt IP_RECVDSTADDR error");
#elif defined IP_PKTINFO
        if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0)
                err_sys("setsockopt IP_PKTINFO error");
#endif

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
                err_sys("bind error");

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
                printf("Recv from %s:%d %d byte, ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), n);
                printf("destination: %s\n", inet_ntoa(dstaddr));
                if (n > 0 && write(1, buf, n) != n)
                        err_sys("write error");
        }

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

