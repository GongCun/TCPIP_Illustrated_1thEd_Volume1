include ../Make.defines

VERSION = 1.0
PROGS = sock simul

OBJS = main.o cliopen.o servopen.o sockopts.o buffers.o loop.o writen.o tcpraw.o

all: ${PROGS}

${OBJS}: mysock.h version.h

sock: ${OBJS}
	${CC} ${CFLAGS} -o $@ ${OBJS} ${LIBS}

simul: simul.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
