include ../Make.defines

PROGS = udptr 

all: ${PROGS}

udptr: udptr.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

icmptr: icmptr.c
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
