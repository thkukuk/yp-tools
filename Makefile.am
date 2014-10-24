#
# Copyright (c) 1997, 1998, 1999, 2001, 2006, 2014 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@thkukuk.de>
#
AUTOMAKE_OPTIONS = 1.5 gnits dist-bzip2
#
SUBDIRS = lib src po man etc

CLEANFILES = *~

ACLOCAL_AMFLAGS = -I m4

M4_FILES = m4/getline.m4

EXTRA_DIST = config.rpath $(M4_FILES)

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA = libnis.pc
