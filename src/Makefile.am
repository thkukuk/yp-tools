#
# Copyright (c) 1997, 1998, 1999 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@suse.de>
#
AUTOMAKE_OPTIONS = 1.4 gnits
MAINT_CHARSET = latin1
#
INSTALL_PROGRAM = @INSTALL_PROGRAM@ -s -m 555

CFLAGS = @WARNFLAGS@ @DEFS@ @CFLAGS@
LDADD = @INTLLIBS@ ../lib/libyptools.a

localedir = $(datadir)/locale
INCLUDES = -I$(srcdir) -I$(top_srcdir) -I../intl -DLOCALEDIR=\"$(localedir)\"

CLEANFILES = *~ domainname

rootexecbindir = @rootbindir@

EXTRA_PROGRAMS = domainname
rootexecbin_PROGRAMS = @DOMAINNAME@
bin_PROGRAMS = ypwhich ypmatch ypcat yppasswd
sbin_PROGRAMS = yppoll ypset

domainname_LDADD := @INTLLIBS@

yppasswd_SOURCES = yppasswd.c yppasswd_xdr.c yppasswd.h
yppasswd_LDADD = ${LDADD} @LIBCRYPT@

install-exec-hook:
	ln -f ${DESTDIR}${bindir}/yppasswd ${DESTDIR}${bindir}/ypchsh
	ln -f ${DESTDIR}${bindir}/yppasswd ${DESTDIR}${bindir}/ypchfn
	-test "x$(DOMAINNAME)" = "xdomainname" && ln -f ${DESTDIR}${rootbindir}/domainname ${DESTDIR}${rootbindir}/ypdomainname
	-test "x$(DOMAINNAME)" = "xdomainname" && ln -f ${DESTDIR}${rootbindir}/domainname ${DESTDIR}${rootbindir}/nisdomainname
