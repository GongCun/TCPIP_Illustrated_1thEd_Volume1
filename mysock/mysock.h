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

int cliopen(char *, char *);
int servopen(char *, char *);
void sockopts(int, int);

#endif
