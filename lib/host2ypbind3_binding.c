/* Copyright (C) 2014, 2016 Thorsten Kukuk
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

#if defined(HAVE_YPBIND3)

#include <rpcsvc/yp_prot.h>
#include "internal.h"

struct ypbind3_binding *
__host2ypbind3_binding (const char *host)
{
  const struct timeval TIMEOUT10 = {1, 0};
  CLIENT *server;
  ypbind3_binding ypb3, *res;
  struct netconfig *nconf;
  struct netbuf nbuf;

  /* connect to server to find out if it exist and runs */
  if ((server = clnt_create_timed (host, YPPROG, YPVERS,
				   "datagram_n", &TIMEOUT10)) == NULL)
    return NULL;

  /* get nconf, netbuf structures */
  nconf = getnetconfigent (server->cl_netid);
  clnt_control(server, CLGET_SVC_ADDR, (char *)&nbuf);

  ypb3.ypbind_nconf = nconf;
  ypb3.ypbind_svcaddr = (struct netbuf *)(&nbuf);
  ypb3.ypbind_servername = (char *)host;
  ypb3.ypbind_hi_vers = YPVERS;
  ypb3.ypbind_lo_vers = YPVERS;

  res = __ypbind3_binding_dup (&ypb3);

  freenetconfigent (nconf);

  clnt_destroy (server);

  return res;
}

#endif
