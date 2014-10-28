#
# Copyright (c) 2014 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@thkukuk.de>
#
# Process this file with automake to produce Makefile.in.
#

CLEANFILES = *~

EXTRA_DIST = libnss_nis6.map

AM_CFLAGS = @WARNFLAGS@ -D_REENTRANT=1
AM_CPPFLAGS = -I$(top_srcdir)/lib @TIRPC_CFLAGS@

lib_LTLIBRARIES = libnss_nis6.la

libnss_nis6_la_LDFLAGS = -version-info 2:0:0 \
	-Wl,--version-script=$(srcdir)/libnss_nis.map
libnss_nis6_la_LIBADD = ../lib/libnis.la
libnss_nis6_la_SOURCES = nis-alias.c nis-ethers.c nis-grp.c nis-hosts.c \
			nis-initgroups.c nis-netgrp.c nis-network.c  \
			nis-proto.c nis-publickey.c nis-pwd.c nis-rpc.c \
			nis-service.c nis-spwd.c

