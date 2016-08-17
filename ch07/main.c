#include "tcpi.h"

#define NROUTES 9 /* number of record route slots */

static uint16_t id, seq;
static int buflen = 56;
int fd, optlen = 0, srroute = 0, timestamps = 0, verbose = 0;;
struct sockaddr *sockaddr;
u_char timeflags = 0; /* OF+FL */

static void usage(const char *str)
{
        err_quit("%s [-s packetsize] [-RTUv] [-g|-G|-t <host>] host", str);
}

static void send_icmp(int fd, uint16_t id, uint16_t seq, struct sockaddr *dest_addr, socklen_t dest_len)
{
        char buf[MAXLINE], data[MAXLINE];
        struct timeval *tv;
        
        if (buflen < sizeof(struct timeval))
                buflen = sizeof(struct timeval);

        memset(data, 0x80, buflen);

        tv = (struct timeval *)data;

        if (gettimeofday(tv, NULL) < 0)
                err_sys("gettimeofday error");
        
        icmp_build_echo((u_char *)buf, buflen, 8, 0, id, seq, (u_char *)data);

        if (sendto(fd, buf, 8+buflen, 0, dest_addr, dest_len) != 8+buflen)
                err_sys("sendto ICMP echo request error");
        alarm(1);

        return;
}

static void sig_alrm(int signo)
{
        send_icmp(fd, id, seq++, sockaddr, sizeof(*sockaddr));
#if defined(_AIX) || defined(_AIX64)
	if (signal(SIGALRM, sig_alrm) == SIG_ERR)
		err_sys("signal SIGALRM error");
#endif

        return;
}

static void proc_icmp(int fd, uint16_t id);

static void ipopt_init(u_char *buf, u_char type)
{
        u_char *optr = buf;

	switch(type) {
		case 0x07:
		case 0x83:
		case 0x89:
			*optr++ = 1; /* no operation */
			*optr++ = type;
			if (type == 0x07) {
				*optr++ = 3 + 4*NROUTES; /* max to 9 addresses */
			} else
				*optr++ = 3;
			*optr++ = 4; /* offset to first address */
			break;
		case 0x44:
			*optr++ = type;
			*optr++ = optlen;
			*optr++ = 5;
			*optr++ = timeflags;
			break;
		default:
			err_quit("Not support ip options code: 0x%x", type);
	}

        return;
}

static void ipopt_print(u_char *buf)
{
        u_char *ptr, ch;
        ptr = buf + 20;
        int len;
        char str[32];
        static char old_rr[MAXLINE];
        static int old_rrlen = 0;
        u_char of, fl;

        while ((ch = *ptr++) == 1)
                ; /* skip any leading NOPs */
	switch (ch) {
	case 0x07:
		len = *ptr++ - 3;
		ptr++;		/* skip over pointer */

		if (len == old_rrlen && memcmp((char *) ptr, old_rr, len) == 0) {
			printf("(same route) ");
			return;
		}
		old_rrlen = len;
		memmove(old_rr, (char *) ptr, len);
		printf("(RR:");
		while (len > 0 && *ptr != 0) {
			printf(" %s", inet_ntop(AF_INET, ptr, str, sizeof(str)));
			ptr += sizeof(struct in_addr);
			len -= sizeof(struct in_addr);
		}
		printf(") ");
		break;
	case 0x83:
	case 0x89:
		len = *ptr++ - 3;
		ptr++;
		if (ch == 0x83)
			printf("(LSRR:");
		else if (ch == 0x89)
			printf("(SSRR:");
		while (len > 0 && *ptr != 0) {
			printf(" %s", inet_ntop(AF_INET, ptr, str, sizeof(str)));
			ptr += sizeof(struct in_addr);
			len -= sizeof(struct in_addr);
		}
		printf(") ");
		break;
	case 0x44:
                len = *ptr - 4;
		ptr += 2; /* OF+FL */
		of = (*ptr >> 4);
		fl = (*ptr & 0x0f);
                ptr++; /* point to timestamp */
                printf("(");
                switch(fl) {
                        case 0:
                                printf("TSONLY:");
                                while (len > 0) {
                                        printf(" %u", ntohl(*(uint32_t *)ptr));
                                        ptr += 4;
                                        len -= 4;
                                }
                                break;
                        case 1:
                        case 3:
                                printf("%s:", (fl == 1) ? "TS+ADDR" : "TS+PRESPEC");
                                while (len > 0) {
                                        printf(" %u@%s", ntohl(*(uint32_t *)(ptr+4)), inet_ntoa(*(struct in_addr *)ptr));
                                        ptr += 8;
                                        len -= 8;
                                }
                                break;
                        default:
                                err_quit("unknown flag: %d", fl);
                }
                printf(" [%d hops not recorded]) ", (of <= 1) ? 0 : of-1);
		break;
	default:
		return;
	}

}

