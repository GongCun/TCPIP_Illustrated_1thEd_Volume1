/*
 * Copyright (c) 2003 W. Richard Stevens, Bill Fenner, Andrew M. Rudoff
 */
#ifndef __TCPI_H
#define __TCPI_H

#include "../config.h"

#define MAXLINE 4096 /* max text line length */
#define BUFFSIZE 8192 /* buffer size for reads and writes */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>
#include <sys/types.h>

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h> /* struct arpreq */
#include <net/if_dl.h> /* struct sockaddr_dl */
#include <arpa/inet.h>

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))


/* Error handler functions */
void err_dump(const char *, ...);
void err_msg(const char *, ...);
void err_quit(const char *, ...);
void err_ret(const char *, ...);
void err_sys(const char *, ...);

/* Common unix function */
int xioctl(int, unsigned long, void *);
int xsocket(int, int, int);
void *xmalloc(size_t);
void *xcalloc(size_t, size_t);

/*
 * Get_ifi_info
 */
#define IFI_NAME 16 /* same as IFINAMSIZ in <net/if.h> */
#define IFI_HADDR 8

struct ifi_info {
    char ifi_name[IFI_NAME]; /* interface name, null-terminated */
    short ifi_index; /* interface name */
    short ifi_mtu; /* interface MTU */
    u_char ifi_haddr[IFI_HADDR]; /* hardware address */
    u_short ifi_hlen; /* bytes# in hardware address: 0, 6, 8 */
    short ifi_flags; /* IFF_xxx constants from <net/if.h> */
    struct sockaddr *ifi_addr; /* primary address */
    struct sockaddr *ifi_brdaddr; /* broadcast address */
    struct sockaddr *ifi_dstaddr; /* P2P destination address */
    struct ifi_info *ifi_next; /* next of these structures */
};
struct ifi_info *get_ifi_info(void); /* only for inet4 */
void free_ifi_info(struct ifi_info *);


#endif /* __TCPI_H */


