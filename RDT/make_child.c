#include "rdt.h"

/*
 * Passing the RX RDT protocol data (include RDT
 * header) to child process. The child process 
 * is the focus of all TX/RX activity.
 */

pid_t make_child(int i, pid_t upid)
{
        pid_t pid;
        int fd[2], connfd;
        struct conn *cptr;
        char buf[PATH_MAX];

        cptr = &conn[i];

        /* The pipefd of cptr->pfd for parent process
         * is used for writing, for child process is
         * used for reading.
         */
        if (pipe(fd) < 0)
                err_sys("pipe() error");

        if ((pid = fork()) < 0) {
		err_sys("fork() child process error");
	} else if (pid > 0) {
                close(fd[0]);
                cptr->pid = pid;
                cptr->pfd = fd[1];
                return (pid); /* parent */
        }

        /* child continue... */
        close(fd[1]);
        cptr->pid = getpid();
        cptr->pfd = fd[0];

        /* Child IPC with user by unix domain stream socket.
         * _NEED_ add wait mechanism before ux_conn().
         */
        sprintf(buf, "%s.%ld", RDT_UX_SOCK, (long)upid);
        connfd = ux_conn(buf);
        cptr->sfd = connfd;

        xmit_pkt(i);

        exit(1); /* shouldn't return */

}

