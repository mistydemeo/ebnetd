pkglocalstatedir = $(localstatedir)/$(PACKAGE)
logdir = @logdir@

LIBEBNETD = $(top_builddir)/lib/libebnetd.a

sbin_PROGRAMS = ndtpd ndtpcheck ndtpcontrol
libexec_SCRIPTS = ndtpstat

ndtpd_SOURCES = book.c config.c global.c misc.c ndtpd.c \
	ndtp.c hookset.c
ndtpd_LDADD = $(LIBEBNETD) $(EBCONF_EBLIBS) $(EBCONF_ZLIBLIBS) \
	$(EBCONF_INTLLIBS)
ndtpd_DEPENDENCIES = $(LIBEBNETD)

ndtpcheck_SOURCES = book.c config.c global.c misc.c ndtpcheck.c
ndtpcheck_LDADD = $(LIBEBNETD) $(EBCONF_EBLIBS) $(EBCONF_ZLIBLIBS) \
	$(EBCONF_INTLLIBS)
ndtpcheck_DEPENDENCIES = $(LIBEBNETD)

ndtpcontrol_SOURCES = book.c config.c global.c misc.c ndtpcontrol.c
ndtpcontrol_LDADD = $(LIBEBNETD) $(EBCONF_EBLIBS) $(EBCONF_ZLIBLIBS) \
	$(EBCONF_INTLLIBS)
ndtpcontrol_DEPENDENCIES = $(LIBEBNETD)

noinst_HEADERS = ndtp.h

INCLUDES = -DUSE_FAKELOG -DNDTPD -I$(top_srcdir)/lib $(EBCONF_EBINCS)

EXTRA_DIST = ndtpstat.in
CLEANFILES = defs.h stamp-defs-h defs.h.tmp ndtpstat ndtpstat.tmp
DISTCLEANFILES = book.c config.c defs.h.in global.c misc.c ndtpd.c \
	ndtpcheck.c ndtpcontrol.c getopt_long.pl
BUILT_SOURCES = defs.h

defs.h: stamp-defs-h

stamp-defs-h: defs.h.in Makefile
	rm -f defs.h.tmp
	sed -e 's;\@sysconfdir\@;$(sysconfdir);' \
	   -e 's;\@pkglocalstatedir\@;$(pkglocalstatedir);' \
	   defs.h.in > defs.h.tmp
	$(top_srcdir)/move-if-change defs.h.tmp defs.h
	@echo timestamp > stamp-defs-h

ndtpstat: getopt_long.pl ndtpstat.in Makefile
	rm -f ndtpstat
	sed -e 's;\@PERL\@;$(PERL);' \
	    $(srcdir)/ndtpstat.in | sed '/^$$/q' > ndtpstat.tmp
	cat getopt_long.pl >> ndtpstat.tmp
	sed -e 's;\@VERSION\@;$(VERSION);' \
	   -e 's;\@MAILING_ADDRESS\@;$(MAILING_ADDRESS);' \
	   $(srcdir)/ndtpstat.in | sed '1,/^$$/d' >> ndtpstat.tmp
	cp ndtpstat.tmp ndtpstat
	chmod +x ndtpstat
	rm -f ndtpstat.tmp
