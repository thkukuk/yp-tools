#
# Copyright (c) 1998, 1999, 2001, 2006, 2014 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@thkukuk.de>
#
# Process this file with automake to produce Makefile.in.
#

CLEANFILES = *~

EXTRA_DIST = libnis.map

localedir = $(datadir)/locale
rpcsvcdir = $(includedir)/rpcsvc

AM_CFLAGS = @WARNFLAGS@ -D_REENTRANT=1
AM_CPPFLAGS = -I$(srcdir) @TIRPC_CFLAGS@ -DLOCALEDIR=\"$(localedir)\"

rpcsvc_HEADERS = rpcsvc/ypclnt.h rpcsvc/yppasswd.x rpcsvc/ypupd.h \
	rpcsvc/yppasswd.h rpcsvc/yp_prot.h rpcsvc/yp.x

noinst_HEADERS = nicknames.h yp_all_host.h internal.h

noinst_LIBRARIES = libyptools.a
lib_LTLIBRARIES = libnis.la

libnis_la_LDFLAGS = -version-info 1:1:0 \
	-Wl,--version-script=$(srcdir)/libnis.map
libnis_la_LIBADD = @TIRPC_LIBS@

libyptools_a_SOURCES = nicknames.c yp_all_host.c

libnis_la_SOURCES = yp_xdr.c do_ypcall.c ypprot_err.c yp_master.c \
	yp_maplist.c yp_order.c yp_first.c yp_next.c \
	yp_match.c taddr2host.c taddr2ipstr.c taddr2port.c \
	host2ypbind3_binding.c ypbind3_binding_dup.c ypbind3_binding_free.c \
	yperr_string.c ypbinderr_string.c yp_get_default_domain.c

noinst_PROGRAMS=xdrfile-test
xdrfile_test_LDADD = libnis.la
