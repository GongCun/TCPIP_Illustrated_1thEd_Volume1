#include "tcpi.h"

#define DNSPORT 53

static char *strname(u_char *buf, int);
int tcp = 0, udp = 0;

int main(int argc, char *argv[])
{
        int fd;
        struct sockaddr_in sa;
        u_char buf[MAXLINE];
        u_char *ptr;

        if (argc != 4)
                err_quit("udpdns <qeury_name> <dns_server> <udp|tcp>");

        if (!strcmp(argv[3], "udp"))
                udp = 1;
        else if (!strcmp(argv[3], "tcp"))
                tcp = 1;
        else
                err_quit("udpdns <qeury_name> <dns_server> <udp|tcp>");

        if ((fd = socket(AF_INET, (udp ? SOCK_DGRAM : SOCK_STREAM), 0)) < 0)
                err_sys("socket error");

        bzero(&sa, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(DNSPORT);
        if (inet_aton(argv[2], &sa.sin_addr) != 1)
                err_quit("inet_aton error");
        if (tcp && connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
                err_sys("connect error");

        /* Wrap DNS Query Packet */
        ptr = udp ? buf : buf + 2; /* TCP is prefixed with a _two_ byte length */
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

        if (tcp)
                *((uint16_t *)buf) = ntohs(ptr - buf - 2);

        len = ptr - buf;

        /* sendto & recvfrom can be used with TCP */
        if (sendto(fd, buf, len, 0, (struct sockaddr *)&sa, sizeof(sa)) != len)
                err_sys("sendto error");

        if ((len = recvfrom(fd, buf, MAXLINE, 0, NULL, NULL)) < 0)
                err_sys("recvfrom error");
        buf[len] = 0;
        ptr = udp ? buf : buf + 2;
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
	int type;
        while (ancount-- > 0) {
                if (*ptr & 0xc0) { /* DNS message compression */
			/* The 14 bits that follow in the pointer specify an offset in the DNS message */
                        name = strname(udp ? buf : buf+2, ntohs(*((uint16_t *)ptr)) & 0x03ff);
                        ptr += 2;
                } else { /* full domain name */
                        printf("%s\n", strname(ptr, 0));
                        while (*ptr++ != '\0')
                                ;
                }
		type = ntohs(*((uint16_t *)ptr));
                ptr += 4; /* pass the type & class */
                printf("TTL = %d sec\n", ntohl(*((uint32_t *)ptr)));
                ptr += 4;

                len = ntohs(*((uint16_t *)ptr));
                ptr += 2;
                if (type == 1)
			printf("Name: %s\nAddress: %s\n", name, inet_ntoa(*((struct in_addr *)ptr)));
		else
			printf("Name: %s\nType: %d\n", name, type);
                ptr += len;
        }

        
        return 0;
}

static char *strname(u_char *buf, int offset)
{
        char i;
        static char name[64];
	static int n = 0;
        char *ptr = name + n, *p = (char *)(buf + offset);

        while ((i = *p) != 0) {
		if (*p & 0xc0) {
			return strname(buf, ntohs(*((uint16_t *)p)) & 0x03ff);
		}
                memcpy(ptr, ++p, i);
                p += i;
                ptr += i;
                *ptr++ = '.';
		n += i+1;
        }
        name[strlen(name)-1] = '\0';
	n = 0;
        return name;
}

