include ../Make.defines

PROGS = rdt
OBJS = main.o getlocaddr.o make_pkt.o to_net.o

all: ${PROGS}

rdt: ${OBJS}
	${CC} ${CFLAGS} -o $@ ${OBJS} ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
