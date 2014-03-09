##
## Autoconf template.
##
AC_INIT

TOPDIR=${ac_pwd}
AC_SUBST(TOPDIR)

AC_CONFIG_AUX_DIR(./mk)
AC_CONFIG_MACRO_DIR(./mk)

if test "${prefix}" = "NONE"; then
    prefix=/usr/local
fi
if test "${exec_prefix}" = "NONE"; then
    exec_prefix='${prefix}'
fi

SHELL=`which bash` > /dev/null 2>&1
if test $? -ne 0 -o -z "${SHELL}"; then
   AC_MSG_ERROR([To run libtool you need the GNU bash, not the old-school /bin/sh.])
   exit 1
fi
export SHELL
AC_SUBST(SHELL)

LT_INIT
AC_SUBST(LIBTOOL)
AC_SUBST(LIBTOOL_DEPS)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_AWK

RM="rm -f"
AC_SUBST(RM)

MKDIR="mkdir -p"
AC_SUBST(MKDIR)

# headers/macros

AC_CONFIG_HEADER([src/include/mccp/mccp_config.h])

AC_C_BIGENDIAN

AC_HEADER_STDC

AC_CHECK_HEADER(inttypes.h, [AC_DEFINE(HAVE_INTTYPES_H)], [])
AC_CHECK_HEADER(stdint.h, [AC_DEFINE(HAVE_STDINT_H)])
AC_CHECK_HEADER(limits.h, [AC_DEFINE(HAVE_LIMITS_H)])
AC_CHECK_HEADER(sys/param.h, [AC_DEFINE(HAVE_SYS_PARAM_H)])
AC_CHECK_HEADER(sys/types.h, [AC_DEFINE(HAVE_SYS_TYPES_H)])
AC_CHECK_HEADER(ctype.h, [AC_DEFINE(HAVE_CTYPE_H)])
AC_CHECK_HEADER(stdio.h, [AC_DEFINE(HAVE_STDIO_H)])
AC_CHECK_HEADER(stdlib.h, [AC_DEFINE(HAVE_STDLIB_H)])
AC_CHECK_HEADER(unistd.h, [AC_DEFINE(HAVE_UNISTD_H)])
AC_CHECK_HEADER(memory.h, [AC_DEFINE(HAVE_MEMORY_H)])
AC_CHECK_HEADER(string.h, [AC_DEFINE(HAVE_STRING_H)])
AC_CHECK_HEADER(malloc.h, [AC_DEFINE(HAVE_MALLOC_H)])
AC_CHECK_HEADER(stdarg.h, [AC_DEFINE(HAVE_STDARG_H)])
AC_CHECK_HEADER(dirent.h, [AC_DEFINE(HAVE_DIRENT_H)])
AC_CHECK_HEADER(time.h, [AC_DEFINE(HAVE_TIME_H)])
AC_CHECK_HEADER(sys/time.h, [AC_DEFINE(HAVE_SYS_TIME_H)])
AC_CHECK_HEADER(sys/termios.h, [AC_DEFINE(HAVE_SYS_TERMIOS_H)])
AC_CHECK_HEADER(sys/stat.h, [AC_DEFINE(HAVE_SYS_STAT_H)])
AC_CHECK_HEADER(fcntl.h, [AC_DEFINE(HAVE_FCNTL_H)])
AC_CHECK_HEADER(errno.h, [AC_DEFINE(HAVE_ERRNO_H)])
AC_CHECK_HEADER(signal.h, [AC_DEFINE(HAVE_SIGNAL_H)])
AC_CHECK_HEADER(netdb.h, [AC_DEFINE(HAVE_NETDB_H)])
AC_CHECK_HEADER(netinet/tcp.h, [AC_DEFINE(HAVE_NETINET_TCP_H)])
AC_CHECK_HEADER(sys/socket.h, [AC_DEFINE(HAVE_SYS_SOCKET_H)])
AC_CHECK_HEADER(arpa/inet.h, [AC_DEFINE(HAVE_ARPA_INET_H)])
AC_CHECK_HEADER(sys/ioctl.h, [AC_DEFINE(HAVE_SYS_IOCTL_H)])
AC_CHECK_HEADER(sys/wait.h, [AC_DEFINE(HAVE_SYS_WAIT_H)])
AC_CHECK_HEADER(pthread.h, [AC_DEFINE(HAVE_PTHREAD_H)])
AC_CHECK_HEADER(stdbool.h, [AC_DEFINE(HAVE_STDBOOL_H)])
AC_CHECK_HEADER(sys/resource.h, [AC_DEFINE(HAVE_SYS_RESOURCE_H)])
AC_CHECK_HEADER(regex.h, [AC_DEFINE(HAVE_REGEX_H)])
AC_CHECK_HEADER(syslog.h, [AC_DEFINE(HAVE_SYSLOG_H)])
AC_CHECK_HEADER(mcheck.h, [AC_DEFINE(HAVE_MCHECK_H)])

