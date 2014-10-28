/* Copyright (C) 2014 Thorsten Kukuk
   Author: Thorsten Kukuk <kukuk@suse.de>

   This library is free software: you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   in version 2.1 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rpcsvc/yp_prot.h>

void
__ypbind3_binding_free (struct ypbind3_binding *ypb)
{
  if (ypb == NULL)
    return;
  /* netdir_free ((void *)ypb->ypbind_svcaddr, ND_ADDR); */
  if (ypb->ypbind_svcaddr->buf)
    free (ypb->ypbind_svcaddr->buf);
  free (ypb->ypbind_svcaddr);
  free (ypb->ypbind_servername);
  freenetconfigent (ypb->ypbind_nconf);
  free (ypb);
}
