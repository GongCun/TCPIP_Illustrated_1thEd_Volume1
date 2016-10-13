include ../Make.defines

PROGS = ripdump ripquery ripreply

all: ${PROGS}

ripdump: ripdump.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

ripquery: ripquery.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

ripreply: ripreply.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
