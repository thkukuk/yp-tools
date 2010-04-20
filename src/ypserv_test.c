/* Copyright (C) 2001, 2002, 2003, 2009 Thorsten Kukuk
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

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif
#include <locale.h>
#include <libintl.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <rpc/rpc.h>
#ifdef HAVE_RPC_CLNT_SOC_H
#include <rpc/clnt_soc.h>
#endif
#include <rpcsvc/yp_prot.h>
#include "lib/nicknames.h"
#include "lib/yp_all_host.h"

#ifndef _
#define _(String) gettext (String)
#endif

static struct timeval TIMEOUT = { 10, 0 };
static char *domainname = NULL;
static char *hostname = "localhost";
static int do_loop = 0;

/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (void)
{
  fprintf (stdout, "ypserv_test (%s) %s\n", PACKAGE, VERSION);
  fprintf (stdout, gettext ("\
Copyright (C) %s Thorsten Kukuk.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2001");
  fprintf (stdout, _("Written by %s.\n"), "Thorsten Kukuk");
}

static void
print_usage (FILE *stream)
{
  fputs (_("Usage: ypserv_test [-l] [-d domain] [-h hostname] [-m map] [-u user]\n"),
	 stream);
}

static void
print_help (void)
{
  print_usage (stdout);
  fputs (_("ypserv_test - call different NIS routines to test ypserv\n\n"),
	 stdout);
  fputs (_("  -d domain      Use 'domain' instead of the default domain\n"),
	 stdout);
  fputs (_("  -h hostname    Query ypserv on 'hostname' instead the current one\n"),
	 stdout);
  fputs (_("  -l             Run all queries in an endless loop\n"),  stdout);
  fputs (_("  -m map         Use this existing map for tests\n"), stdout);
  fputs (_("  -u user        Use the existing NIS user 'user' for tests\n"),
	 stdout);
  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (void)
{
  const char *program = "ypserv_test";
  print_usage (stderr);
  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   program, program);
}

static enum clnt_stat
ypproc_null_2(void *argp, void *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_NULL,
		    (xdrproc_t) xdr_void, (caddr_t) argp,
		    (xdrproc_t) xdr_void, (caddr_t) clnt_res,
		    TIMEOUT));
}

static void *
test_ypproc_null_2 (void *v_param __attribute__((unused)))
{
  CLIENT *clnt;
  unsigned long int count = 0;

  clnt = clnt_create (hostname, YPPROG, YPVERS, "udp");
  if (clnt == NULL)
    {
      int retval = 1;
      clnt_pcreateerror (hostname);
      pthread_exit (&retval);
    }

  do
    {
      if (ypproc_null_2 (NULL, NULL, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_null_2");
	}
    } while (do_loop);

  return NULL;
}

static enum clnt_stat
ypproc_domain_2 (char **argp, bool_t *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_DOMAIN,
		    (xdrproc_t) xdr_domainname, (caddr_t) argp,
		    (xdrproc_t) xdr_bool, (caddr_t) clnt_res,
		    TIMEOUT));
}

static void *
test_ypproc_domain_2 (void *v_param __attribute__ ((unused)))
{
  CLIENT *clnt;
  char *domain_ack = domainname;
  char *domain_inv = "../../etc/";
  char *domain_nak = "doesnotexist";
  bool_t result;
  unsigned long int count = 0;

  clnt = clnt_create (hostname, YPPROG, YPVERS, "udp");
  if (clnt == NULL)
    {
      int retval = 1;
      clnt_pcreateerror (hostname);
      pthread_exit (&retval);
    }

  do
    {
      /* At first, try a correct domainname.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_domain_2 (&domain_ack, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_domain_2");
	}
      else if (result != TRUE)
	{
	  count++;
	  fprintf (stderr, "ypproc_domain_2: ypserv sends NAK instead of ACK\n");
	}


      /* Second try: Invalid domainname.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_domain_2 (&domain_inv, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_domain_2");
	}
      else if (result == TRUE)
	{
	  count++;
	  fprintf (stderr, "ypproc_domain_2: ypserv sends ACK instead of NAK\n");
	}

      /* Third try: Not existing domainname.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_domain_2 (&domain_nak, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_domain_2");
	}
      else if (result == TRUE)
	{
	  count++;
	  fprintf (stderr, "ypproc_domain_2: ypserv sends ACK instead of NAK\n");
	}
    } while (do_loop);

  return NULL;
}

static enum clnt_stat
ypproc_domain_nonack_2 (char **argp, bool_t *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_DOMAIN_NONACK,
		    (xdrproc_t) xdr_domainname, (caddr_t) argp,
		    (xdrproc_t) xdr_bool, (caddr_t) clnt_res,
		    TIMEOUT));
}

static void *
test_ypproc_domain_nonack_2 (void *v_param __attribute__ ((unused)))
{
  CLIENT *clnt;
  char *domain_ack = domainname;
  char *domain_inv = "../../etc/";
  char *domain_nak = "doesnotexist";
  bool_t result;
  unsigned long int count = 0;

  clnt = clnt_create (hostname, YPPROG, YPVERS, "udp");
  if (clnt == NULL)
    {
      int retval = 1;
      clnt_pcreateerror (hostname);
      pthread_exit (&retval);
    }

  do
    {
      /* At first, try a correct domainname.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_domain_nonack_2 (&domain_ack, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_domain_nonack_2");
	}
      else if (result != TRUE)
	{
	  count++;
	  fprintf (stderr, "ypproc_domain_nonack_2: ypserv sends NAK instead of ACK\n");
	}


      /* Second try: Invalid domainname.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_domain_nonack_2 (&domain_inv, &result, clnt) != RPC_TIMEDOUT)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_domain_nonack_2");
	}
      else if (result == TRUE)
	{
	  count++;
	  fprintf (stderr, "ypproc_domain_nonack_2: ypserv sends ACK instead of NAK\n");
	}

      /* Third try: Not existing domainname.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_domain_nonack_2 (&domain_nak, &result, clnt) != RPC_TIMEDOUT)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_domain_nonack_2");
	}
      else if (result == TRUE)
	{
	  count++;
	  fprintf (stderr, "ypproc_domain_nonack_2: ypserv sends ACK instead of NAK\n");
	}
    } while (do_loop);

  return NULL;
}

static enum clnt_stat
ypproc_match_2 (struct ypreq_key *argp, struct ypresp_val *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_MATCH,
		    (xdrproc_t) xdr_ypreq_key, (caddr_t) argp,
		    (xdrproc_t) xdr_ypresp_val, (caddr_t) clnt_res,
		    TIMEOUT));
}

/*
 * for unit alignment
 */
