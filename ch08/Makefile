include ../Make.defines

PROGS = udptr icmptr 

all: ${PROGS}

udptr: udptr.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

icmptr: icmptr.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
