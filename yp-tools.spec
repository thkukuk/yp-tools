Summary: NIS (YP) clients
Name: yp-tools
Version: 2.3
Release: 1
Copyright: GPL
Group: Networking/Utilities
Source: ftp.kernel.org:/pub/linux/utils/net/NIS/yp-tools-%{PACKAGE_VERSION}.tar.gz
Packager: Thorsten Kukuk <kukuk@suse.de>
URL: http://www.suse.de/~kukuk/linux/nis.html
BuildRoot: /var/tmp/yp-tools
Conflicts: yppasswd

%description
This is an implementation of NIS _clients_ for Linux and works with every
Linux C Library.

This implementation only provides NIS _clients_. You must already have
a ypbind daemon running on the same host, and a NIS server running somewhere
in the net. You can find both for linux on
http://www.suse.de/~kukuk/linux/nis.html. Please read the NIS-HOWTO, too.

%prep
%setup

%clean
make distclean
rm -rf $RPM_BUILD_ROOT

%build
./configure
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR="$RPM_BUILD_ROOT" install

%files
%doc AUTHORS COPYING ChangeLog NEWS README THANKS TODO
%doc etc/nsswitch.conf
/bin/domainname
/bin/nisdomainname
/bin/ypdomainname
/usr/bin/ypcat
/usr/bin/ypchfn
/usr/bin/ypchsh
/usr/bin/ypmatch
/usr/bin/yppasswd
/usr/bin/ypwhich
/usr/man/man1/ypcat.1
/usr/man/man1/ypchfn.1
/usr/man/man1/ypchsh.1
/usr/man/man1/ypmatch.1
/usr/man/man1/yppasswd.1
/usr/man/man1/ypwhich.1
/usr/man/man8/domainname.8
/usr/man/man8/nisdomainname.8
/usr/man/man8/ypdomianname.8
/usr/man/man8/yppoll.8
/usr/man/man8/ypset.8
/usr/sbin/yppoll
/usr/sbin/ypset
/usr/share/locale/de/LC_MESSAGES/yp-tools.mo
/var/yp/nicknames
