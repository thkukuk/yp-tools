/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include "yp-tools.h"

bool_t
ytxdr_ypstat (XDR *xdrs, enum ypstat *objp)
{
  if (!xdr_enum(xdrs, (enum_t *)objp))
    return (FALSE);
  return (TRUE);
}

bool_t
ytxdr_domainname(XDR *xdrs, char **objp)
{
  if (!xdr_string(xdrs, objp, YPMAXDOMAIN)) {
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_mapname(XDR *xdrs, char **objp)
{
  if (!xdr_string(xdrs, objp, YPMAXMAP)) {
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_peername(XDR *xdrs, char **objp)
{
  if (!xdr_string(xdrs, objp, YPMAXPEER)) {
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_ypreq_nokey(XDR *xdrs, struct ypreq_nokey *objp)
{
  if (!ytxdr_domainname(xdrs, &objp->domain)) {
    return (FALSE);
  }
  if (!ytxdr_mapname(xdrs, &objp->map)) {
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_ypresp_master(XDR *xdrs, struct ypresp_master *objp)
{
  if (!ytxdr_ypstat(xdrs, &objp->status)) {
    return (FALSE);
  }
  if (!ytxdr_peername(xdrs, &objp->master)) {
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_ypresp_order(XDR *xdrs, struct ypresp_order *objp)
{
  if (!ytxdr_ypstat(xdrs, &objp->status)) {
    return (FALSE);
  }
  if (!xdr_u_int(xdrs, &objp->ordernum)) {
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_ypbind_resptype(XDR *xdrs, enum ypbind_resptype *objp)
{
  if (!xdr_enum(xdrs, (enum_t *)objp)) {
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_ypbind_binding(XDR *xdrs, struct ypbind_binding *objp)
{
  if (!xdr_opaque(xdrs, (char *)&objp->ypbind_binding_addr, 4)) {
    return (FALSE);
  }
  if (!xdr_opaque(xdrs, (char *)&objp->ypbind_binding_port, 2)) {
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_ypbind_resp(XDR *xdrs, struct ypbind_resp *objp)
{
  if (!ytxdr_ypbind_resptype(xdrs, &objp->ypbind_status)) {
    return (FALSE);
  }
  switch (objp->ypbind_status) {
  case YPBIND_FAIL_VAL:
    if (!xdr_u_int(xdrs, &objp->ypbind_respbody.ypbind_error)) {
      return (FALSE);
		 }
    break;
  case YPBIND_SUCC_VAL:
    if (!ytxdr_ypbind_binding(xdrs, &objp->ypbind_respbody.ypbind_bindinfo)) {
      return (FALSE);
    }
    break;
  default:
    return (FALSE);
  }
  return (TRUE);
}

bool_t
ytxdr_ypbind_setdom(XDR *xdrs, struct ypbind_setdom *objp)
{
  if (!ytxdr_domainname(xdrs, &objp->ypsetdom_domain)) {
    return (FALSE);
  }
  if (!ytxdr_ypbind_binding(xdrs, &objp->ypsetdom_binding)) {
    return (FALSE);
  }
  if (!xdr_u_int(xdrs, &objp->ypsetdom_vers)) {
    return (FALSE);
  }
  return (TRUE);
}
