#
# Copyright (c) 1997, 1998, 1999, 2001, 2002 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@suse.de>
#
AUTOMAKE_OPTIONS = 1.7 gnits
MAINT_CHARSET = latin1
#
INSTALL_PROGRAM = @INSTALL_PROGRAM@ -s -m 555

AM_CFLAGS = @WARNFLAGS@
LDADD = @LIBINTL@ ../lib/libyptools.a

localedir = $(datadir)/locale
INCLUDES = -I$(srcdir) -I$(top_srcdir) -DLOCALEDIR=\"$(localedir)\"

CLEANFILES = *~ domainname

rootexecbindir = @rootbindir@

EXTRA_PROGRAMS = domainname
rootexecbin_PROGRAMS = @DOMAINNAME@
bin_PROGRAMS = ypwhich ypmatch ypcat yppasswd
sbin_PROGRAMS = yppoll ypset yptest ypserv_test

domainname_LDADD = @LIBINTL@

ypserv_test_LDADD = -lpthread

yppasswd_SOURCES = yppasswd.c yppasswd_xdr.c yppasswd.h
yppasswd_LDADD = ${LDADD} @LIBCRYPT@ @LIBCRACK@
yppasswd_CFLAGS = -DPASSWD_PROG=\"${PASSWD_PROG}\" \
	-DCHFN_PROG=\"${CHFN_PROG}\" -DCHSH_PROG=\"${CHSH_PROG}\"

install-exec-hook:
	ln -f ${DESTDIR}${bindir}/yppasswd ${DESTDIR}${bindir}/ypchsh
	ln -f ${DESTDIR}${bindir}/yppasswd ${DESTDIR}${bindir}/ypchfn
	-test "x$(DOMAINNAME)" = "xdomainname" && ln -f ${DESTDIR}${rootbindir}/domainname ${DESTDIR}${rootbindir}/ypdomainname
	-test "x$(DOMAINNAME)" = "xdomainname" && ln -f ${DESTDIR}${rootbindir}/domainname ${DESTDIR}${rootbindir}/nisdomainname
