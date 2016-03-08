/* Copyright (C) 1998, 1999, 2001, 2016 Thorsten Kukuk
   This file is part of the yp-tools.
   Author: Thorsten Kukuk <kukuk@suse.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rpcsvc/yppasswd.h>

bool_t
xdr_passwd (XDR *xdrs, struct passwd *objp)
{
  if (!xdr_string (xdrs, &objp->pw_name, ~0))
    return FALSE;
  if (!xdr_string (xdrs, &objp->pw_passwd, ~0))
    return FALSE;
  if (!xdr_int32_t (xdrs, &objp->pw_uid))
    return FALSE;
  if (!xdr_int32_t (xdrs, &objp->pw_gid))
    return FALSE;
  if (!xdr_string (xdrs, &objp->pw_gecos, ~0))
    return FALSE;
  if (!xdr_string (xdrs, &objp->pw_dir, ~0))
    return FALSE;
  if (!xdr_string (xdrs, &objp->pw_shell, ~0))
    return FALSE;
  return TRUE;
}

bool_t
xdr_yppasswd (XDR *xdrs, yppasswd *objp)
{
  if (!xdr_string (xdrs, &objp->oldpass, ~0))
    return FALSE;
  if (!xdr_passwd (xdrs, &objp->newpw))
    return FALSE;
  return TRUE;
}
