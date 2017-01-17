include ../Make.defines

VERSION = 1.0
ifneq (,$(findsting "AIX",$(PLATFORM)))
	PROGS = sock simul strpt
else
	PROGS = sock simul
endif

OBJS = main.o cliopen.o servopen.o sockopts.o buffers.o loop.o writen.o tcpraw.o term.o sourcesink.o

all: ${PROGS}

${OBJS}: mysock.h version.h

sock: ${OBJS}
	${CC} ${CFLAGS} -o $@ ${OBJS} ${LIBS}

simul: simul.o
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

strpt: CFLAGS += -lbsd
strpt: strpt.c ver.h
	${CC} ${CFLAGS} -o $@ $< ${LIBS}

LEFTPAREN := (
ifeq (${CC},cc)
CCV = $(shell $(CC) -qversion | head -1 | cut -d'$(LEFTPAREN)' -f1) 
else
CCV = $(shell $(CC) --version | grep '^gcc' | head -1)
endif
VER = $(shell grep '\$$Id' strpt.c | awk '{print $$4}')
ver.h: CFLAGS += -lbsd
ver.h: strpt.c
	@echo constructing ver.h
	@rm -f ver.h
	@echo 'P("Version: ${VER}");' >./ver.h
	@echo 'P("OS version: '`oslevel -s`'");' >>./ver.h
	@echo 'P("Compiler version: ${CCV}");' >>./ver.h
	@echo 'P("Compiler flags: ${CFLAGS}");' >>./ver.h
	@echo 'P("Constructed date: '`date "+%Y %b %d %H:%M:%S"`'");' >>./ver.h

clean:
	rm -rf ${PROGS} ${CLEANFILES}
