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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <libintl.h>
#include "yp-tools.h"

#ifndef _
#define _(String) gettext (String)
#endif

const char *
ypbinderr_string (const int error)
{
  switch (error)
    {
    case 0:
      return _("Success");
    case YPBIND_ERR_ERR:
      return _("Internal ypbind error");
    case YPBIND_ERR_NOSERV:
      return _("Domain not bound");
    case YPBIND_ERR_RESC:
      return _("System resource allocation failure");
    default:
      return _("Unknown ypbind error");
    }
}