static const char xdr_zero[BYTES_PER_XDR_UNIT] = {0, 0, 0, 0};


static bool_t
xdr_opaque_fake (XDR *xdrs, caddr_t cp, u_int cnt)
{
  u_int rndup;
  static char crud[BYTES_PER_XDR_UNIT];

  /*
   * if no data we are done
   */
  if (cnt == 0)
    return TRUE;

  /*
   * round byte count to full xdr units
   */
  rndup = cnt % BYTES_PER_XDR_UNIT;
  if (rndup > 0)
    rndup = BYTES_PER_XDR_UNIT - rndup;

  switch (xdrs->x_op)
    {
    case XDR_DECODE:
      if (!XDR_GETBYTES (xdrs, cp, cnt))
        {
          return FALSE;
        }
      if (rndup == 0)
        return TRUE;
      return XDR_GETBYTES (xdrs, (caddr_t)crud, rndup);

    case XDR_ENCODE:
      if (!XDR_PUTBYTES (xdrs, cp, cnt))
        {
          return FALSE;
        }
      if (rndup == 0)
        return TRUE;
      return XDR_PUTBYTES (xdrs, xdr_zero, rndup);

    case XDR_FREE:
      return TRUE;
    }
  return FALSE;
}


static bool_t
xdr_bytes_fake (XDR *xdrs, char **cpp, u_int *sizep, u_int maxsize)
{
  char *sp = *cpp;      /* sp is the actual string pointer */
  u_int nodesize;

  /*
   * first deal with the length since xdr bytes are counted
   */
  if (!xdr_u_int (xdrs, sizep))
    {
      return FALSE;
    }
  nodesize = *sizep;
  if ((nodesize > maxsize) && (xdrs->x_op != XDR_FREE))
    {
      return FALSE;
    }

  /*
   * now deal with the actual bytes
   */
  switch (xdrs->x_op)
    {
    case XDR_DECODE:
      if (nodesize == 0)
        {
          return TRUE;
        }
      if (sp == NULL)
        {
          *cpp = sp = (char *) mem_alloc (nodesize);
        }
      if (sp == NULL)
        {
          (void) fprintf (stderr, "%s: %s", __func__, _("out of memory\n"));
          return FALSE;
        }
      /* fall into ... */

    case XDR_ENCODE:
      return xdr_opaque_fake (xdrs, sp, nodesize);

    case XDR_FREE:
      if (sp != NULL)
        {
          mem_free (sp, nodesize);
          *cpp = NULL;
        }
      return TRUE;
    }
  return FALSE;
}


