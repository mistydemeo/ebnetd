noinst_LIBRARIES = libebnetd.a

libebnetd_a_SOURCES = confutil.c daemon.c fakelog.c fdset.c filename.c \
	getopt.c hostname.c inet_pton2.c ioall.c linebuf.c logpid.c \
	makedir.c mappedaddr.c openmax.c permission.c privilege.c \
	readconf.c serverport.c signame.c ticket.c wildcard.c
libebnetd_a_LIBADD = @LIBOBJS@

noinst_HEADERS = dummyin6.h getaddrinfo.h confutil.h daemon.h fakelog.h \
	fdset.h filename.h getopt.h hostname.h inet_pton2.h ioall.h \
	linebuf.h logpid.h makedir.h mappedaddr.h openmax.h permission.h \
	privilege.h readconf.h serverport.h signame.h ticket.h wildcard.h

INCLUDES = -DUSE_FAKELOG
