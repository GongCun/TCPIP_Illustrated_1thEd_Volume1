#include "mysock.h"

char *host;
char *port;
char *addr;
int bindport; /* 0 or TCP or UDP port number to bind for client */
int client = 1;
int server;
int verbose;
int debug;
int reuseaddr;
int reuseport;
int linger = -1; /* 0 or positive turns on option */
int listenq = 5; /* listen queue for TCP server */
int readlen = 1024; /* default read length for socket */
int writelen = 1024; /* default write length for socket */
char *rbuf; /* pointer that is malloc'ed */
char *wbuf; /* pointer that is malloc'ed */
int rcvbuflen;
int sndbuflen;
int echo;
int dofork;
int timeout; /* positive turns on option,
                connection timeout for client,
                read socket timeout for server */
int pauselisten; /* seconds to sleep after listen() */
int rawopt;
int id;
unsigned int seq;
unsigned int ack;
unsigned char event;
int cbreak;
int nodelay; /* TCP_NODELAY (Nagle algorithm) */
int sourcesink; /* source/sink mode */
int nbuf = 1; /* number of buffers to write (sink mode) */
int pauserw; /* seconds to sleep before each read or write */
int pauseinit; /* seconds to sleep before first read or write */
int pauseclose; /* seconds to sleep after recv FIN, before close */
int urgwrite; /* write urgent byte after this write */
int mss; /* maximum sigment size */
int timestamp; /* display timestamp */
int udp; /* use UDP instead of TCP */
char foreignip[32];
int foreignport;
int recvdstaddr; /* IP_RECVDSTADDR option */
int broadcast; /* SO_BROADCAST */
char dev[16]; /* multicast interface */
int multicast;
struct sockaddr_in serv_addr;

