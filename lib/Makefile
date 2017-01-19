include ../Make.defines

LIB_OBJS = error.o wrapunix.o get_ifi_info.o free_ifi_info.o prmac.o icmp_build.o \
	   checksum.o icmp_recv.o dispatch_pcap.o loop_pcap.o open_pcap.o udp_write.o \
	   signal_func.o mcast_func.o recvdst.o
LIBTCPI_NAME = ../libtcpi.a
RANLIB = ranlib

all:	${LIB_OBJS}
		ar ${AROPT} ${LIBTCPI_NAME} $?
		${RANLIB} ${LIBTCPI_NAME}

clean:
		rm -f ${CLEANFILES} ${LIBTCPI_NAME}
