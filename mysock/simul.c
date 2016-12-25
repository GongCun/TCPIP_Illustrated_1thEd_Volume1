#include "mysock.h"

static void sig_chld(int signo)
{
        pid_t pid;
        while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
                fprintf(stderr, "pid %d exited\n", (int)pid);
        exit(0);
}

int main(int argc, char *argv[])
{
        int sockfd;
        int n;
        const int on = 1;
        struct sockaddr_in sarecv;
        struct sockaddr_in safrom;
        socklen_t salen;
        char line[MAXLINE];
        char *file;
        pid_t pid = 0;

        if (argc != 5)
                err_quit("Usage: %s <interface> <multicast addr> <#port> <cmd_file>", basename(argv[0]));

        if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                err_sys("socket error");

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                err_sys("setsockopt error");
#ifdef SO_REUSEPORT
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0)
                err_sys("setsockopt error");
#endif
        file = argv[4];

        bzero(&sarecv, sizeof(struct sockaddr_in));
        sarecv.sin_family = AF_INET;
        sarecv.sin_port = htons(atoi(argv[3]));
        if (inet_pton(AF_INET, argv[2], &sarecv.sin_addr) != 1) {
                errno = EINVAL;
                err_sys("inet_pton error");
        }
        if (bind(sockfd, (struct sockaddr *)&sarecv, sizeof(struct sockaddr_in)) < 0)
                err_sys("bind error");

        if (mcast_join(sockfd, argv[1], argv[2]) < 0)
                err_sys("mcast_join error");

        if (signal(SIGTERM, SIG_IGN) == SIG_ERR)
                err_sys("signal() error");
        if (signal(SIGCHLD, sig_chld) == SIG_ERR)
                err_sys("signal() error");

        for (;;) {
                salen = sizeof(struct sockaddr_in);
                n = recvfrom(sockfd, line, MAXLINE, 0, (struct sockaddr *)&safrom, &salen);
                if (n <= 0)
                        continue;
                line[n-1] = 0;
                if (getenv("VERBOSE"))
                        fprintf(stderr, "message from %s: %s\n", inet_ntoa(safrom.sin_addr), line);
                if (strcmp(line, "start") == 0) {
                        if ((pid = fork()) < 0)
                                err_sys("fork() error");
                        else if (pid == 0) {
                                /* SIG_IGN will be inherited,
                                 * so reset to default.
                                 */
                                if (signal(SIGTERM, SIG_DFL) == SIG_ERR)
                                        err_sys("signal() error");
                                execlp(file, file, NULL);
                                err_sys("execlp() error");
                        }
                } else if (strcmp(line, "end") == 0) {
                        if (pid && kill(pid, 0) == 0 && kill(0, 15) != 0)
                                err_sys("kill process %d error", (int)pid);
                        break;
                } else
                        err_msg("unknown message: %s", line);
        }

        if (wait(NULL) < 0 && errno != ECHILD)
                err_sys("wait() error");

        return 0;
}
