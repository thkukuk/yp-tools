/* Copyright (C) 2014 Thorsten Kukuk
   Author: Thorsten Kukuk <kukuk@suse.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <libintl.h>
#include <rpcsvc/yp.h>
#include <rpcsvc/ypclnt.h>

#ifndef _
#define _(String) gettext (String)
#endif

#ifndef N_
#define N_(String) String
#endif


const char *
ypbinderr_string (const int error)
{
  const char *str;
  switch (error)
    {
    case 0:
      str = N_("Success");
      break;
    case YPBIND_ERR_ERR:
      str = N_("Internal ypbind error");
      break;
    case YPBIND_ERR_NOSERV:
      str = N_("Domain not bound");
      break;
    case YPBIND_ERR_RESC:
      str = N_("System resource allocation failure");
      break;
    default:
      str = N_("Unknown ypbind error");
      break;
    }
  return _(str);
}
