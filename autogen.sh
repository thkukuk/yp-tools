#!/bin/sh -x

aclocal -I m4
autoheader
automake --add-missing --copy --force
autoreconf
chmod 755 configure