AC_CHECK_HEADER(gmp.h, [AC_DEFINE(HAVE_GMP_H)], [AC_MSG_ERROR([The GNU MP must be installed.])])

resultOK=0
GMP_LIBS=''
oLIBS=${LIBS}
LIBS='-lgmp'
AC_MSG_CHECKING([mpz_init_set_str])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]
		 [#include <gmp.h>]],
		[[char *str = "12345678";]
		 [MP_INT i;]
		 [int r;]
		 [r = mpz_init_set_str(&i, str, 10);]
		 [return r;]])],
	[resultOK=1],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
LIBS=${oLIBS}
if test ${resultOK} -eq 1; then
    AC_MSG_RESULT([available.])
    GMP_LIBS='-lgmp'
else
    AC_MSG_ERROR([The GNU MP library must be installed.])
fi
AC_SUBST(GMP_LIBS)

AC_CHECK_FUNC(strtold, [AC_DEFINE(HAVE_STRTOLD)])

# Assume 32 bit for cross compilation.
AC_CHECK_SIZEOF(short int, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long int, 4)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(int64_t, 8)
AC_CHECK_SIZEOF(size_t, 4)
AC_CHECK_SIZEOF(void *, 4)
AC_CHECK_SIZEOF(pthread_t, 4)

# size_t printf() designator.
oCFLAGS=${CFLAGS}
CFLAGS="${CFLAGS} -Wall -Werror"
AC_MSG_CHECKING([print format for size_t])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]],
		[[char buf[24];]
		 [snprintf(buf, sizeof(buf), "%zu", (size_t)1);]
		 [return (strcmp("1", buf) == 0 && strlen(buf) == 1) ? ]
		 [0 : 1;]])],
	[AC_DEFINE(HAVE_PRINT_FORMAT_FOR_SIZE_T)
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}

# pthread_setname_np()
oCFLAGS=${CFLAGS}
oLDFLAGS=${LDFLAGS}
CFLAGS="${CFLAGS} -Wall -Werror -D_REENTRANT -D_GNU_SOURCE -pthread"
LDFLAGS="${LDFLAGS} -lpthread"
AC_MSG_CHECKING([pthread_setname_np()])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]
		 [#include <pthread.h>]],
		[[char buf[16];]
		 [snprintf(buf, sizeof(buf), "a thread");]
		 [return pthread_setname_np(pthread_self(), buf);]])],
	[AC_DEFINE(HAVE_PTHREAD_SETNAME_NP)
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}
LDFLAGS="${oLDFLAGS} ${LIBS}"

