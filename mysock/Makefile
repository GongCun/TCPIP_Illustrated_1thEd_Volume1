include ../Make.defines

VERSION = 1.0
PROGS = sock

OBJS = main.o cliopen.o servopen.o sockopts.o buffers.o loop.o writen.o

all: ${PROGS}

${OBJS}: mysock.h version.h

sock: ${OBJS}
	${CC} ${CFLAGS} -o $@ ${OBJS} ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
