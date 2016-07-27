include ../Make.defines

PROGS = ifinfo

all: ${PROGS}

ifinfo: ifinfo.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -f ${PROGS} ${CLEANFILES}
