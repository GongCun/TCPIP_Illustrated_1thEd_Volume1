include ../Make.defines

PROGS = icmpredirect

all: ${PROGS}

icmpredirect: icmpredirect.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
