#
# Copyright (c) 1997, 1998, 1999, 2001, 2002, 2006, 2014 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@thkukuk.de>
#

localedir = $(datadir)/locale

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(srcdir) -I$(top_srcdir) \
	-DLOCALEDIR=\"$(localedir)\"
AM_CFLAGS = @WARNFLAGS@ @NSL_CFLAGS@ @TIRPC_CFLAGS@
LDADD = @NSL_LIBS@ @TIRPC_LIBS@


CLEANFILES = *~ domainname

EXTRA_PROGRAMS = domainname
bin_PROGRAMS = ypwhich ypmatch ypcat yppasswd @DOMAINNAME@
sbin_PROGRAMS = yppoll ypset yp_dump_binding yptest

yppasswd_SOURCES = yppasswd.c yppasswd_xdr.c
yppasswd_LDADD = ${LDADD} @LIBCRYPT@ @LIBCRACK@
yppasswd_CFLAGS = ${AM_CFLAGS} -DPASSWD_PROG=\"${PASSWD_PROG}\" \
	-DCHFN_PROG=\"${CHFN_PROG}\" -DCHSH_PROG=\"${CHSH_PROG}\"
ypcat_LDADD = ../lib/libyptools.a ${LDADD}
ypset_LDADD = ../lib/libyptools.a ${LDADD}
ypmatch_LDADD = ../lib/libyptools.a ${LDADD}
ypwhich_LDADD = ../lib/libyptools.a ${LDADD}
yptest_LDADD = ../lib/yp_all_host.o ${LDADD}

install-exec-hook:
	ln -f ${DESTDIR}${bindir}/yppasswd ${DESTDIR}${bindir}/ypchsh
	ln -f ${DESTDIR}${bindir}/yppasswd ${DESTDIR}${bindir}/ypchfn
	-test "x$(DOMAINNAME)" = "xdomainname" && ln -f ${DESTDIR}${bindir}/domainname ${DESTDIR}${bindir}/ypdomainname
	-test "x$(DOMAINNAME)" = "xdomainname" && ln -f ${DESTDIR}${bindir}/domainname ${DESTDIR}${bindir}/nisdomainname
