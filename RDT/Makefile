include ../Make.defines

PROGS = t_send t_recv rdt
OBJS  = get_addr.o get_dev.o get_mtu.o make_pkt.o to_net.o \
	from_net.o krdt_connect.o make_sock.o oper_fifo.o \
	chk_chksum.o pkt_arrive.o rtt.o rexmt_pkt.o \
	pkt_debug.o krdt_listen.o rdt_connect.o rdt_listen.o \
	conn_info_debug.o conn_user_debug.o pass_pkt.o get_pkt.o \
	rdt_send.o rdt_recv.o rdt_close.o conn_debug.o rdt_pipe.o \
	conn_alloc.o rdt_fin.o

all: ${PROGS}

rdt: rdt.o ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_send: t_send.o ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

t_recv: t_recv.o ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}

${OBJS}: rdt.h

clean:
	rm -rf ${PROGS} ${CLEANFILES}
