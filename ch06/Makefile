include ../Make.defines

PROGS = icmpaddrmask

all: ${PROGS}

icmpaddrmask: icmpaddrmask.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -f ${PROGS} ${CLEANFILES}
