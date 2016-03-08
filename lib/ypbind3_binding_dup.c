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
__ypbind3_binding_dup (struct ypbind3_binding *src)
{
#define copy_str(source, dest) \
  if (source != NULL) \
    { \
      dest = strdup (source); \
      if (dest == NULL) \
        { \
          __ypbind3_binding_free (dst); \
          return NULL; \
        } \
    }

  struct ypbind3_binding *dst;
  unsigned long i;

  dst = calloc(1, sizeof (struct ypbind3_binding));
  if (dst == NULL)
    return NULL;

  dst->ypbind_nconf = calloc (1, sizeof (struct netconfig));
  if (dst->ypbind_nconf == NULL)
    {
      __ypbind3_binding_free (dst);
      return NULL;
    }
  dst->ypbind_svcaddr = calloc(1, sizeof (struct netbuf));
  if (dst->ypbind_svcaddr == NULL)
    {
      __ypbind3_binding_free (dst);
      return NULL;
    }
  dst->ypbind_hi_vers = src->ypbind_hi_vers;
  dst->ypbind_lo_vers = src->ypbind_lo_vers;
  if (src->ypbind_servername)
    dst->ypbind_servername =
      strdup(src->ypbind_servername);

  copy_str (src->ypbind_nconf->nc_netid, dst->ypbind_nconf->nc_netid);
  dst->ypbind_nconf->nc_semantics = src->ypbind_nconf->nc_semantics;
  dst->ypbind_nconf->nc_flag = src->ypbind_nconf->nc_flag;
  copy_str (src->ypbind_nconf->nc_protofmly, dst->ypbind_nconf->nc_protofmly);
  copy_str (src->ypbind_nconf->nc_proto, dst->ypbind_nconf->nc_proto);
  copy_str (src->ypbind_nconf->nc_device, dst->ypbind_nconf->nc_device);
  dst->ypbind_nconf->nc_nlookups = src->ypbind_nconf->nc_nlookups;

  dst->ypbind_nconf->nc_lookups = calloc (src->ypbind_nconf->nc_nlookups,
                                          sizeof (char *));
  if (dst->ypbind_nconf->nc_lookups == NULL)
    {
      __ypbind3_binding_free (dst);
      return NULL;
    }
  for (i = 0; i < src->ypbind_nconf->nc_nlookups; i++)
    dst->ypbind_nconf->nc_lookups[i] =
      src->ypbind_nconf->nc_lookups[i] ?
      strdup (src->ypbind_nconf->nc_lookups[i]) : NULL;

  for (i = 0; i < 8; i++)
    dst->ypbind_nconf->nc_unused[i] = src->ypbind_nconf->nc_unused[i];

  dst->ypbind_svcaddr->maxlen = src->ypbind_svcaddr->maxlen;
  dst->ypbind_svcaddr->len = src->ypbind_svcaddr->len;
  dst->ypbind_svcaddr->buf = malloc (src->ypbind_svcaddr->maxlen);
  if (dst->ypbind_svcaddr->buf == NULL)
    {
      __ypbind3_binding_free (dst);
      return NULL;
    }
  memcpy (dst->ypbind_svcaddr->buf, src->ypbind_svcaddr->buf,
          dst->ypbind_svcaddr->len);

  return dst;
#undef copy_str
}

#endif
