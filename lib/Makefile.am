#
# Copyright (c) 1998, 1999, 2001 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@suse.de>
#
# Process this file with automake to produce Makefile.in.
#
AUTOMAKE_OPTIONS = 1.4 gnits
MAINT_CHARSET = latin1

noinst_LIBRARIES = libyptools.a

AM_CFLAGS = @WARNFLAGS@ @DEFS@ @CFLAGS@

EXTRA_DIST = getopt.c getopt1.c getline.c ypbinderr_string.c

noinst_HEADERS = getopt.h getline.h yp-tools.h nicknames.h yp_all_host.h

libyptools_a_SOURCES = nicknames.c yp_xdr.c yp_all_host.c
libyptools_a_LIBADD = @LIBOBJS@

localedir = $(datadir)/locale
INCLUDES = -I../intl -DLOCALEDIR=\"$(localedir)\"

CLEANFILES = *~
