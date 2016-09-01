#!/bin/sh
# $Id: cflags.sh,v 1.3 2016/09/01 07:02:27 gongcunjust Exp gongcunjust $

PID=$$
OS=`uname -s | tr '[:lower:]' '[:upper:]'`

CFLAGS="-D_${OS} -Wall" # suppose gcc

if test "X$OS" = "XAIX"; then
    unset CFLAGS

    # xlc or gcc ?
    echo "main(){}" >${PID}.c
    cc -o ${PID}.o -c ${PID}.c -qlist >/dev/null 2>&1
    if head -1 ${PID}.lst 2>/dev/null | grep -i 'XL C for AIX' >/dev/null 2>&1; then
        CFLAGS="-qflag=w:w"
        if ls -l /unix | grep unix_64 >/dev/null 2>&1; then
            CFLAGS="-D_AIX64 -q64 $CFLAGS"
        else
            CFLAGS="-D_AIX $CFLAGS"
        fi
    else
        CFLAGS="-Wall" # suppose gcc
        if ls -l /unix | grep unix_64 >/dev/null 2>&1; then
            CFLAGS="-D_AIX64 -maix64 $CFLAGS"
        else
            CFLAGS="-D_AIX $CFLAGS"
        fi
    fi
    rm -f ${PID}.* >/dev/null 2>&1
fi

#-+- Find the pcap library -+-
cat >./${PID}.c <<\EOF
#include <pcap.h>
#include <stdlib.h>

int main(void) {
  printf("%s\n", pcap_lib_version());
  return 0;
}
EOF

typeset -i found=0
for folder in /usr/local/lib /usr/lib64 /usr/lib; do
    if test -d $folder && ls -1 $folder | grep -w libpcap >/dev/null 2>&1; then
        LIBS="-L$folder -lpcap"
        cc -o ${PID}.x ${PID}.c $LIBS >/dev/null 2>&1 && {
          found=1; break
        }
    fi
done

test $found -eq 0 && unset LIBS || {
cat >./${PID}.c <<\EOF  
#include <stdlib.h>
#include <pcap.h>
#if defined(_AIX) || defined(_AIX64)
#include <net/bpf.h>
#endif

int main(void) {
  struct bpf_program bp;
  pcap_freecode(&bp);
  return 0;
}
EOF
cc -o ${PID}.x ${PID}.c $LIBS >/dev/null 2>&1 && \
CFLAGS="-DHAVE_PCAP_FREECODE $CFLAGS"
}

rm -rf ${PID}.* 2>/dev/null

echo "$CFLAGS%$LIBS"

