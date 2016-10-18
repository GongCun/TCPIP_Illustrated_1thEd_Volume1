include ../Make.defines

PROGS = ripdump ripquery ripreply test_mg multiserv

all: ${PROGS}

ripdump: ripdump.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

ripquery: ripquery.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

ripreply: ripreply.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

test_mg: test_mg.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

multiserv: multiserv.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
