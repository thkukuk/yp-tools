#
# Copyright (c) 1998, 1999, 2001, 2006, 2014 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@thkukuk.de>
#
# Process this file with automake to produce Makefile.in.
#

CLEANFILES = *~

localedir = $(datadir)/locale

AM_CFLAGS = @WARNFLAGS@ -D_REENTRANT=1
AM_CPPFLAGS = -I$(srcdir) @TIRPC_CFLAGS@ @NSL_CFLAGS@ -DLOCALEDIR=\"$(localedir)\"

noinst_HEADERS = nicknames.h yp_all_host.h internal.h

noinst_LIBRARIES = libyptools.a

libyptools_a_SOURCES = nicknames.c yp_all_host.c ypbind3_binding_dup.c ypbind3_binding_free.c host2ypbind3_binding.c

check_PROGRAMS=xdrfile-test
xdrfile_test_LDADD = libyptools.a @NSL_LIBS@ @TIRPC_LIBS@
