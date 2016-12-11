#include "tcpi.h"

#define DNSPORT 53

static char *strname(u_char *, u_char *);
int tcp = 0, udp = 0;

int main(int argc, char *argv[])
{
        int fd;
        char *PTR;
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

        PTR = getenv("PTR");
	if (PTR == NULL) {
		str[0] = strtok(argv[1], ".");
		p = (str[0] == NULL) ? argv[1] : str[0];
		len = strlen(p);
		snprintf((char *)ptr++, 2 + len, "%c%s", (u_char) len, p);
		ptr += len;
		if (str[0] == NULL) {
			*ptr++ = '\0';
		} else {
			for (i = 1; i < 20; i++) {
				if ((str[i] = strtok(NULL, ".")) == NULL)
					break;
				len = strlen(str[i]);
				snprintf((char *)ptr++, 2 + len, "%c%s", (u_char) len, str[i]);
				ptr += len;
			}
			*ptr++ = '\0';
		}
	} else {
		str[0] = strtok(argv[1], ".");
		if (str[0] == NULL)
			err_quit("IP format error");
		for (i = 1; i <= 3; i++)
			if ((str[i] = strtok(NULL, ".")) == NULL)
				err_quit("IP format error");
		str[4] = "in-addr";
		str[5] = "arpa";
		for (i = 3; i >= 0; i--) {
			len = strlen(str[i]);
			sprintf((char *)ptr, "%c%s", (u_char) len, str[i]);
			ptr += len + 1;
		}
		for (i = 4; i <= 5; i++) {
			len = strlen(str[i]);
			sprintf((char *)ptr, "%c%s", (u_char) len, str[i]);
			ptr += len + 1;
		}
		*ptr++ = '\0';
	}


        if (!PTR)
                *((uint16_t *)ptr) = htons(1); /* query type = A */
        else
                *((uint16_t *)ptr) = htons(12); /* query type = PTR */
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
        char *name, ch;
	int type;
        while (ancount-- > 0) {
                name = strname(udp ? buf : buf+2, ptr);
                while ((ch = *ptr++) != '\0') {
                        if (ch & 0xc0) {
                                ptr++;
                                break;
                        }
                }
		type = ntohs(*((uint16_t *)ptr));
                ptr += 4; /* pass the type & class */
                printf("TTL = %d sec\n", ntohl(*((uint32_t *)ptr)));
                ptr += 4;

                len = ntohs(*((uint16_t *)ptr));
                ptr += 2;
                printf("Name: %s\n", name);
		switch (type) {
		case 1:
			printf("Address: %s\n", inet_ntoa(*((struct in_addr *)ptr)));
			break;
		case 2:
			name = strname(udp ? buf : buf + 2, ptr);
			printf("%s: %s\n", "NSDName", name);
                        break;
		case 5:
			name = strname(udp ? buf : buf + 2, ptr);
			printf("%s: %s\n", "CName", name);
                        break;
                case 12:
			name = strname(udp ? buf : buf + 2, ptr);
			printf("%s: %s\n", "PTRDName", name);
			break;
		default:
			printf("Type: %d\n", type);
		}
                ptr += len;
        }

        return 0;
}

static char *strname(u_char *buf, u_char *ptr)
{
        char i;
        static char name[64];
        char *p = (char *)ptr;
        char *s = name;

        if (name[0] != 0)
                bzero(name, sizeof(name));

        while ((i = *p) != 0) {
		if (*p & 0xc0) {
                        p = (char *)buf + (ntohs(*((uint16_t *)p)) & 0x3fff);
                        continue;
		}
                memcpy(s, ++p, i);
                p += i;
                s += i;
                *s++ = '.';
        }
        name[strlen(name)-1] = '\0';
        return name;
}