static bool_t
xdr_keydat_fake (XDR *xdrs, keydat_t *objp)
{
  return xdr_bytes_fake (xdrs, (char **) &objp->keydat_val,
			 (u_int *) &objp->keydat_len, ~0);
}

static bool_t
xdr_ypreq_key_fake (XDR *xdrs, struct ypreq_key *objp)
{
  if (!xdr_domainname (xdrs, &objp->domain))
    return FALSE;
  if (!xdr_mapname (xdrs, &objp->map))
    return FALSE;
  return xdr_keydat_fake (xdrs, &objp->keydat);
}

static enum clnt_stat
ypproc_match_2_fake (struct ypreq_key *argp, struct ypresp_val *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_MATCH,
		    (xdrproc_t) xdr_ypreq_key_fake, (caddr_t) argp,
		    (xdrproc_t) xdr_ypresp_val, (caddr_t) clnt_res,
		    TIMEOUT));
}

static void *
test_ypproc_match_2 (void *v_param)
{
  CLIENT *clnt;
  char *key = v_param;
  struct ypreq_key request1 = {domainname, "passwd.byname", {strlen(key), key}};
  struct ypreq_key request2 = {domainname, "passwd.byname", {5, "nokey"}};
  struct ypreq_key request3 = {domainname, "passwd-byname", {strlen(key), key}};
  struct ypreq_key request4 = {"../../etc/", "passwd.byname", {strlen(key), key}};
  struct ypreq_key request5 = {domainname, "passwd.byname", {0, ""}};
  struct ypreq_key request6 = {domainname, "passwd.byname", {8000, ""}};
  struct ypresp_val result;
  unsigned long int count = 0;

  clnt = clnt_create (hostname, YPPROG, YPVERS, "udp");
  if (clnt == NULL)
    {
      int retval = 1;
      clnt_pcreateerror (hostname);
      pthread_exit (&retval);
    }

  do
    {
      /* At first, try a correct query.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_match_2 (&request1, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_match_2");
	}
      else if (result.status != YP_TRUE)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_match_2: ypserv sends %d instead of YP_TRUE\n",
		   result.status);
	}

      /* Second try: Unknown user  */
      memset (&result, 0, sizeof (result));
      if (ypproc_match_2 (&request2, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_match_2");
	}
      else if (result.status != YP_NOKEY)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_match_2: ypserv sends %d instead of YP_NOKEY\n",
		   result.status);
	}

      /* Third: Invalid map name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_match_2 (&request3, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_match_2");
	}
      else if (result.status != YP_NOMAP)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_match_2: ypserv sends %d instead of YP_NOMAP\n",
		   result.status);
	}

      /* Fourth: Invalid domain name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_match_2 (&request4, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_match_2");
	}
      else if (result.status != YP_NODOM)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_match_2: ypserv sends %d instead of YP_NODOM\n",
		   result.status);
	}

      /* Fifth: Invalid key name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_match_2 (&request5, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_match_2");
	}
      else if (result.status != YP_BADARGS)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_match_2: ypserv sends %d instead of YP_BADARGS\n",
		   result.status);
	}

      /* Six: Invalid size of key name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_match_2_fake (&request6, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_match_2");
	}
      else if (result.status != YP_BADARGS)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_match_2(6): ypserv sends %d instead of YP_BADARGS\n",
		   result.status);
	}


    } while (do_loop);

  return NULL;
}

static enum clnt_stat
ypproc_first_2 (struct ypreq_nokey *argp, struct ypresp_key_val *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_FIRST,
		    (xdrproc_t) xdr_ypreq_nokey, (caddr_t) argp,
		    (xdrproc_t) xdr_ypresp_key_val, (caddr_t) clnt_res,
		    TIMEOUT));
}

static void *
test_ypproc_first_2 (void *v_param __attribute__ ((unused)))
{
  CLIENT *clnt;
  struct ypreq_nokey request1 = {domainname, "passwd.byname"};
  struct ypreq_nokey request2 = {domainname, "passwd-byname"};
  struct ypreq_nokey request3 = {"../../etc/", "passwd.byname"};
  struct ypreq_nokey request4 = {domainname, ""};
  struct ypreq_nokey request5 = {"", "passwd.byname"};
  struct ypresp_key_val result;
  unsigned long int count = 0;

  clnt = clnt_create (hostname, YPPROG, YPVERS, "udp");
  if (clnt == NULL)
    {
      int retval = 1;
      clnt_pcreateerror (hostname);
      pthread_exit (&retval);
    }

  do
    {
      /* At first, try a correct query.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_first_2 (&request1, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_first_2");
	}
      else if (result.status != YP_TRUE)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_first_2: ypserv sends %d instead of YP_TRUE\n",
		   result.status);
	}

      /* Second try: Invalid map name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_first_2 (&request2, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_first_2");
	}
      else if (result.status != YP_NOMAP)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_first_2: ypserv sends %d instead of YP_NOMAP\n",
		   result.status);
	}

      /* Third: Invalid domainname name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_first_2 (&request3, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_first_2");
	}
      else if (result.status != YP_NODOM)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_first_2: ypserv sends %d instead of YP_NODOM\n",
		   result.status);
	}

      /* Fourth: Invalid domain name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_first_2 (&request4, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_first_2");
	}
      else if (result.status != YP_BADARGS)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_first_2: ypserv sends %d instead of YP_BADARGS\n",
		   result.status);
	}

      /* Fifth: Empty map name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_first_2 (&request5, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_first_2");
	}
      else if (result.status != YP_NODOM)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_first_2: ypserv sends %d instead of YP_NODOM\n",
		   result.status);
	}


    } while (do_loop);

  return NULL;
}

static enum clnt_stat
ypproc_next_2 (struct ypreq_key *argp, struct ypresp_key_val *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_NEXT,
		    (xdrproc_t) xdr_ypreq_key, (caddr_t) argp,
		    (xdrproc_t) xdr_ypresp_key_val, (caddr_t) clnt_res,
		    TIMEOUT));
}

static void *
test_ypproc_next_2 (void *v_param)
{
  CLIENT *clnt;
  char *key = v_param;
  struct ypreq_key request1 = {domainname, "passwd.byname", {strlen(key), key}};
  struct ypreq_key request2 = {domainname, "passwd.byname", {5, "nokey"}};
  struct ypreq_key request3 = {domainname, "passwd.byname", {0, NULL}};
  struct ypreq_key request4 = {domainname, "passwd-byname", {strlen(key), key}};
  struct ypreq_key request5 = {"../../etc/", "passwd.byname", {strlen(key), key}};
  struct ypreq_key request6 = {domainname, "", {strlen(key), key}};
  struct ypreq_key request7 = {"", "passwd.byname", {strlen(key), key}};
  struct ypresp_key_val result;
  unsigned long int count = 0;

  clnt = clnt_create (hostname, YPPROG, YPVERS, "udp");
  if (clnt == NULL)
    {
      int retval = 1;
      clnt_pcreateerror (hostname);
      pthread_exit (&retval);
    }

  do
    {
      /* At first, try a correct query.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_next_2 (&request1, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_next_2");
	}
      else if (result.status != YP_TRUE)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_next_2: ypserv sends %d instead of YP_TRUE\n",
		   result.status);
	}

      /* 2: not existing key  */
      memset (&result, 0, sizeof (result));
      if (ypproc_next_2 (&request2, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_next_2");
	}
      else if (result.status != YP_NOMORE)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_next_2: ypserv sends %d instead of YP_NOMORE\n",
		   result.status);
	}

      /* 3: Invalid key  */
      memset (&result, 0, sizeof (result));
      if (ypproc_next_2 (&request3, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_next_2");
	}
      else if (result.status != YP_NOMORE)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_next_2: ypserv sends %d instead of YP_NOMORE\n",
		   result.status);
	}

      /* Fourth try: Invalid map name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_next_2 (&request4, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_next_2");
	}
      else if (result.status != YP_NOMAP)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_next_2: ypserv sends %d instead of YP_NOMAP\n",
		   result.status);
	}

      /* 5: Invalid domainname name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_next_2 (&request5, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_next_2");
	}
      else if (result.status != YP_NODOM)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_next_2: ypserv sends %d instead of YP_NODOM\n",
		   result.status);
	}

      /* 6: Invalid domain name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_next_2 (&request6, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_next_2");
	}
      else if (result.status != YP_BADARGS)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_next_2: ypserv sends %d instead of YP_BADARGS\n",
		   result.status);
	}

      /* 7: Empty map name.  */
      memset (&result, 0, sizeof (result));
      if (ypproc_next_2 (&request7, &result, clnt) != RPC_SUCCESS)
	{
	  count++;
	  clnt_perror (clnt, "ypproc_next_2");
	}
      else if (result.status != YP_NODOM)
	{
	  count++;
	  fprintf (stderr,
		   "ypproc_next_2: ypserv sends %d instead of YP_NODOM\n",
		   result.status);
	}


    } while (do_loop);

  return NULL;
}


