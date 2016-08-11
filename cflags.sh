#!/bin/sh
# $Id: cflags.sh,v 1.2 2016/07/27 23:27:17 gongcunjust Exp $

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
for folder in /usr/lib /usr/lib64 /usr/local/lib; do
    if test -d $folder; then
        LIBS="-L$folder -lpcap"
        cc -o ${PID}.exe ${PID}.c $LIBS >/dev/null 2>&1 && {
          found=1; break
        }
    fi
done

test $found -eq 0 && unset LIBS

rm -rf ${PID}.* 2>/dev/null

echo "$CFLAGS%$LIBS"

