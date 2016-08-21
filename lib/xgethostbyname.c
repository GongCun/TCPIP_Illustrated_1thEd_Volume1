#include "tcpi.h"

void xgethostbyname(const char *host, struct in_addr *addr)
{
        struct addrinfo hints, *res;
        int n;

        bzero(&hints, sizeof(struct addrinfo));
        hints.ai_flags = AI_CANONNAME;
        hints.ai_family = AF_INET;
        hints.ai_socktype = 0;

        if ((n = getaddrinfo(host, NULL, &hints, &res)) != 0) {
                err_quit("getaddrinfo error: %s", gai_strerror(n));
        }
        memmove(addr, &((struct sockaddr_in *)res->ai_addr)->sin_addr, sizeof(struct in_addr));
        return;
}
