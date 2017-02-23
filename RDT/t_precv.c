#include "tcpi.h"
#include "rdt.h"

static void sig_io(int signo);
static int fd, dummyfd;
static pid_t Make_child(void);

int
main(void)
{
	const int on = 1;

        if (mkfifo(RDT_FIFO, 0666) < 0 && errno != EEXIST)
                err_sys("can't create %s", RDT_FIFO);

        if ((fd = open(RDT_FIFO, O_RDONLY, 0)) < 0)
                err_sys("open() %s error", RDT_FIFO);

        /* Never used */
        if ((dummyfd = open(RDT_FIFO, O_WRONLY, 0)) < 0)
                err_sys("open() %s error", RDT_FIFO);

	if (signal(SIGIO, sig_io) == SIG_ERR)
		err_sys("signal() of SIGIO error");

	/*
	 * fcntl() of F_SETOWN error: Operation not supported on socket
	 */
	if (fcntl(fd, F_SETOWN, getpid()) < 0)
		err_sys("fcntl() of F_SETOWN error");
	if (ioctl(fd, FIOASYNC, &on) < 0)
		err_sys("ioctl() of FIOASYNC error");
	if (ioctl(fd, FIONBIO, &on) < 0)
		err_sys("ioctl() of FIONBIO error");

	for (;;)
		pause();
}

static void
sig_io(int signo)
{
	fprintf(stderr, "caught SIGIO\n");
	Make_child();
	return;
}

pid_t
Make_child(void)
{
	pid_t pid;
	int n;
	char buf[MAXLINE];

	if ((pid = fork()) < 0) {
		err_sys("fork() error");
	} else if (pid == 0) {
		while ((n = read(fd, buf, MAXLINE)) > 0)
			if (write(1, buf, n) != n)
				err_sys("write() error");
		exit(0);
	}
	return (pid);
}
