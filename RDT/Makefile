include ../Make.defines

PROGS = t_send t_mtu t_recv
OBJS1 = t_send.o get_addr.o make_pkt.o to_net.o
OBJS2 = t_mtu.o get_addr.o get_dev.o get_mtu.o
OBJS3 = t_recv.o get_addr.o get_dev.o get_mtu.o from_net.o make_child.o \
	xmit_pkt.o chk_chksum.o krdt_listen.o pkt_arrive.o make_sock.o \
	to_net.o make_pkt.o

all: ${PROGS}

t_send: ${OBJS1}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_mtu: ${OBJS2}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_recv: ${OBJS3}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
