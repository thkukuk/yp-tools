#
# Copyright (c) 1997, 1998, 1999 Thorsten Kukuk, Germany
#
# Author: Thorsten Kukuk <kukuk@suse.de>
#
AUTOMAKE_OPTIONS = 1.4 gnits
MAINT_CHARSET = latin1
#
CLEANFILES = *~ sedscript domainname.8 ypcat.1 ypmatch.1 yppasswd.1\
	yppoll.8 ypset.8 ypwhich.1 nicknames.5

EXTRA_DIST = sedscript.in domainname.8.in nisdomainname.8 ypdomainname.8\
	ypcat.1.in ypchfn.1 ypchsh.1 ypmatch.1.in yppasswd.1.in\
	yppoll.8.in ypset.8.in ypwhich.1.in nicknames.5.in

man_MANS = ypcat.1 ypchfn.1 ypchsh.1 ypmatch.1 yppasswd.1 yppoll.8 ypset.8\
	 ypwhich.1 nicknames.5 @MAN_DOMAINNAME@

domainname.8: domainname.8.in sedscript
	-rm -f domainname.8
	./sedscript < $(srcdir)/domainname.8.in > domainname.8

nicknames.5: nicknames.5.in sedscript
	-rm -f nicknames.5
	./sedscript < $(srcdir)/nicknames.5.in > nicknames.5

ypcat.1: ypcat.1.in sedscript
	-rm -f ypcat.1
	./sedscript < $(srcdir)/ypcat.1.in > ypcat.1

ypmatch.1: ypmatch.1.in sedscript
	-rm -f ypmatch.1
	./sedscript < $(srcdir)/ypmatch.1.in > ypmatch.1

yppasswd.1: yppasswd.1.in sedscript
	-rm -f yppasswd.1
	./sedscript < $(srcdir)/yppasswd.1.in > yppasswd.1

yppoll.8: yppoll.8.in sedscript
	-rm -f yppoll.8
	./sedscript < $(srcdir)/yppoll.8.in > yppoll.8

ypset.8: ypset.8.in sedscript
	-rm -f ypset.8
	./sedscript < $(srcdir)/ypset.8.in > ypset.8

ypwhich.1: ypwhich.1.in sedscript
	-rm -f ypwhich.1
	./sedscript < $(srcdir)/ypwhich.1.in > ypwhich.1
