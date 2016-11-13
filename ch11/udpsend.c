#include "tcpi.h"

#define OPTSTR "b:v"

static void usage(const char *s)
{
        err_quit("Usage: %s -b LocalIP.Port <IPaddress> <Port>", s);
}

int
main(int argc, char **argv)
{
	int sockfd;
	int n = 1;
        const int on = 1;
        size_t len = sizeof(n);
	struct sockaddr_in servaddr;
	char buf[MAXLEN+1];

        char localip[32];
        int localport;
        int c;
        char *ptr;
        static int verbose = 0;

        bzero(localip, sizeof(localip));

	if (argc < 3)
		usage(basename(argv[0]));

        while ((c = getopt(argc, argv, OPTSTR)) != -1)
                switch(c) {
                        case 'b':
                                if ((ptr = strrchr(optarg, '.')) == NULL)
                                        err_quit("Invalid -b option");
                                *ptr++ = 0;
                                localport = atoi(ptr);
                                strcpy(localip, optarg);
                                break;
                        case 'v':
                                verbose = 1;
                                break;
                        case '?':
                                err_quit("Unrecognized option");
                }

        if (optind != argc-2)
                usage(basename(argv[0]));

        argc -= optind;
        argv += optind;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[1]));
	if (inet_pton(AF_INET, argv[0], &servaddr.sin_addr) != 1) {
		errno = EINVAL;
		err_sys("inet_pton error");
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err_sys("socket error");
        for (n += 128; n <= MAXLEN; n += 128) {
                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &n, len) < 0) {
                        if (errno == ENOBUFS)
                                break;
                        err_sys("setsockopt SO_SNDBUF error");
                }
        }
        if (verbose) printf("SNDBUF = %d\n", n - 128);
        if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
                err_sys("setsockopt SO_BROADCAST error");

        if (localip[0] != 0) {
                if (verbose) printf("Local IP: %s, Port: %d\n", localip, localport);
                struct sockaddr_in sa;
                sa.sin_family = AF_INET;
                if (inet_aton(localip, &sa.sin_addr) == 0)
                        err_quit("Invalid IP address: %s", localip);
                sa.sin_port = htons(localport);

                if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
                        err_sys("bind() error");
        }

	while ((n = read(0, buf, sizeof(buf))) != 0) {
		if (n == -1) {
			if (errno == EINTR)
				continue;
			else
				err_sys("read error");
		}
		if (sendto(sockfd, buf, n, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) != n)
			err_sys("write error");
	}

	exit(0);
}
