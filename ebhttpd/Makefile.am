@SET_MAKE@

pkglocalstatedir = $(localstatedir)/ebnetd
logdir = @logdir@

LIBEBNETD = $(top_builddir)/lib/libebnetd.a

sbin_PROGRAMS = ebhttpd ebhtcheck ebhtcontrol
libexec_SCRIPTS = ebhtstat

ebhttpd_SOURCES = book.c config.c global.c misc.c ebhttpd.c \
	cgiquery.c entity.c generator.c hookset.c httpdate.c http.c \
	httpmisc.c httprequest.c httprescode.c pathparts.c urlparts.c

ebhttpd_LDADD = $(LIBEBNETD) $(EBCONF_EBLIBS) $(EBCONF_ZLIBLIBS) \
	$(EBCONF_INTLLIBS)
ebhttpd_DEPENDENCIES = $(LIBEBNETD)

ebhtcheck_SOURCES = book.c config.c global.c ebhtcheck.c misc.c
ebhtcheck_LDADD = $(LIBEBNETD) $(EBCONF_EBLIBS) $(EBCONF_ZLIBLIBS) \
	$(EBCONF_INTLLIBS)
ebhtcheck_DEPENDENCIES = $(LIBEBNETD)

ebhtcontrol_SOURCES = book.c config.c global.c ebhtcontrol.c misc.c
ebhtcontrol_LDADD = $(LIBEBNETD) $(EBCONF_EBLIBS) $(EBCONF_ZLIBLIBS) \
	$(EBCONF_INTLLIBS)
ebhtcontrol_DEPENDENCIES = $(LIBEBNETD)

INCLUDES = -DUSE_FAKELOG -DEBHTTPD -I$(top_srcdir) -I$(top_srcdir)/lib \
	$(EBCONF_EBINCS) $(EBCONF_ZLIBINCS) $(EBCONF_INTLINCS)

EXTRA_DIST = ebhtstat.in
CLEANFILES = defs.h stamp-defs-h defs.h.tmp ebhtstat ebhtstat.tmp
DISTCLEANFILES = book.c config.c defs.h.in global.c misc.c ebhttpd.c \
	ebhtcheck.c ebhtcontrol.c getopt_long.pl
BUILT_SOURCES = defs.h
noinst_HEADERS = cgiquery.h entity.h http.h httpdate.h httprequest.h \
	httprescode.h pathparts.h urlparts.h

defs.h: stamp-defs-h

stamp-defs-h: defs.h.in Makefile
	rm -f defs.h.tmp
	sed -e 's;\@sysconfdir\@;$(sysconfdir);' \
	   -e 's;\@pkglocalstatedir\@;$(pkglocalstatedir);' \
	   defs.h.in > defs.h.tmp
	$(top_srcdir)/move-if-change defs.h.tmp defs.h
	@echo timestamp > stamp-defs-h

ebhtstat: getopt_long.pl ebhtstat.in Makefile
	rm -f ebhtstat
	sed -e 's;\@PERL\@;$(PERL);' \
	    $(srcdir)/ebhtstat.in | sed '/^$$/q' > ebhtstat.tmp
	cat getopt_long.pl >> ebhtstat.tmp
	sed -e 's;\@VERSION\@;$(VERSION);' \
	   -e 's;\@MAILING_ADDRESS\@;$(MAILING_ADDRESS);' \
	   $(srcdir)/ebhtstat.in | sed '1,/^$$/d' >> ebhtstat.tmp
	cp ebhtstat.tmp ebhtstat
	chmod +x ebhtstat
	rm -f ebhtstat.tmp
