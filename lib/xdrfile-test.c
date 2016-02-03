/* Copyright (C) 2014 Thorsten Kukuk
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include "internal.h"

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static void
dump_nconf (struct netconfig *nconf, char *prefix)
{
  printf ("%snc_netid: %s\n", prefix, nconf->nc_netid);
  printf ("%snc_semantics: %lu\n", prefix, nconf->nc_semantics);
  printf ("%snc_flag: %lu\n", prefix, nconf->nc_flag);
  printf ("%snc_protofmly: '%s'\n", prefix, nconf->nc_protofmly);
  printf ("%snc_proto: '%s'\n", prefix, nconf->nc_proto);
  printf ("%snc_device: '%s'\n", prefix, nconf->nc_device);
  printf ("%snc_nlookups: %lu\n", prefix, nconf->nc_nlookups);
}

static void
ypbind3_binding_dump (struct ypbind3_binding *ypb3)
{
  char buf[INET6_ADDRSTRLEN];

  printf ("ypbind_nconf:\n");
  if (ypb3->ypbind_nconf)
    dump_nconf (ypb3->ypbind_nconf, "\t");
  else
    printf ("\tNULL\n");

  printf ("ypbind_svcaddr: %s:%i\n",
          taddr2ipstr (ypb3->ypbind_nconf, ypb3->ypbind_svcaddr,
                       buf, sizeof (buf)),
	  taddr2port (ypb3->ypbind_nconf, ypb3->ypbind_svcaddr));

  printf ("ypbind_servername: ");
  if (ypb3->ypbind_servername)
    printf ("%s\n", ypb3->ypbind_servername);
  else
    printf ("NULL\n");
  printf ("ypbind_hi_vers: %lu\n", (u_long) ypb3->ypbind_hi_vers);
  printf ("ypbind_lo_vers: %lu\n", (u_long) ypb3->ypbind_lo_vers);
}

static void
write_ypbind3_binding (struct ypbind3_binding *ypb3)
{
  char path3[MAXPATHLEN + 1];
  FILE *fp;

  sprintf (path3, "binding.3");

  if ((fp = fopen (path3, "wce")) == NULL)
    printf ("fopen (%s): %s", path3, strerror (errno));
  else
    {
      XDR xdrs;
      bool_t status;

      xdrstdio_create (&xdrs, fp, XDR_ENCODE);
      status = xdr_ypbind3_binding (&xdrs, ypb3);
      if (!status)
	{
	  printf ("write of %s failed!", path3);
	  unlink (path3);
	}
      xdr_destroy (&xdrs);
      fclose (fp);
    }
}

static void
read_ypbind3_binding (struct ypbind3_binding *ypb3)
{
  char path[100];

  snprintf (path, sizeof (path), "binding.3");

  FILE *in = fopen (path, "rce");
  if (in != NULL)
    {
      bool_t status;

      XDR xdrs;
      xdrstdio_create (&xdrs, in, XDR_DECODE);
      memset(ypb3, 0, sizeof (struct ypbind3_binding));
      status = xdr_ypbind3_binding (&xdrs, ypb3);
      if (!status)
        {
          printf ("read of %s failed!", path);
          unlink (path);
        }
      xdr_destroy (&xdrs);
    }
}

int
main (void)
{
   struct ypbind3_binding *ypb3, res;

   ypb3 = __host2ypbind3_binding ("phex");
   ypbind3_binding_dump (ypb3);
   write_ypbind3_binding (ypb3);

   printf ("\nRead data\n\n");

   read_ypbind3_binding (&res);
   ypbind3_binding_dump (&res);

   return 0;
}
