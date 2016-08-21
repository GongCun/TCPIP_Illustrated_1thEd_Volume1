#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([lib/tcpi.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS(sys/sysctl.h sys/param.h)
AC_CHECK_HEADERS(pcap.h)
AC_CHECK_HEADERS(net/if.h netinet/in.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h syslog.h net/if_arp.h net/if_dl.h, [], [], [
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct ifreq.ifr_mtu], [], [],
[#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
])
AC_CHECK_MEMBERS([struct sockaddr.sa_len], [AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, [Define to 1 if socket address structures have length fields])], [],
[#include <sys/types.h>
#include <sys/socket.h>
])

AC_CHECK_TYPE([struct sockaddr_dl], [AC_DEFINE(HAVE_SOCKADDR_DL_STRUCT, 1, [Define to 1 if <net/if_dl.h> defines struct sockaddr_dl])], [],
[#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>
])

AC_CHECK_TYPE([struct sockaddr_inarp], [AC_DEFINE(HAVE_SOCKADDR_INARP_STRUCT, 1, [Define to 1 if <netinet/if_ether.h> defines struct sockaddr_inarp])], [],
[#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/if_ether.h>
])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero socket strchr strerror vsnprintf sysctl])

# Check for system services.

# Let's see if the system really supports IPv6
AC_MSG_CHECKING(for IPv6 support)
AC_CACHE_VAL(ac_cv_ipv6,
AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
int main(void)
{
	int fd;
	struct sockaddr_in6 foo;
	fd = socket(AF_INET6, SOCK_STREAM, 0);
	exit((fd >= 0) ? 0 : 1);
}],
[ac_cv_ipv6=yes],
[ac_cv_ipv6=no],
[ac_cv_ipv6=no]))
AC_MSG_RESULT($ac_cv_ipv6)
if test $ac_cv_ipv6 = yes; then
	AC_DEFINE([IPV6], 1, [Define to 1 if the system supports IPv6])
	AC_DEFINE([IPv6], 1, [Define to 1 if the system supports IPv6])
fi

AC_CONFIG_FILES([lib/Makefile])
AC_OUTPUT
