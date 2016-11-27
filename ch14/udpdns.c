#include "tcpi.h"

#define DNSPORT 53

static char *strname(u_char *buf);

int main(int argc, char *argv[])
{
        int fd;
        struct sockaddr_in sa;
        u_char buf[MAXLINE];
        u_char *ptr;

        if (argc != 3)
                err_quit("udpdns <qeury_name> <dns_server>");

        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                err_sys("socket error");

        bzero(&sa, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(DNSPORT);
        if (inet_aton(argv[2], &sa.sin_addr) != 1)
                err_quit("inet_aton error");

        /* Wrap DNS Query Packet */
        ptr = buf;
        *((uint16_t *)ptr) = htons(1234);
        ptr += 2;
        *((uint16_t *)ptr) = htons(0x0100); /* RD=1 (recursion desired) */
        ptr += 2;
        *((uint16_t *)ptr) = htons(1); /* QDCOUNT */
        ptr += 2;
        *((uint16_t *)ptr) = 0; /* ANCOUNT */
        ptr += 2;
        *((uint16_t *)ptr) = 0; /* NSCOUNT */
        ptr += 2;
        *((uint16_t *)ptr) = 0; /* ARCOUNT */

        ptr += 2;
        int i, len;
        char *str[20], *p;

        str[0] = strtok(argv[1], ".");
        p = (str[0] == NULL) ? argv[1] : str[0];
        len = strlen(p);
        snprintf((char *)ptr++, 2+len, "%c%s", (u_char)len, p); 
        ptr += len;
        if (str[0] == NULL) {
                *ptr++ = '\0';
        } else {
                for (i = 1; i < 20; i++) {
                        if ((str[i] = strtok(NULL, ".")) == NULL)
                                break;
                        len = strlen(str[i]);
                        snprintf((char *)ptr++, 2+len, "%c%s", (u_char)len, str[i]);
                        ptr += len;
                }
                *ptr++ = '\0';
        }

        *((uint16_t *)ptr) = htons(1); /* query type = A */
        ptr += 2;
        *((uint16_t *)ptr) = htons(1); /* query class = 1 */
        ptr += 2;

        len = ptr - buf;

        if (sendto(fd, buf, len, 0, (struct sockaddr *)&sa, sizeof(sa)) != len)
                err_sys("sendto error");

        if ((len = recvfrom(fd, buf, MAXLINE, 0, NULL, NULL)) < 0)
                err_sys("recvfrom error");
        buf[len] = 0;
        ptr = buf;
        ptr += 2;
        if (*((uint16_t *)ptr) & 0x0200) /* truncated */
                err_msg("message truncated");

        int ancount;
        ptr += 4; /* must get the ANCOUNT */
        ancount = ntohs(*((uint16_t *)ptr));
        printf("ANCOUNT = %d\n", ancount);
        ptr += 2+4; /* pass other XXCOUNT */
       
        /* Quenstion section */
        while (*ptr++ != '\0')
                ;
        printf("QType = %d\n", ntohs(*((uint16_t *)ptr)));
        ptr += 2;
        printf("QClass = %d\n", ntohs(*((uint16_t *)ptr)));
        ptr += 2;

        /* answer section */
        char *name;
        while (ancount-- > 0) {
                if (*ptr & 0xc0) { /* DNS message compression */
                        ptr++;
                        name = strname(buf + (int)(*ptr++));
                } else { /* full domain name */
                        printf("%s\n", strname(ptr));
                        while (*ptr++ != '\0')
                                ;
                }
                ptr += 4; /* pass the type & class */
                printf("TTL = %d sec\n", ntohl(*((uint32_t *)ptr)));
                ptr += 4;

                len = ntohs(*((uint16_t *)ptr));
                ptr += 2;
                printf("Name: %s\nAddress: %s\n", name, inet_ntoa(*((struct in_addr *)ptr)));
                ptr += len;
        }

        
        return 0;
}

static char *strname(u_char *buf)
{
        char i;
        static char name[64];
        char *ptr = name;

        while ((i = *buf) != 0) {
                memcpy(ptr, ++buf, i);
                buf += i;
                ptr += i;
                *ptr++ = '.';
        }
        name[strlen(name)-1] = '\0';
        return name;
}

