#include "tcpi.h"

ssize_t
recvdst(int sockfd, char *buf, size_t buflen, int *flags, struct sockaddr *sa, socklen_t *socklen, struct in_addr *dst)
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

        if ((n = recvmsg(sockfd, &msg, 0)) < 0)
                return n;

        *socklen = msg.msg_namelen; /* pass back results */

        *flags = msg.msg_flags;
#ifndef HAVE_MSGHDR_MSG_CONTROL
        return n;
#else
#ifdef IP_RECVDSTADDR
        if (msg.msg_controllen < CMSG_SPACE(sizeof(struct in_addr)) ||
                        (msg.msg_flags & MSG_CTRUNC) || dst == NULL)
	{
                return n;
	}
        for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr)) {
                if (cmptr->cmsg_level == IPPROTO_IP &&
                                cmptr->cmsg_type == IP_RECVDSTADDR)
		{
                        memcpy(dst, CMSG_DATA(cmptr), sizeof(struct in_addr));
		}
        }
#elif defined(IP_PKTINFO) && defined(HAVE_IN_PKTINFO_STRUCT)
        if (msg.msg_controllen < CMSG_SPACE(sizeof(struct in_pktinfo)) ||
                        (msg.msg_flags & MSG_CTRUNC) || dst == NULL)
                return n;
        for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr)) {
                if (cmptr->cmsg_level == IPPROTO_IP &&
                                cmptr->cmsg_type == IP_PKTINFO)
		{
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

