#include "tcpi.h"
#include "rdt.h"

static void sig_io(int signo);
static int fd;
static pid_t Make_child(void);

int
main(void)
{
	const int on = 1;

	fd = ux_serv(RDT_UX_SOCK);
	if (signal(SIGIO, sig_io) == SIG_ERR)
		err_sys("signal() of SIGIO error");
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
		while ((n = recvfrom(fd, buf, MAXLINE, 0, NULL, NULL)) > 0)
			if (write(1, buf, n) != n)
				err_sys("write() error");
		exit(0);
	}
	return (pid);
}
