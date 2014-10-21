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

#include <rpc/clnt.h>
#include <rpcsvc/yp.h>
#include <rpcsvc/ypclnt.h>

int
yp_master (const char *indomain, const char *inmap, char **outname)
{
  ypreq_nokey req;
  ypresp_master resp;
  enum clnt_stat result;

  if (indomain == NULL || indomain[0] == '\0' ||
      inmap == NULL || inmap[0] == '\0')
    return YPERR_BADARGS;

  req.domain = (char *) indomain;
  req.map = (char *) inmap;

  memset (&resp, '\0', sizeof (ypresp_master));

#if 0 /* XXX */
  result = do_ypcall_tr (indomain, YPPROC_MASTER, (xdrproc_t) xdr_ypreq_nokey,
                         (caddr_t) &req, (xdrproc_t) xdr_ypresp_master,
                         (caddr_t) &resp);
#else
  result = rpc_call ("ipv6-localhost", YPBINDPROG, YPBINDVERS, YPPROC_MASTER,
		     (xdrproc_t) xdr_ypreq_nokey,
		     (xdrproc_t) xdr_ypresp_master, 
		     (caddr_t) &req, &resp, "tcp");
#endif
  if (result != YPERR_SUCCESS)
    return result;

  *outname = strdup (resp.peer);
  xdr_free ((xdrproc_t) xdr_ypresp_master, (char *) &resp);

  return *outname == NULL ? YPERR_YPERR : YPERR_SUCCESS;
}
