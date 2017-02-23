#include "tcpi.h"
#include "rdt.h"

int main(void)
{
        char buf[MAXLINE];
        int fd, n;

        if (mkfifo(RDT_FIFO, 0666) < 0 && errno != EEXIST)
                err_sys("can't create %s", RDT_FIFO);
        if ((fd = open(RDT_FIFO, O_WRONLY, 0)) < 0)
                err_sys("open() %s error", RDT_FIFO);

        while ((n = read(0, buf, MAXLINE)) > 0)
                if (write(fd, buf, n) != n)
                        err_sys("write() error");
}


