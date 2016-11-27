include ../Make.defines

PROGS = udpdns dnsquery

all: ${PROGS}

udpdns: udpdns.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

dnsquery: dnsquery.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}
clean:
	rm -rf ${PROGS} ${CLEANFILES}
