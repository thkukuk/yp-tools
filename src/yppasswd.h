/* Copyright (C) 1998 Thorsten Kukuk
   This file is part of the yp-tools.
   Author: Thorsten Kukuk <kukuk@vt.uni-paderborn.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef __YPPASSWD_H__
#define __YPPASSWD_H__

#include <rpc/rpc.h>

struct xpasswd {
  char *pw_name;
  char *pw_passwd;
  int pw_uid;
  int pw_gid;
  char *pw_gecos;
  char *pw_dir;
  char *pw_shell;
};
typedef struct xpasswd xpasswd;

struct yppasswd {
  char *oldpass;
  xpasswd newpw;
};
typedef struct yppasswd yppasswd;

#define YPPASSWDPROG ((u_long)100009)
#define YPPASSWDVERS ((u_long)1)

#define YPPASSWDPROC_UPDATE ((u_long)1)

extern  bool_t xdr_xpasswd (XDR *, xpasswd*);
extern  bool_t xdr_yppasswd (XDR *, yppasswd*);

#endif /* !__YPPASSWD_H__ */
