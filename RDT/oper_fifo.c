#include "rdt.h"

char fifoname[PATH_MAX];

void fifoexit(void)
{
	unlink(fifoname);
}

/* User process create FIFO for read */
int make_fifo(pid_t pid, const char *str)
{
	int fd;

	if (atexit(fifoexit) < 0)
		err_ret("atexit() error");

	sprintf(fifoname, "%s%s.%ld", RDT_FIFO, str, (long)pid);
	if (mkfifo(fifoname, FILE_MODE) < 0)
		err_sys("mkfifo() %s error", fifoname);

	/*
 	 * If use O_RDONLY, will block until server opening for
	 * writing. If use O_RDONLY|O_NONBLOCK, read() will return 0
	 * immediately, so use O_RDWR.
	 */
	if ((fd = open(fifoname, O_RDWR, 0)) < 0)
		err_sys("opern() %s error", fifoname);

	return(fd);
}

/* RDT process open FIFO for write */
int open_fifo(pid_t pid, const char *str)
{
	int fd;

	if (atexit(fifoexit) < 0)
		err_ret("atexit() error");

	sprintf(fifoname, "%s%s.%ld", RDT_FIFO, str, (long)pid);

	if ((fd = open(fifoname, O_WRONLY, 0)) < 0)
		err_sys("opern() %s error", fifoname);

	return(fd);
}

