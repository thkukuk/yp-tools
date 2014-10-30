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

#define _GNU_SOURCE

#include <nss.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <netinet/ether.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>

#include "libc-symbols.h"
#include "libc-lock.h"
#include "nss-nis6.h"

/* Protect global state against multiple changers */
__libc_lock_define_initialized (static, lock)

/* Because the `ethers' lookup does not fit so well in the scheme we
   define a dummy struct here which helps us to use the available
   functions.  */
struct etherent
{
  const char *e_name;
  struct ether_addr e_addr;
};

#define ENTNAME         etherent
#define DATABASE        "ethers"
#include "files-parse.c"
LINE_PARSER
("#",
 /* Read the ethernet address: 6 x 8bit hexadecimal number.  */
 {
   size_t cnt;

   for (cnt = 0; cnt < 6; ++cnt)
     {
       unsigned int number;

       if (cnt < 5)
         INT_FIELD (number, ISCOLON , 0, 16, (unsigned int))
       else
         INT_FIELD (number, isspace, 1, 16, (unsigned int))

       if (number > 0xff)
         return 0;
       result->e_addr.ether_addr_octet[cnt] = number;
     }
 };
 STRING_FIELD (result->e_name, isspace, 1);
 )

struct response
{
  struct response *next;
  char val[0];
};

static struct response *start;
static struct response *next;

static int
saveit (int instatus, char *inkey, int inkeylen, char *inval,
	int invallen, char *indata)
{
  if (instatus != YP_TRUE)
    return 1;

  if (inkey && inkeylen > 0 && inval && invallen > 0)
    {
      struct response *newp = malloc (sizeof (struct response) + invallen + 1);
      if (newp == NULL)
	return 1; /* We have no error code for out of memory */

      if (start == NULL)
	start = newp;
      else
	next->next = newp;
      next = newp;

      newp->next = NULL;
      *((char *) mempcpy (newp->val, inval, invallen)) = '\0';
    }

  return 0;
}

static void
internal_nis6_endetherent (void)
{
  while (start != NULL)
    {
      next = start;
      start = start->next;
      free (next);
    }
}

enum nss_status
_nss_nis6_endetherent (void)
{
  __libc_lock_lock (lock);

  internal_nis6_endetherent ();
  next = NULL;

  __libc_lock_unlock (lock);

  return NSS_STATUS_SUCCESS;
}

static enum nss_status
internal_nis6_setetherent (void)
{
  char *domainname;
  struct ypall_callback ypcb;
  enum nss_status status;

  yp_get_default_domain (&domainname);

  internal_nis6_endetherent ();

  ypcb.foreach = saveit;
  ypcb.data = NULL;
  status = yperr2nss (yp_all (domainname, "ethers.byname", &ypcb));
  next = start;

  return status;
}

enum nss_status
_nss_nis6_setetherent (int stayopen)
{
  enum nss_status result;

  __libc_lock_lock (lock);

  result = internal_nis6_setetherent ();

  __libc_lock_unlock (lock);

  return result;
}

static enum nss_status
internal_nis6_getetherent_r (struct etherent *eth, char *buffer, size_t buflen,
			     int *errnop)
{
  struct parser_data *data = (void *) buffer;
  int parse_res;

  if (start == NULL)
    internal_nis6_setetherent ();

  /* Get the next entry until we found a correct one. */
  do
    {
      char *p;

      if (next == NULL)
	return NSS_STATUS_NOTFOUND;

      p = strncpy (buffer, next->val, buflen);

      while (isspace (*p))
        ++p;

      parse_res = _nss_files_parse_etherent (p, eth, data, buflen, errnop);
      if (parse_res == -1)
	return NSS_STATUS_TRYAGAIN;
      next = next->next;
    }
  while (!parse_res);

  return NSS_STATUS_SUCCESS;
}

enum nss_status
_nss_nis6_getetherent_r (struct etherent *result, char *buffer, size_t buflen,
			 int *errnop)
{
  int status;

  __libc_lock_lock (lock);

  status = internal_nis6_getetherent_r (result, buffer, buflen, errnop);

  __libc_lock_unlock (lock);

  return status;
}

enum nss_status
_nss_nis6_gethostton_r (const char *name, struct etherent *eth,
			char *buffer, size_t buflen, int *errnop)
{
  if (name == NULL)
    {
      *errnop = EINVAL;
      return NSS_STATUS_UNAVAIL;
    }

  char *domain;
  if (__glibc_unlikely (yp_get_default_domain (&domain)))
    return NSS_STATUS_UNAVAIL;

  char *result;
  int len;
  int yperr = yp_match (domain, "ethers.byname", name, strlen (name), &result,
			&len);

  if (__glibc_unlikely (yperr != YPERR_SUCCESS))
    {
      enum nss_status retval = yperr2nss (yperr);

      if (retval == NSS_STATUS_TRYAGAIN)
        *errnop = errno;
      return retval;
    }

  if (__glibc_unlikely ((size_t) (len + 1) > buflen))
    {
      free (result);
      *errnop = ERANGE;
      return NSS_STATUS_TRYAGAIN;
    }

  char *p = strncpy (buffer, result, len);
  buffer[len] = '\0';
  while (isspace (*p))
    ++p;
  free (result);

  int parse_res = _nss_files_parse_etherent (p, eth, (void *) buffer, buflen,
					     errnop);
  if (__glibc_unlikely (parse_res < 1))
    {
      if (parse_res == -1)
	return NSS_STATUS_TRYAGAIN;
      else
	return NSS_STATUS_NOTFOUND;
    }
  return NSS_STATUS_SUCCESS;
}

enum nss_status
_nss_nis6_getntohost_r (const struct ether_addr *addr, struct etherent *eth,
			char *buffer, size_t buflen, int *errnop)
{
  if (addr == NULL)
    {
      *errnop = EINVAL;
      return NSS_STATUS_UNAVAIL;
    }

  char *domain;
  if (__glibc_unlikely (yp_get_default_domain (&domain)))
    return NSS_STATUS_UNAVAIL;

  char buf[33];
  int nlen = snprintf (buf, sizeof (buf), "%x:%x:%x:%x:%x:%x",
		      (int) addr->ether_addr_octet[0],
		      (int) addr->ether_addr_octet[1],
		      (int) addr->ether_addr_octet[2],
		      (int) addr->ether_addr_octet[3],
		      (int) addr->ether_addr_octet[4],
		      (int) addr->ether_addr_octet[5]);

  char *result;
  int len;
  int yperr = yp_match (domain, "ethers.byaddr", buf, nlen, &result, &len);

  if (__glibc_unlikely (yperr != YPERR_SUCCESS))
    {
      enum nss_status retval = yperr2nss (yperr);

      if (retval == NSS_STATUS_TRYAGAIN)
        *errnop = errno;
      return retval;
    }

  if (__glibc_unlikely ((size_t) (len + 1) > buflen))
    {
      free (result);
      *errnop = ERANGE;
      return NSS_STATUS_TRYAGAIN;
    }

  char *p = strncpy (buffer, result, len);
  buffer[len] = '\0';
  while (isspace (*p))
    ++p;
  free (result);

  int parse_res = _nss_files_parse_etherent (p, eth, (void *) buffer, buflen,
					     errnop);
  if (__glibc_unlikely (parse_res < 1))
    {
      if (parse_res == -1)
	return NSS_STATUS_TRYAGAIN;
      else
	return NSS_STATUS_NOTFOUND;
    }
  return NSS_STATUS_SUCCESS;
}
