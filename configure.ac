dnl Process this file with autoconf to produce a configure script.
AC_INIT(yp-tools, 2.99.5, kukuk@linux-nis.org)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/ypcat.c])
AM_CONFIG_HEADER(config.h)
AC_PREFIX_DEFAULT(/usr)

AC_SUBST(PACKAGE)
AC_SUBST(VERSION)

dnl Change libexec to lib/yp
if test ${libexecdir} = '${exec_prefix}/libexec'
then
  libexecdir='${exec_prefix}/lib/yp'
fi

dnl Should we install domainname ? default yes
AC_ARG_ENABLE(domainname,
        [  --disable-domainname    Don't install domainname [default=yes]],
              INSTALL_DOMAINNAME=$enableval, INSTALL_DOMAINNAME=yes)
if test "$INSTALL_DOMAINNAME" = "no"
then
   DOMAINNAME=
   MAN_DOMAINNAME=
else
   DOMAINNAME=domainname
   MAN_DOMAINNAME="domainname.8 nisdomainname.8 ypdomainname.8"
fi
AC_SUBST(DOMAINNAME)
AC_SUBST(MAN_DOMAINNAME)

dnl Should we use the CRACKLIB ? default no
AC_ARG_ENABLE(cracklib,
        [  --enable-cracklib       Use the cracklib for yppasswd [default=no]],
        USE_CRACKLIB=1
        AC_DEFINE(USE_CRACKLIB,1,[Define if you want to use libcrack to check the new passwords]))

dnl Should we do extra checks even if we use the CRACKLIB ? default no
AC_ARG_ENABLE(cracklib-strict,
        [  --enable-cracklib-strict Do some extra tests before accepting [default=no]],
        USE_CRACKLIB=1;
        USE_CRACKLIB_STRICT=1;
        AC_DEFINE(USE_CRACKLIB,1,[Define if you want to use libcrack to check the new passwords])
        AC_DEFINE(USE_CRACKLIB_STRICT,1,[Define if you want to use some extra checks besides the cracklib]))

dnl Should passwd be used instead of yppasswd ? Default is no.
AC_ARG_ENABLE(call-passwd,
        [  --enable-call-passwd  yppasswd calls passwd, chfn or chsh [default=no]],
	YPPASSWD_IS_DEPRECATED=1
        AC_DEFINE(YPPASSWD_IS_DEPRECATED,1,[Define if yppasswd is deprecated and passwd should be used instead]))


dnl Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE
AM_PROG_CC_C_O
dnl If we're using gcc, we want warning flags
dnl test -n "$GCC" && WARNFLAGS="-Wall"
test -n "$GCC" && WARNFLAGS="-W -Wall -Wbad-function-cast -Wcast-align -Winline -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wshadow -Wstrict-prototypes -Wundef"
AC_SUBST(WARNFLAGS)
AC_PROG_CPP
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL
LT_INIT

if test "${YPPASSWD_IS_DEPRECATED}" = "1"; then
  AC_PATH_PROG([PASSWD_PROG], [passwd], [/usr/bin/passwd])
  AC_PATH_PROG([CHFN_PROG], [chfn], [/usr/bin/chfn])
  AC_PATH_PROG([CHSH_PROG], [chsh], [/usr/bin/chsh])
fi

dnl Checks for libraries.
PKG_CHECK_MODULES([TIRPC], [libtirpc])
AC_CHECK_LIB(crypt,crypt,LIBCRYPT="-lcrypt",LIBCRYPT="")
AC_SUBST(LIBCRYPT)

dnl Check for libcrack
if test "${USE_CRACKLIB}" = "1"; then
  AC_CHECK_LIB(crack, FascistCheck, LIBCRACK="-lcrack",LIBCRACK="")
  AC_SUBST(LIBCRACK)
fi

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(getopt.h rpc/clnt_soc.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl AC_REPLACE_FUNCS(ypbinderr_string)

dnl internationalization macros
AM_GNU_GETTEXT_VERSION([0.19.2])
AM_GNU_GETTEXT([external])

AC_OUTPUT([Makefile lib/Makefile src/Makefile man/Makefile
	   etc/Makefile po/Makefile.in nss_nis6/Makefile libnis.pc
	   man/domainname.8 man/nicknames.5
	   man/ypcat.1 man/ypmatch.1 man/yppasswd.1 man/yppoll.8
	   man/ypset.8 man/yptest.8 man/ypwhich.1])
