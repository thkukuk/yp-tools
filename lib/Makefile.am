#
# Copyright (c) 1998, 1999, 2001, 2006, 2014 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@thkukuk.de>
#
# Process this file with automake to produce Makefile.in.
#
noinst_LIBRARIES = libyptools.a

AM_CFLAGS = @WARNFLAGS@

noinst_HEADERS = nicknames.h yp_all_host.h

libyptools_a_SOURCES = nicknames.c yp_xdr.c yp_all_host.c \
	do_ypcall.c yp_master.c yp_maplist.c ypprot_err.c \
	yperr_string.c ypbinderr_string.c yp_get_default_domain.c

localedir = $(datadir)/locale
AM_CPPFLAGS = -I. @TIRPC_CFLAGS@ -DLOCALEDIR=\"$(localedir)\"

CLEANFILES = *~
