/* Copyright (C) 1996-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Thorsten Kukuk <kukuk@suse.de>, 1996.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <netdb.h>
#include <nss.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpcsvc/ypclnt.h>

#include "nss-nis6.h"
#include "netgroup.h"

static char *
strip_whitespace (char *str)
{
  char *cp = str;

  /* Skip leading spaces.  */
  while (isspace (*cp))
    cp++;

  str = cp;
  while (*cp != '\0' && ! isspace(*cp))
    cp++;

  /* Null-terminate, stripping off any trailing spaces.  */
  *cp = '\0';

  return *str == '\0' ? NULL : str;
}

static enum nss_status
_nss_netgroup_parseline (char **cursor, struct __netgrent *result,
                         char *buffer, size_t buflen, int *errnop)
{
  enum nss_status status;
  const char *host, *user, *domain;
  char *cp = *cursor;

  /* Some sanity checks.  */
  if (cp == NULL)
    return NSS_STATUS_NOTFOUND;

  /* First skip leading spaces.  */
  while (isspace (*cp))
    ++cp;

  if (*cp != '(')
    {
      /* We have a list of other netgroups.  */
      char *name = cp;

      while (*cp != '\0' && ! isspace (*cp))
        ++cp;

      if (name != cp)
        {
          /* It is another netgroup name.  */
          int last = *cp == '\0';

          result->type = group_val;
          result->val.group = name;
          *cp = '\0';
          if (! last)
            ++cp;
          *cursor = cp;
          result->first = 0;

          return NSS_STATUS_SUCCESS;
        }

      return result->first ? NSS_STATUS_NOTFOUND : NSS_STATUS_RETURN;
    }

  /* Match host name.  */
  host = ++cp;
  while (*cp != ',')
    if (*cp++ == '\0')
      return result->first ? NSS_STATUS_NOTFOUND : NSS_STATUS_RETURN;

  /* Match user name.  */
  user = ++cp;
  while (*cp != ',')
    if (*cp++ == '\0')
      return result->first ? NSS_STATUS_NOTFOUND : NSS_STATUS_RETURN;

  /* Match domain name.  */
  domain = ++cp;
  while (*cp != ')')
    if (*cp++ == '\0')
      return result->first ? NSS_STATUS_NOTFOUND : NSS_STATUS_RETURN;
  ++cp;


  /* When we got here we have found an entry.  Before we can copy it
     to the private buffer we have to make sure it is big enough.  */
  if (cp - host > buflen)
    {
      *errnop = ERANGE;
      status = NSS_STATUS_TRYAGAIN;
    }
  else
    {
      memcpy (buffer, host, cp - host);
      result->type = triple_val;

      buffer[(user - host) - 1] = '\0'; /* Replace ',' with '\0'.  */
      result->val.triple.host = strip_whitespace (buffer);

      buffer[(domain - host) - 1] = '\0'; /* Replace ',' with '\0'.  */
      result->val.triple.user = strip_whitespace (buffer + (user - host));

      buffer[(cp - host) - 1] = '\0'; /* Replace ')' with '\0'.  */
      result->val.triple.domain = strip_whitespace (buffer + (domain - host));

      status = NSS_STATUS_SUCCESS;

      /* Remember where we stopped reading.  */
      *cursor = cp;

      result->first = 0;
    }

  return status;
}

static void
internal_nis6_endnetgrent (struct __netgrent *netgrp)
{
  free (netgrp->data);
  netgrp->data = NULL;
  netgrp->data_size = 0;
  netgrp->cursor = NULL;
}


enum nss_status
_nss_nis6_setnetgrent (const char *group, struct __netgrent *netgrp)
{
  int len;
  enum nss_status status;

  status = NSS_STATUS_SUCCESS;

  if (__glibc_unlikely (group == NULL || group[0] == '\0'))
    return NSS_STATUS_UNAVAIL;

  char *domain;
  if (__glibc_unlikely (yp_get_default_domain (&domain)))
    return NSS_STATUS_UNAVAIL;

  status = yperr2nss (yp_match (domain, "netgroup", group, strlen (group),
				&netgrp->data, &len));
  if (__glibc_likely (status == NSS_STATUS_SUCCESS))
    {
      /* Our implementation of yp_match already allocates a buffer
	 which is one byte larger than the value in LEN specifies
	 and the last byte is filled with NUL.  So we can simply
	 use that buffer.  */
      assert (len >= 0);
      assert (malloc_usable_size (netgrp->data) >= len + 1);
      assert (netgrp->data[len] == '\0');

      netgrp->data_size = len;
      netgrp->cursor = netgrp->data;
    }

  return status;
}


enum nss_status
_nss_nis6_endnetgrent (struct __netgrent *netgrp)
{
  internal_nis6_endnetgrent (netgrp);

  return NSS_STATUS_SUCCESS;
}


enum nss_status
_nss_nis6_getnetgrent_r (struct __netgrent *result, char *buffer, size_t buflen,
			int *errnop)
{
  return _nss_netgroup_parseline (&result->cursor, result, buffer, buflen,
				  errnop);
}
