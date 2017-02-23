#include "tcpi.h"
#include "rdt.h"

int main(void)
{
        char buf[MAXLINE];
        int fd, n;
        struct sockaddr_un un;

        if ((fd = ux_cli(RDT_UX_SOCK, &un)) < 0)
                err_sys("ux_conn() error");
        while ((n = read(0, buf, MAXLINE)) > 0)
                if (sendto(fd, buf, n, 0, (struct sockaddr *)&un, sizeof(un)) != n)
                        err_sys("write() error");
}


