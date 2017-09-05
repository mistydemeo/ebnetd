ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib ebtiny ebnetd ndtpd ebhttpd doc doc-ja m4

pkglocalstatedir = $(localstatedir)/$(PACKAGE)

sysconf_DATA = ebnetd.conf.sample
pkglocalstate_DATA =

EXTRA_DIST = INSTALL-ja README-ja UPGRADE UPGRADE-ja \
	ebnetd.conf.sample.in move-if-change
CLEANFILES = ebnetd.conf.sample ebnetd.conf.sample.tmp

ebnetd.conf.sample: ebnetd.conf.sample.in Makefile
	rm -f ebnetd.conf.sample
	sed -e 's;\@pkglocalstatedir\@;$(pkglocalstatedir);' \
	   -e 's;\@datadir\@;$(datadir);' \
	   $(srcdir)/ebnetd.conf.sample.in > ebnetd.conf.sample.tmp
	cp ebnetd.conf.sample.tmp ebnetd.conf.sample
	rm -f ebnetd.conf.sample.tmp