AC_CHECK_TYPES([sighandler_t], [AC_DEFINE(HAVE_SIGHANDLER_T)], [],
    [[#include <signal.h>]])
AC_CHECK_TYPES([sig_t], [AC_DEFINE(HAVE_SIG_T)], [],
    [[#include <signal.h>]])

# sysdeps.

eval `echo ${target} | awk -F- '{ printf "tagCPU=%s; tagOS=%s;\n", $1, $3 }'`

changequote(<<, >>)dnl
CPU=`echo ${tagCPU} | tr '[A-Z]' '[a-z]'`
OS=`echo ${tagOS} | tr '[A-Z]' '[a-z]'`
changequote([, ])dnl
unset tagOS tagCPU

case $CPU in
    i*86)
        CPU=i386;;
esac

OS_CPPFLAGS='-D_REENTRANT'
OS_LDFLAGS=''
OS_LIBS=''
OS_CFLAGS=''
if test -z "${MAKE}"; then
	MAKE=make
fi

ECHO_N='echo -n'
STAT='stat -c %a'

case $OS in
    sunos*)
        OS=sunos;;
    solaris*)
        OS=solaris;;
    freebsd*)
	OS=freebsd;;
    linux*)
	OS_CPPFLAGS="${OS_CPPFLAGS} -D_GNU_SOURCE -D_POSIX_SOURCE"
	OS_LIBS="-lrt -lpthread"
	OS=linux;;
    netbsd*)
        MAKE=gmake
        OS_CPPFLAGS="${OS_CPPFLAGS} -I/usr/pkg/include"
    	OS_LDFLAGS="-L/usr/pkg/lib"
	OS_LIBS="-lpthread"
	OS=netbsd;;
    darwin*)
	ECHO_N='/bin/echo -n'
	STAT='stat -f %p'
	OS_CPPFLAGS="${OS_CPPFLAGS} -D_GNU_SOURCE -D_DARWIN_C_SOURCE"
	OS_LIBS="-lpthread"
	OS=darwin;;
esac

AC_SUBST(ECHO_N)
AC_SUBST(STAT)
AC_SUBST(MAKE)
AC_SUBST(OS_CPPFLAGS)
AC_SUBST(OS_CFLAGS)
AC_SUBST(OS_LDFLAGS)
AC_SUBST(OS_LIBS)

changequote(<<, >>)dnl
CPUDEF=MCCP_CPU_`echo $CPU | tr '[a-z]' '[A-Z]'`
OSDEF=MCCP_OS_`echo $OS | tr '[a-z]' '[A-Z]'`
changequote([, ])dnl
AC_SUBST(CPUDEF)
AC_SUBST(OSDEF)

MKRULESDIR=${TOPDIR}/mk
AC_SUBST(MKRULESDIR)

CPPFLAGS=''
CFLAGS=''
AC_SUBST(CPPFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)

# emit.
AC_CONFIG_FILES(
	mk/changeincludes
	mk/changename
	mk/doDosText.sh
	mk/initdepend
	mk/vars.mk
	Makefile
	src/include/mccp/Makefile
	src/include/mccp/mccp_platform.h
	src/lib/Makefile
	src/lib/check/Makefile,
	[chmod 755 \
		mk/changeincludes \
		mk/changename \
		mk/doDosText.sh \
		mk/initdepend]
)

AC_CONFIG_COMMANDS([modify-config],
	[sh mk/finalizeconfig ${OS_CPPFLAGS}],
	[OS_CPPFLAGS="${OS_CPPFLAGS}"])

AC_CONFIG_COMMANDS([depend],
	[sh mk/initdepend && ${MAKE} depend],
	[MAKE=${MAKE}])

AC_CONFIG_COMMANDS([config-clean],
	[rm -rf ./config.log ./config.status.* autom4te.cache])

AC_OUTPUT

${MAKE} clean

echo ""
echo "CC:	'${CC}'"
echo "CFLAGS:	'${OS_CFLAGS}'"
echo "CPPFLAGS:	'${CPPFLAGS} ${OS_CPPFLAGS} ${CPUDEF} ${OSDEF}'"
echo "LDFLAGS:	'${LDFLAGS} ${OS_LDFLAGS} ${OS_LIBS}'"
echo ""
