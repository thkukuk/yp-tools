/* Copyright (C) 2001, 2003, 2014, 2015 Thorsten Kukuk
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

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include <arpa/inet.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include "yp_all_host.h"

static struct timeval RPCTIMEOUT = {10, 0};

static void *ypall_data;
static int (*ypall_foreach) (int status, char *key, int keylen,
                             char *val, int vallen, char *data);

static bool_t
__xdr_ypresp_all (XDR *xdrs, u_long *objp)
{
  while (1)
    {
      struct ypresp_all resp;

      memset (&resp, '\0', sizeof (struct ypresp_all));
      if (!xdr_ypresp_all (xdrs, &resp))
        {
          xdr_free ((xdrproc_t) xdr_ypresp_all, (char *) &resp);
          *objp = YP_YPERR;
          return FALSE;
        }
      if (resp.more == 0)
        {
          xdr_free ((xdrproc_t) xdr_ypresp_all, (char *) &resp);
          *objp = YP_NOMORE;
          return TRUE;
        }

      switch (resp.ypresp_all_u.val.status)
        {
        case YP_TRUE:
          {
            char key[resp.ypresp_all_u.val.keydat.keydat_len + 1];
            char val[resp.ypresp_all_u.val.valdat.valdat_len + 1];
            int keylen = resp.ypresp_all_u.val.keydat.keydat_len;
            int vallen = resp.ypresp_all_u.val.valdat.valdat_len;

            /* We are not allowed to modify the key and val data.
               But we are allowed to add data behind the buffer,
               if we don't modify the length. So add an extra NUL
               character to avoid trouble with broken code. */
            *objp = YP_TRUE;
            memcpy (key, resp.ypresp_all_u.val.keydat.keydat_val, keylen);
            key[keylen] = '\0';
            memcpy (val, resp.ypresp_all_u.val.valdat.valdat_val, vallen);
            val[vallen] = '\0';
            xdr_free ((xdrproc_t) xdr_ypresp_all, (char *) &resp);
            if ((*ypall_foreach) (*objp, key, keylen,
                                  val, vallen, ypall_data))
              return TRUE;
          }
          break;
        default:
          *objp = resp.ypresp_all_u.val.status;
          xdr_free ((xdrproc_t) xdr_ypresp_all, (char *) &resp);
          /* Sun says we don't need to make this call, but must return
             immediatly. Since Solaris makes this call, we will call
             the callback function, too. */
          (*ypall_foreach) (*objp, NULL, 0, NULL, 0, ypall_data);
          return TRUE;
        }
    }
}

int
yp_all_host (const char *indomain, const char *inmap,
	     const struct ypall_callback *incallback, const char *hostname)
{
  struct ypreq_nokey req;
  int res;
  enum clnt_stat result;
  CLIENT *clnt;
  long status = 0;

  if (hostname == NULL || hostname[0] == '\0' ||
      indomain == NULL || indomain[0] == '\0' ||
      inmap == NULL || inmap[0] == '\0')
    return YPERR_BADARGS;

  res = YPERR_YPERR;

  clnt = clnt_create_timed (hostname, YPPROG, YPVERS, "tcp", NULL);
  if (clnt == NULL)
    return YPERR_PMAP;

  req.domain = (char *) indomain;
  req.map = (char *) inmap;

  ypall_foreach = incallback->foreach;
  ypall_data = (void *) incallback->data;

  result = clnt_call (clnt, YPPROC_ALL, (xdrproc_t) xdr_ypreq_nokey,
		      (caddr_t) &req, (xdrproc_t) __xdr_ypresp_all,
		      (caddr_t) &status, RPCTIMEOUT);

  if (result != RPC_SUCCESS)
    res = YPERR_RPC;
  else
    res = YPERR_SUCCESS;

  clnt_destroy (clnt);

  if (res == YPERR_SUCCESS && status != YP_NOMORE)
    return ypprot_err (status);

  return res;
}
