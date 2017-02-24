include ../Make.defines

PROGS = t_send t_recv t_uxsend t_uxrecv t_psend t_precv rdt
OBJS  = get_addr.o get_dev.o get_mtu.o make_pkt.o to_net.o \
	from_net.o krdt_connect.o make_sock.o make_child.o xmit_pkt.o \
	chk_chksum.o pkt_arrive.o rtt.o rtt_alloc.o rexmt_pkt.o \
	pkt_debug.o krdt_listen.o rdt_connect.o rdt_listen.o \
	conn_info_debug.o

all: ${PROGS}

rdt: rdt.o ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_send: t_send.o ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_recv: t_recv.o ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_uxsend: t_uxsend.o
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_uxrecv: t_uxrecv.o
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_psend: t_psend.o
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_precv: t_precv.o
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

clean:
	rm -rf ${PROGS} ${CLEANFILES}
