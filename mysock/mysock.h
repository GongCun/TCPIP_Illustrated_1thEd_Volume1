#ifndef _MY_SOCK_H
#define _MY_SOCK_H

#include "tcpi.h"

extern int bindport;
extern int client;
extern int server;
extern int verbose;
extern int reuseaddr;
extern int debug;
extern int linger;
extern int listenq;
extern int readlen;
extern int writelen;
extern char *rbuf; /* pointer that is malloc'ed */
extern char *wbuf; /* pointer that is malloc'ed */
extern int rcvbuflen;
extern int sndbuflen;
extern int echo;
extern int dofork;

int cliopen(char *, char *);
int servopen(char *, char *);
void sockopts(int, int);
void loop(FILE *, int);
void buffers(int);
ssize_t  writen(int fd, const void *buf, size_t len);


#endif
