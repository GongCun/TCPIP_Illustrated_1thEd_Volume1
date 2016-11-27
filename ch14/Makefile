include ../Make.defines

PROGS = udpdns

all: ${PROGS}

udpdns: udpdns.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
