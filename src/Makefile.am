#
# Copyright (c) 1997, 1998, 1999, 2001, 2002, 2006 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@thkukuk.de>
#
AM_CFLAGS = @WARNFLAGS@
LDADD = @LIBINTL@ ../lib/libyptools.a

localedir = $(datadir)/locale
INCLUDES = -I$(srcdir) -I$(top_srcdir) -DLOCALEDIR=\"$(localedir)\"

CLEANFILES = *~ domainname

EXTRA_PROGRAMS = domainname
bin_PROGRAMS = ypwhich ypmatch ypcat yppasswd @DOMAINNAME@
sbin_PROGRAMS = yppoll ypset yptest

domainname_LDADD = @LIBINTL@

yppasswd_SOURCES = yppasswd.c yppasswd_xdr.c yppasswd.h
yppasswd_LDADD = ${LDADD} @LIBCRYPT@ @LIBCRACK@
yppasswd_CFLAGS = -DPASSWD_PROG=\"${PASSWD_PROG}\" \
	-DCHFN_PROG=\"${CHFN_PROG}\" -DCHSH_PROG=\"${CHSH_PROG}\"

install-exec-hook:
	ln -f ${DESTDIR}${bindir}/yppasswd ${DESTDIR}${bindir}/ypchsh
	ln -f ${DESTDIR}${bindir}/yppasswd ${DESTDIR}${bindir}/ypchfn
	-test "x$(DOMAINNAME)" = "xdomainname" && ln -f ${DESTDIR}${bindir}/domainname ${DESTDIR}${bindir}/ypdomainname
	-test "x$(DOMAINNAME)" = "xdomainname" && ln -f ${DESTDIR}${bindir}/domainname ${DESTDIR}${bindir}/nisdomainname
