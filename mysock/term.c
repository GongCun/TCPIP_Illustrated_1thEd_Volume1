#include "mysock.h"
#include <termios.h>

static struct termios save_termios;
static int ttysavefd = -1;
static enum { TTY_RESET, TTY_CBREAK } ttystate = TTY_RESET;

int tty_cbreak(int fd)
{
	struct termios buf;
	if (tcgetattr(fd, &save_termios) < 0)
		return -1;
	buf = save_termios;
	buf.c_lflag &= ~(ECHO | ICANON); /* echo off, canonical mode off */
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
		return -1;
	ttystate = TTY_CBREAK;
	ttysavefd = fd;
	return 0;
}

int tty_reset(int fd) /* restore terminal's mode */
{
	if (ttystate == TTY_RESET)
		return 0;
	if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0)
		return -1;
	ttystate = TTY_RESET;
	return 0;
}

void tty_atexit(void) /* can be set up by atexit(tty_atexit) */
{
	if (ttysavefd >= 0)
		tty_reset(ttysavefd);
}