static int ipopt_srr_add(u_char *buf, char *addr)
        /* source and record routes */
{
        u_char *optr = buf;
        int len;
        optr += 2;
        len = *optr + 1;
        if (len >= 44)
                err_quit("too many source routes with %s", addr);
        if (inet_aton(addr, (struct in_addr *)(buf + len)) != 1)
                err_sys("inet_aton (%s) error", addr);
        *optr += sizeof(struct in_addr);

        return(*optr + 1); /* size for setsockopt() */
}

static void ipopt_time_add(u_char *buf, char *addr)
	/* routes that require timestamp */
{
	static int cnt = 0;
	u_char *optr;

	if (++cnt > 9)
		err_quit("too many routes require timestamp with %s", addr);

	optr = buf + 4 * cnt;
	if (inet_aton(addr, (struct in_addr *)optr) != 1)
		err_sys("inet_aton (%s) error", addr);

	return;
}



int main(int argc, char *argv[])
{
        int sockfd, recvfd, size, ch;
        struct hostent *hptr;
        struct sockaddr_in to;
        char **pptr;
        char *boundif = NULL;
        u_char *optspace = NULL, type = 0;


        opterr = 0; /* don't want getopt() writing to stderr */
        optind = 1;
        while ((ch = getopt(argc, argv, "s:RgGb:TUtv")) != -1)
                /* INTERNET TIMESTAMP flags: T = 0; U = 1; t = 3 */
                switch(ch) {
                        case 's':
                                buflen = atoi(optarg);
                                break;
                        case 'R':
                                if (optspace)
                                        err_quit("Can't use both -R, -g or -G");
                                optlen = 40;
                                if ((optspace = calloc(optlen, 1)) == NULL)
					err_sys("calloc error");
                                ipopt_init(optspace, 0x07); /* IPOPT_RR */
                                break;
                        case 'g':
                        case 'G':
                                srroute = 1;
                                if (optspace)
                                        err_quit("Can't use both -R, -g, -G, -T, -U or -t");
                                optlen = 44;
				if ((optspace = calloc(optlen, 1)) == NULL)
					err_sys("calloc error");
				type = (ch == 'g') ? 0x83 : 0x89;
                                ipopt_init(optspace, type); /* loose source route */
                                break;
                        case 'b':
                                boundif = optarg;
                                break;
                        case 'T':
                        case 'U':
                        case 't':
                                timestamps = 1;
				if (optspace)
					err_quit("Can't use both -R, -g, -G, -T, -U or -t");
				optlen = 36;
				type = 0x44;
                                if (ch == 'T')
                                        timeflags = 0;
                                else if (ch == 'U')
                                        timeflags = 1;
                                else
                                        timeflags = 3;
				if ((optspace = calloc(optlen, 1)) == NULL)
					err_sys("calloc error");
				ipopt_init(optspace, type);
                                break;
                        case 'v':
                                verbose = 1;
                                break;
                        default:
                                usage(basename(argv[0]));
                }


        if (optspace && (srroute == 1 || timeflags == 3)) {
                if (optind == argc - 1)
                        usage(basename(argv[0]));
		if (srroute == 1) {
			while (optind < argc - 1)
				optlen = ipopt_srr_add(optspace, argv[optind++]);
		} else if (timestamps == 1) {
			while (optind < argc - 1)
				ipopt_time_add(optspace, argv[optind++]);
		}
        }

        if (optind != argc-1)
                usage(basename(argv[0]));
#ifdef _DEBUG
        printf("final dest = %s\n", argv[optind]);
#endif

        bzero(&to, sizeof(struct sockaddr_in));
        if ((hptr = gethostbyname(argv[optind])) == NULL) {
                if (h_errno == HOST_NOT_FOUND) {
                        /* Check if a dotted-decimal string */
                        if (inet_pton(AF_INET, argv[optind], &to.sin_addr) != 1) {
                                errno = EINVAL;
                                err_sys("The address %s error", argv[optind]);
                        } else
                                printf("inet_pton(): address is %s\n", inet_ntoa(to.sin_addr));
                }
        } else {
                pptr = hptr->h_addr_list;
#ifdef _DEBUG
                printf("gethostbyname(): address is %s\n", inet_ntoa(*((struct in_addr *)(*pptr))));
#endif
                memmove(&to.sin_addr, (struct in_addr *)(*pptr), sizeof(struct in_addr));
        }

        if (optspace && srroute == 1)
                optlen = ipopt_srr_add(optspace, inet_ntoa(to.sin_addr));

	if (verbose) {
		printf("pid = %ld; ", (long)getpid());
		u_char *p;
		printf("optlen = %d\n", optlen);
		if (optspace && (srroute == 1 || timeflags == 3)) {
			p = optspace + (srroute == 1 ? 2 : 1);
			printf("len pointer value = %d\n{ ", (int)*p);
			for (p = optspace + 4; p < optspace + optlen; p += 4)
				printf("%s ", inet_ntoa(*(struct in_addr *)p));
			printf("}\n");
		}
	}
        
        to.sin_family = AF_INET;

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
                err_sys("socket sockfd error");
        if ((recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
                err_sys("socket recvfd error");
        setuid(getuid());       /* don't need special permissions any more */

        size = 60 * 1024;       /* OK if setsockopt fails */
        setsockopt(recvfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

        if (optspace) {
                if (setsockopt(sockfd, IPPROTO_IP, IP_OPTIONS, optspace, optlen) < 0)
                        err_sys("setsockopt IP_OPTIONS error");
                free(optspace);
        }

        if (boundif != NULL) {
#if defined(_AIX) || defined(_AIX64)
                printf("Don't support bind interface on this platform\n");
#elif defined(_DARWIN)
                unsigned int ifscope;
                if ((ifscope = if_nametoindex(boundif)) == 0)
                        err_sys("bad interface name (%s)", boundif);

                if (verbose)
                        printf("bind index = %d\n", ifscope);

                if (setsockopt(sockfd, IPPROTO_IP, IP_BOUND_IF, (char *)&ifscope, sizeof(ifscope)) < 0)
                        err_sys("setsockopt IP_BOUND_IF error");
#elif defined(_LINUX)
                if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, boundif, strlen(boundif)) < 0)
                        err_sys("setsockopt SO_BINDTODEVICE error");
#endif
        }

        id = getpid() & 0xffff;
        seq = 0;
        fd = sockfd;
        sockaddr = (struct sockaddr *)&to;

        if (signal(SIGALRM, sig_alrm) == SIG_ERR)
                err_sys("signal SIGALRM error");

        if (kill(getpid(), SIGALRM) < 0)
                err_sys("kill error");

        printf("PING %s (%s): %d data bytes\n", argv[argc-1], inet_ntoa(to.sin_addr), buflen);
        for (;;) {
                proc_icmp(recvfd, id);
        }

        return 0;
}

static void proc_icmp(int recvfd, uint16_t id)
{
        char buf[MAXLINE];
        struct ip *ip;
        struct icmp *icmp;
        u_char *ptr;
        int len, n;
        struct sockaddr_in from;
        struct timeval *tv, now;
        float delta;

        len = sizeof(struct sockaddr_in);
        n = icmp_recv(recvfd, (u_char *) buf, sizeof(buf), (struct sockaddr *)&from, (socklen_t *) & len, &ptr);

        icmp = (struct icmp *)ptr;
        if (icmp->icmp_id != htons(id) || icmp->icmp_type != 0 || n < 8+sizeof(struct timeval))
                return;
        ip = (struct ip *)buf;
        ipopt_print((u_char *)buf);
        tv = (struct timeval *)(ptr + 8);
        if (gettimeofday(&now, NULL) < 0)
                err_sys("gettimeofday error");
        delta = (now.tv_sec*1000.0 + now.tv_usec/1000.0) - (tv->tv_sec*1000.0 + tv->tv_usec/1000.0);

        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                        n, inet_ntoa(from.sin_addr), ntohs(icmp->icmp_seq), ip->ip_ttl, delta);
        return;
}