static void usage(const char *msg)
{
	err_msg(
"usage: sock [ options ] <host> <port> (for client)\n"
"       sock [ options ] -s [ <IPaddr> ] <port> (for server)\n"
"options: -b n bind n as client's local port number\n"
"         -V display version\n"
"         -v verbose\n"
"         -a SO_REUSEPORT option\n"
"         -A SO_REUSEADDR option\n"
"         -D SO_DEBUG option\n"
"         -s operate as server instead of client\n"
"         -r n #bytes per read()  (default: 1024)\n"
"         -w n #bytes per write() (default: 1024)\n"
"         -R n SO_RCVBUF option\n"
"         -S n SO_SNDBUF option\n"
"         -m n TCP_MAXSEG options\n"
"         -e operate as echo server (combined with -s)\n"
"         -f a.b.c.d.p foreign IP address = a.b.c.d, foreign port# = p\n"
"         -L n SO_LINGER option, n = linger time\n"
"         -F fork after connection accepted (TCP concurrent server)\n"
"         -T n #seconds timeout for connection or recvmsg\n"
"         -O n #seconds to pause after listen, but before first accept\n"
"         -q n #size of listen queue for TCP server (default 5)\n"
"         -o \"EVENT:ADDR:SEQ:ACK:ID\" raw socket event\n"
"         -C set terminal to cbreak mode\n"
"         -N TCP_NODELAY option\n"
"         -i \"source\" client or \"sink\" server (-w/-r)\n"
"         -n n #buffers to write for \"source\" client (default 1024)\n"
"         -p n #seconds to pause before each read or write (source/sink)\n"
"         -Q n #seconds to pause after receiving FIN, but before close\n"
"         -P n #seconds to pause before first read or write (source/sink)\n"
"         -U n enter urgent mode after write number n (source only)\n"
"         -d display timestamp\n"
"         -u use UDP instead of TCP\n"
"         -E display destination IP address of a received UDP datagram\n"
"         -B SO_BROADCAST option\n"
"         -M \"Multicase Interface\", should bind Class-D IP"
	);
	if (msg[0] != 0)
		err_quit("%s", msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int c, fd;
        char *ptr;

	if (argc < 2)
		usage("");

	opterr = 0;
	while ((c = getopt(argc, argv, "M:BEf:audm:U:Q:P:p:n:iNCq:O:AVvb:sDL:r:w:R:S:eFT:o:")) != EOF) {
		switch (c) {
			case 'V':
#define P(s) (void)fputs(s "\n", stderr);
#include "version.h"
#undef P
				exit(0);

			case 'b':
				bindport = atoi(optarg);
				break;
			case 's':
				server = 1;
				client = 0;
				break;
                        case 'v':
                                verbose = 1;
                                break;
                        case 'A':
                                reuseaddr = 1;
                                break;
                        case 'a':
                                reuseport = 1;
                                break;
                        case 'D':
                                debug = 1;
                                break;
                        case 'L':
                                linger = atol(optarg);
                                break;
			case 'r':
				readlen = atoi(optarg);
				break;
			case 'w':
				writelen = atoi(optarg);
				break;
			case 'R':
				rcvbuflen = atoi(optarg);
				break;
			case 'S':
				sndbuflen = atoi(optarg);
				break;
			case 'e':
				echo = 1;
				break;
			case 'F':
				dofork = 1;
				break;
                        case 'T':
                                timeout = atoi(optarg);
                                break;
                        case 'O':
                                pauselisten = atoi(optarg);
                                break;
                        case 'q':
                                listenq = atoi(optarg);
                                break;
                        case 'o':
                                rawopt = 1;
                                if ((ptr = strrchr(optarg, ':')) == NULL)
                                        usage("unrecognized option");
                                *ptr++ = '\0';
                                id = atoi(ptr);

                                if ((ptr = strrchr(optarg, ':')) == NULL)
                                        usage("unrecognized option");
                                *ptr++ = '\0';
                                ack = strtoul(ptr, NULL, 10);

                                if ((ptr = strrchr(optarg, ':')) == NULL)
                                        usage("unrecognized option");
                                *ptr++ = '\0';
                                seq = strtoul(ptr, NULL, 10);

                                if ((ptr = strrchr(optarg, ':')) == NULL)
                                        usage("unrecognized option");
                                *ptr++ = '\0';
                                addr = ptr;

                                switch (*optarg) {
                                        case 'a': case 'A':
                                                event = TH_ACK;
                                                break;
                                        case 'p': case 'P':
                                                event = TH_PUSH;
                                                break;
                                        case 'r': case 'R':
                                                event = TH_RST;
                                                break;
                                        case 's': case 'S':
                                                event = TH_SYN;
                                                break;
                                        case 'f': case 'F':
                                                event = TH_FIN;
                                                break;
                                        case 'u': case 'U':
                                                event = TH_URG;
                                                break;
                                        default:
                                                usage("unrecognized option");
                                }
                                break;

			case 'C':
				cbreak = 1;
				break;

			case 'N':
				nodelay = 1;
				break;

                        case 'i':
                                sourcesink = 1;
                                break;

                        case 'n':
                                nbuf = atol(optarg);
                                break;

                        case 'p':
                                pauserw = atoi(optarg);
                                break;

                        case 'P':
                                pauseinit = atoi(optarg);
                                break;

                        case 'Q':
                                pauseclose = atoi(optarg);
                                break;

                        case 'U':
                                urgwrite = atoi(optarg);
                                break;

                        case 'm':
                                mss = atoi(optarg);
				break;

                        case 'd':
                                ++timestamp;
				break;

                        case 'u': /* use UDP instead of TCP */
                                ++udp;
				break;

                        case 'f':
                                if ((ptr = strrchr(optarg, '.')) == NULL)
                                        usage("invalid -f option");
                                *ptr++ = 0;
                                foreignport = atoi(ptr);
                                strcpy(foreignip, optarg);
                                break;

			case 'E':
				++recvdstaddr;
				break;

			case 'B':
				++broadcast;
				break;

			case 'M':
				strcpy(dev, optarg);
				break;

			case '?':
				usage("unrecognized option");
		}
	}

        if (udp && debug)
                usage("can't specify -D and -u");
        if (udp && linger >= 0)
                usage("can't specify -L and -u");
        if (udp && nodelay)
                usage("can't specify -N and -u");
        if (udp && rawopt)
                usage("can't specify -o and -u");
        if (udp && mss)
                usage("can't specify -m and -u");
        if (udp && urgwrite)
                usage("can't specify -U and -u");
        if (udp && pauseclose)
                usage("can't specify -Q and -u");
	if (!udp && broadcast)
		usage("can't specify -B with TCP");
	if (!udp && foreignip[0] != 0)
		usage("can't specify -f with TCP");
	if (!udp && dev[0] != 0)
		usage("can't specify -M with TCP");
	if (client && dev[0] != 0)
		usage("can't specify -M with client");

	if (client) {
		if (optind != argc - 2)
			usage("missing <hostname> and/or <port>");
		if (echo)
			usage("");
		host = argv[argc-2];
		port = argv[argc-1];
	} else {
		if (optind == argc - 2) {
			host = argv[argc-2];
			port = argv[argc-1];
		} else if (optind == argc - 1) {
			host = NULL;
			port = argv[argc-1];
		} else
			usage("missing <hostname> or <port>");
	}

	if (client) {
                if (rawopt) {
                        if (bindport == 0)
                                bindport = (getpid() & 0xffff) | 0x8000;
                        int foreignport;
                        struct servent *sp;
                        if ((foreignport = atoi(port)) == 0) {
                                if ((sp = getservbyname(port, "tcp")) == NULL)
                                        err_quit("getservbyname() error for: %s", port);
                                foreignport = ntohs(sp->s_port);
                        }

                        tcpraw(event, id, seq, ack, addr, host, bindport, foreignport);
                        exit(0);
                } else {
		        fd = cliopen(host, port);
                }
        } else
		fd = servopen(host, port);

        if (sourcesink)
                sink(fd);
        else
                loop(stdin, fd);

	return 0;
}

