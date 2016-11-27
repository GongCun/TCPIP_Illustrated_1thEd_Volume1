include ../Make.defines

PROGS = mjoin igmpdump

all: ${PROGS}

mjoin: mjoin.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}
igmpdump: igmpdump.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}
clean:
	rm -rf ${PROGS} ${CLEANFILES}