int
main (int argc, char **argv)
{
  char *domain = NULL;
  char *map = "passwd.byname";
  char *key = "nobody";

  setlocale (LC_MESSAGES, "");
  setlocale (LC_CTYPE, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  while (1)
    {
      int c;
      int option_index = 0;
      static struct option long_options[] =
      {
        {"version", no_argument, NULL, '\255'},
        {"usage", no_argument, NULL, '\254'},
        {"help", no_argument, NULL, '?'},
        {NULL, 0, NULL, '\0'}
      };

      c = getopt_long (argc, argv, "d:h:k:lm:?", long_options, &option_index);
      if (c == (-1))
        break;
      switch (c)
        {
	case 'd':
	  domainname = optarg;
	  break;
	case 'h':
	  hostname = optarg;
	  break;
	case 'l':
	  do_loop = 1;
	  break;
	case 'm':
	  map = optarg;
	  break;
	case 'k':
	  key = optarg;
	  break;
	case '?':
	  print_help ();
	  return 0;
	case '\255':
	  print_version ();
	  return 0;
	case '\254':
	  print_usage (stdout);
	  return 0;
	default:
	  print_usage (stderr);
	  return 1;
	}
    }

  argc -= optind;
  argv += optind;

  if (argc >= 1)
    {
      print_error ();
      return 1;
    }

  if (!domainname)
    {
      yp_get_default_domain(&domain);
      if (domain == NULL || domain[0] == '\0')
	{
	  fputs (_("ERROR: domainname is not set!\n"), stderr);
	  return 1;
	}
    }

  if (domainname == NULL)
    domainname = domain;

  if (do_loop)
    {
      pthread_t thread1, thread2, thread3, thread4, thread5, thread6;

      pthread_create (&thread1, NULL, &test_ypproc_null_2, NULL);
      pthread_create (&thread2, NULL, &test_ypproc_domain_2, NULL);
      pthread_create (&thread3, NULL, &test_ypproc_domain_nonack_2, NULL);
      pthread_create (&thread4, NULL, &test_ypproc_match_2, key);
      pthread_create (&thread5, NULL, &test_ypproc_first_2, NULL);
      pthread_create (&thread6, NULL, &test_ypproc_next_2, key);
      sleep (5*60);
    }
  else
    {
      test_ypproc_null_2 (NULL);
      test_ypproc_domain_2 (NULL);
      test_ypproc_domain_nonack_2 (NULL);
      test_ypproc_match_2 (key);
      test_ypproc_first_2 (NULL);
      test_ypproc_next_2 (key);
    }

  return 0;
}
