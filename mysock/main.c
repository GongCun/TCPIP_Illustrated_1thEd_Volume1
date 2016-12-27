#include "mysock.h"
#include "version.h"

char *host;
char *port;
int bindport; /* 0 or TCP or UDP port number to bind for client */
int client = 1;
int server;
int verbose;
int debug;
int reuseaddr;
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

static void usage(const char *msg)
{
	err_msg(
"usage: sock [ options ] <host> <port> (for client)\n"
"       sock [ options ] -s [ <IPaddr> ] <port> (for server)\n"
"options: -b n bind n as client's local port number\n"
"         -V display version\n"
"         -v verbose\n"
"         -A SO_REUSEADDR option\n"
"         -d debug\n"
"         -s operate as server instead of client\n"
"         -r n #bytes per read()  (default: 1024)\n"
"         -w n #bytes per write() (default: 1024)\n"
"         -R n SO_RCVBUF option\n"
"         -S n SO_SNDBUF option\n"
"         -e operate as echo server (combined with -s)\n"
"         -L n SO_LINGER option, n = linger time\n"
"         -F fork after connection accepted (TCP concurrent server)\n"
"         -T n #seconds timeout for connection or recvmsg\n"
"         -O n #seconds to pause after listen, but before first accept\n"
"         -q n #size of listen queue for TCP server (default 5)"
	);
	if (msg[0] != 0)
		err_quit("%s", msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int c, fd;

	if (argc < 2)
		usage("");

	opterr = 0;
	while ((c = getopt(argc, argv, "q:O:AVvb:sdL:r:w:R:S:eFT:")) != EOF) {
		switch (c) {
			case 'V':
				printf("Version: %s\n", VERSION);
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
                        case 'd':
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
			case '?':
				usage("unrecognized option");
		}
	}

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

	if (client)
		fd = cliopen(host, port);
	else
		fd = servopen(host, port);

	loop(stdin, fd);

	return 0;
}

