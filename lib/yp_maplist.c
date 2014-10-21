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

#include <rpcsvc/yp.h>
#include <rpcsvc/ypclnt.h>

int
yp_maplist (const char *indomain, struct ypmaplist **outmaplist)
{
# if 0 /* XXX */
  struct ypresp_maplist resp;
  enum clnt_stat result;
  
  if (indomain == NULL || indomain[0] == '\0')
    return YPERR_BADARGS;
  
  memset (&resp, '\0', sizeof (resp));
  
  result = do_ypcall_tr (indomain, YPPROC_MAPLIST, (xdrproc_t) xdr_domainname,
                         (caddr_t) &indomain, (xdrproc_t) xdr_ypresp_maplist,
                         (caddr_t) &resp);
  
  if (result == YPERR_SUCCESS)
    {
      *outmaplist = resp.maps;
      /* We don't free the list, this will be done by ypserv
         xdr_free((xdrproc_t)xdr_ypresp_maplist, (char *)&resp); */
    }

  return result;
#else
  return -1;
#endif
}
