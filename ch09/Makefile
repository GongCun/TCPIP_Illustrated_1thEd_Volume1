include ../Make.defines

PROGS = icmpredirect icmpselection icmpadver

all: ${PROGS}

icmpredirect: icmpredirect.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

icmpselection: icmpselection.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

icmpadver: icmpadver.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
