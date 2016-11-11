include ../Make.defines

PROGS = udpcksum udpfrag udpwrite udpsend udprecv

all: ${PROGS}

udpcksum: udpcksum.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

udpfrag: udpfrag.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

udpwrite: udpwrite.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

udpsend: udpsend.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

udprecv: udprecv.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}
clean:
	rm -rf ${PROGS} ${CLEANFILES}
