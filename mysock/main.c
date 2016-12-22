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

static void usage(const char *msg)
{
	err_msg(
"usage: sock [ options ] <host> <port> (for client)\n"
"       sock [ options ] -s [ <IPaddr> ] <port> (for server)\n"
"options: -b n bind n as client's local port number\n"
"         -V display version\n"
"         -v verbose\n"
"         -A SO_REUSEADDR option\n"
"         -d debug"
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
	while ((c = getopt(argc, argv, "AVvb:sd:L")) != EOF)
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
			case '?':
				usage("unrecognized option");
		}
	if (client) {
		if (optind != argc - 2)
			usage("missing <hostname> and/or <port>");
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

	/* loop(fd); */

	return 0;
}

