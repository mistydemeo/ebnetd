dnl *
dnl * Process this file with autoconf to produce a configure script.
dnl *
AC_INIT(ebnetd, 1.0beta0)
AC_CONFIG_SRCDIR(ebnetd/ebnetd.c)
AC_PREREQ(2.54)
AM_INIT_AUTOMAKE

dnl *
dnl * Set mailing address.
dnl *
MAILING_ADDRESS='m-kasahr@sra.co.jp'
AC_SUBST(MAILING_ADDRESS)
AC_DEFINE_UNQUOTED(MAILING_ADDRESS, "$MAILING_ADDRESS", [Mailing address])

dnl *
dnl * C Compiler
dnl *
AC_PROG_CC

dnl *
dnl * UNIX Variants
dnl *
AC_AIX
AC_MINIX

dnl * 
dnl * GNU libtool
dnl * 
AC_PROG_LIBTOOL

dnl *
dnl * --enable-ipv6 option.
dnl *
AC_ARG_ENABLE(ipv6,
AC_HELP_STRING([--enable-ipv6],
    [IPv6 support [[yes]] (if the system supports IPv6)]),
[ENABLE_IPV6="${enableval}"], [ENABLE_IPV6='auto'])

dnl *
dnl * --with-logdir option.
dnl *
AC_ARG_WITH(logdir,
AC_HELP_STRING([--with-logdir=DIR],
    [syslog files that ndtpdaily rotates by default are placed on DIR [[LOCALSTATEDIR/ndtpd/log]]]),
[logdir="${withval}"], [logdir='$(localstatedir)/ndtpd/log'])
AC_SUBST(logdir)

dnl *
dnl * EB Library 3.x.
dnl *
eb_LIB_EB3

dnl * 
dnl * Alternative Programs.
dnl * 
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

AC_PATH_PROG(PERL, perl, no)
if test "$PERL" = no ; then
    AC_MSG_WARN(perl not found)
    PERL=/usr/bin/perl
fi

AC_PATH_PROGS(MAILX, Mail mailx, no, $PATH /usr/sbin)
if test "$MAILX" = no ; then
    AC_MSG_WARN(neither Mail nor mailx found)
    MAILX=true
fi

AC_PATH_PROG(COMPRESS, compress, no)
AC_PATH_PROG(GZIPCMD, gzip, no)
AC_PATH_PROG(BZIP2, bzip2, no)

dnl * 
dnl * Compiler Characteristics.
dnl * 
AC_C_CONST

dnl * 
dnl * Libraries.
dnl * 
AC_CHECK_LIB(resolv, res_query)
if test "$ac_cv_lib_resolv_res_query" = no ; then
    AC_CHECK_LIB(bind, res_query)
fi
AC_CHECK_LIB(nsl, gethostname)
AC_CHECK_LIB(socket, socket)

dnl * 
dnl * Library Functions.
dnl * 
AC_FUNC_VPRINTF
AC_FUNC_GETPGRP
AC_FUNC_SETPGRP
dnl AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS(getcwd getdtablesize getrlimit killpg memcpy psignal setenv \
    setlocale setsid sigsetjmp sigsetmask sigprocmask strchr sysconf \
    strsignal syslog utime vsyslog waitpid wait3)
AC_REPLACE_FUNCS(memchr memmove memset strcasecmp strerror strtol)

dnl * 
dnl * Header Files.
dnl * 
AC_DECL_SYS_SIGLIST
AC_DECL_H_ERRNO
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS(fcntl.h limits.h locale.h memory.h stdlib.h string.h \
    sys/time.h sys/wait.h syslog.h unistd.h utime.h)

dnl * 
dnl * Structures.
dnl * 
AC_HEADER_TIME
AC_HEADER_STAT
AC_UNION_WAIT
AC_STRUCT_UTIMBUF

dnl * 
dnl * Typedefs and macros.
dnl * 
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_TYPE_MODE_T
AC_TYPE_IN_PORT_T
AC_TYPE_SA_FAMILY_T
AC_TYPE_SOCKLEN_T
AC_CHECK_TYPE(ssize_t, int)

AC_TYPE_SIGNAL
if test "$ac_cv_type_signal" = void; then
    AC_DEFINE(RETSIGTYPE_VOID, 1,
	[Define if the signal function returns void.])
fi

dnl *
dnl * Check for IPv6.
dnl *
AC_STRUCT_IN6_ADDR
AC_STRUCT_SOCKADDR_IN6
AC_STRUCT_SOCKADDR_STORAGE
AC_DECL_IN6ADDR_ANY
AC_DECL_IN6ADDR_LOOPBACK

AC_CHECK_FUNCS(getaddrinfo getnameinfo)
if test "$ac_cv_func_getaddrinfo$ac_cv_func_getnameinfo" != yesyes ; then
    AC_LIBOBJ(getaddrinfo)
fi

AC_CHECK_FUNCS(inet_pton)
if test "$ac_cv_struct_in6_addr$ac_cv_func_inet_pton" != yesyes ; then
    AC_LIBOBJ(inet_pton)
fi

if test "$ac_cv_decl_in6addr_any$ac_cv_decl_in6addr_loopback" != yesyes ; then
    AC_LIBOBJ(dummyin6)
fi

if test "$ENABLE_IPV6" = auto ; then
    if test "$ac_cv_struct_in6_addr$ac_cv_func_getaddrinfo" = yesyes ; then
	ENABLE_IPV6=yes
    else
	ENABLE_IPV6=no
    fi
fi

AC_MSG_CHECKING(for IPv6 support)
if test "$ENABLE_IPV6" = yes ; then
    if test "$ac_cv_struct_in6_addr" = no ; then
	AC_MSG_ERROR(IPv6 not available)
    fi
    if test "${ac_cv_func_getaddrinfo}" = no ; then
	AC_MSG_ERROR(IPv6 not available)
    fi
    AC_DEFINE(ENABLE_IPV6, 1, [Define if build with IPv6 support])
fi
AC_MSG_RESULT($ENABLE_IPV6)

dnl * 
dnl * Output Files.
dnl * 
AC_CONFIG_HEADER(config.h)
AC_CONFIG_LINKS([ndtpd/book.c:ebnetd/book.c
    ebhttpd/book.c:ebnetd/book.c
    ndtpd/config.c:ebnetd/config.c
    ebhttpd/config.c:ebnetd/config.c
    ndtpd/defs.h.in:ebnetd/defs.h.in
    ebhttpd/defs.h.in:ebnetd/defs.h.in
    ndtpd/ndtpcheck.c:ebnetd/ebncheck.c
    ebhttpd/ebhtcheck.c:ebnetd/ebncheck.c
    ndtpd/ndtpcontrol.c:ebnetd/ebncontrol.c
    ebhttpd/ebhtcontrol.c:ebnetd/ebncontrol.c
    ndtpd/ndtpd.c:ebnetd/ebnetd.c
    ebhttpd/ebhttpd.c:ebnetd/ebnetd.c
    ndtpd/global.c:ebnetd/global.c
    ebhttpd/global.c:ebnetd/global.c
    ndtpd/misc.c:ebnetd/misc.c
    ebhttpd/misc.c:ebnetd/misc.c
    ndtpd/getopt_long.pl:ebnetd/getopt_long.pl
    ebhttpd/getopt_long.pl:ebnetd/getopt_long.pl])
AC_CONFIG_FILES([Makefile lib/Makefile ebtiny/Makefile ebnetd/Makefile
    ndtpd/Makefile ebhttpd/Makefile doc/Makefile doc-ja/Makefile m4/Makefile])
AC_OUTPUT
