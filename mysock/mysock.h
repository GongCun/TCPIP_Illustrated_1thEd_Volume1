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
extern int timeout;
extern int pauselisten;
extern int rawopt;
extern unsigned int seq;
extern unsigned int ack;
extern unsigned char event; /* ACK|PSH|RST|SYN|FIN|URG */
extern int cbreak;
extern int nodelay;
extern int sourcesink;
extern int nbuf;
extern int pauserw;
extern int pauseinit;
extern int pauseclose;
extern int urgwrite;
extern int mss;
extern int timestamp;

int cliopen(char *, char *);
int servopen(char *, char *);
void sockopts(int, int);
void loop(FILE *, int);
void buffers(int);
ssize_t  writen(int fd, const void *buf, size_t len);
void tcpraw(unsigned char, int, unsigned int, unsigned int, char *, char *, int, int);

int tty_cbreak(int fd);
int tty_reset(int fd);
void tty_atexit(void);
void sink(int sockfd);

#endif
