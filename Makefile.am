#
# Copyright (c) 1997, 1998, 1999, 2001, 2006 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@thkukuk.de>
#
AUTOMAKE_OPTIONS = 1.5 gnits dist-bzip2
#
SUBDIRS = lib src po man etc

CLEANFILES = *~

ACLOCAL_AMFLAGS = -I m4

M4_FILES = m4/codeset.m4 m4/getline.m4 m4/gettext.m4 m4/glibc21.m4 \
	m4/glibc2.m4 m4/iconv.m4 m4/intdiv0.m4 m4/intmax.m4 \
	m4/inttypes_h.m4 m4/inttypes-h.m4 m4/inttypes.m4 \
	m4/inttypes-pri.m4 m4/isc-posix.m4 m4/lcmessage.m4 \
	m4/lib-ld.m4 m4/lib-link.m4 m4/lib-prefix.m4 m4/lock.m4 \
	m4/longdouble.m4 m4/longlong.m4 m4/nls.m4 m4/po.m4 \
	m4/printf-posix.m4 m4/progtest.m4 m4/signed.m4 m4/size_max.m4 \
	m4/stdint_h.m4 m4/uintmax_t.m4 m4/ulonglong.m4 m4/visibility.m4 \
	m4/wchar_t.m4 m4/wint_t.m4 m4/xsize.m4

EXTRA_DIST = config.rpath $(M4_FILES)
