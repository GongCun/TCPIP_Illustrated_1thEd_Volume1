include ../Make.defines

PROGS = icmpaddrmask icmptime icmpdesterr

all: ${PROGS}

icmpaddrmask: icmpaddrmask.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

icmptime: icmptime.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

icmpdesterr: icmpdesterr.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -f ${PROGS} ${CLEANFILES}
