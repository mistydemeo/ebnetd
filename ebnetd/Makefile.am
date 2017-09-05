pkglocalstatedir = $(localstatedir)/$(PACKAGE)
logdir = @logdir@

LIBEBNETD = $(top_builddir)/lib/libebnetd.a
LIBEBTINY = $(top_builddir)/ebtiny/libebtiny.a

sbin_PROGRAMS = ebnetd ebncheck ebncontrol
sbin_SCRIPTS = ebndaily ebnupgrade
libexec_SCRIPTS = ebnstat

ebnetd_SOURCES = book.c config.c global.c misc.c ebnetd.c \
	ebnet.c
ebnetd_LDADD = $(LIBEBNETD) $(LIBEBTINY)
ebnetd_DEPENDENCIES = $(LIBEBNETD) $(LIBEBTINY)

ebncheck_SOURCES = book.c config.c global.c misc.c ebncheck.c
ebncheck_LDADD = $(LIBEBNETD) $(LIBEBTINY)
ebncheck_DEPENDENCIES = $(LIBEBNETD) $(LIBEBTINY)

ebncontrol_SOURCES = book.c config.c global.c misc.c ebncontrol.c
ebncontrol_LDADD = $(LIBEBNETD) $(LIBEBTINY)
ebncontrol_DEPENDENCIES = $(LIBEBNETD) $(LIBEBTINY)

INCLUDES = -DUSE_FAKELOG -DEBNETD -I$(top_srcdir) -I$(top_srcdir)/lib \
	$(EBCONF_EBINCS)

EXTRA_DIST = ebndaily.in ebnstat.in ebnupgrade.in defs.h.in getopt_long.pl
CLEANFILES = defs.h stamp-defs-h defs.h.tmp ebnstat ebnstat.tmp \
	ebndaily ebndaily.tmp ebnupgrade ebnupgrade.tmp
BUILT_SOURCES = defs.h

defs.h: stamp-defs-h

stamp-defs-h: defs.h.in Makefile
	rm -f defs.h.tmp
	sed -e 's;\@sysconfdir\@;$(sysconfdir);' \
	   -e 's;\@pkglocalstatedir\@;$(pkglocalstatedir);' \
	   $(srcdir)/defs.h.in > defs.h.tmp
	$(top_srcdir)/move-if-change defs.h.tmp defs.h
	@echo timestamp > stamp-defs-h

ebnstat: getopt_long.pl ebnstat.in Makefile
	rm -f ebnstat
	sed -e 's;\@PERL\@;$(PERL);' \
	    $(srcdir)/ebnstat.in | sed '/^$$/q' > ebnstat.tmp
	cat $(srcdir)/getopt_long.pl >> ebnstat.tmp
	sed -e 's;\@VERSION\@;$(VERSION);' \
	   -e 's;\@MAILING_ADDRESS\@;$(MAILING_ADDRESS);' \
	   $(srcdir)/ebnstat.in | sed '1,/^$$/d' >> ebnstat.tmp
	cp ebnstat.tmp ebnstat
	chmod +x ebnstat
	rm -f ebnstat.tmp

ebndaily: getopt_long.pl ebndaily.in Makefile
	rm -f ebndaily
	sed -e 's;\@PERL\@;$(PERL);' \
	    $(srcdir)/ebndaily.in | sed '/^$$/q' > ebndaily.tmp
	cat $(srcdir)/getopt_long.pl >> ebndaily.tmp
	EBNSTAT="`echo ebnstat | sed '$(transform)'`"; \
	NDTPSTAT="`echo ndtpstat | sed '$(transform)'`"; \
	EBHTSTAT="`echo ebhtstat | sed '$(transform)'`"; \
	sed -e 's;\@logdir\@;$(logdir);' \
	   -e 's;\@libexecdir\@;$(libexecdir);' \
	   -e 's;\@VERSION\@;$(VERSION);' \
	   -e 's;\@MAILX\@;$(MAILX);' \
	   -e 's;\@COMPRESS\@;$(COMPRESS);' \
	   -e 's;\@GZIPCMD\@;$(GZIPCMD);' \
	   -e 's;\@BZIP2\@;$(BZIP2);' \
	   -e 's;\@MAILING_ADDRESS\@;$(MAILING_ADDRESS);' \
	   -e 's;\@EBNSTAT\@;'$$EBNSTAT';' \
	   -e 's;\@NDTPSTAT\@;'$$NDTPSTAT';' \
	   -e 's;\@EBHTSTAT\@;'$$EBHTSTAT';' \
	   $(srcdir)/ebndaily.in | sed '1,/^$$/d' >> ebndaily.tmp
	cp ebndaily.tmp ebndaily
	chmod +x ebndaily
	rm -f ebndaily.tmp

ebnupgrade: getopt_long.pl ebnupgrade.in Makefile
	rm -f ebnupgrade
	sed -e 's;\@PERL\@;$(PERL);' \
	    $(srcdir)/ebnupgrade.in | sed '/^$$/q' > ebnupgrade.tmp
	cat $(srcdir)/getopt_long.pl >> ebnupgrade.tmp
	sed -e 's;\@VERSION\@;$(VERSION);' \
	   -e 's;\@MAILING_ADDRESS\@;$(MAILING_ADDRESS);' \
	   -e 's;\@sysconfdir\@;$(sysconfdir);' \
	   $(srcdir)/ebnupgrade.in | sed '1,/^$$/d' >> ebnupgrade.tmp
	cp ebnupgrade.tmp ebnupgrade
	chmod +x ebnupgrade
	rm -f ebnupgrade.tmp
