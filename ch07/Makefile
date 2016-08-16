include ../Make.defines

PROGS = Ping

all: ${PROGS}

Ping: main.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

clean:
	rm -f ${PROGS} ${CLEANFILES}
