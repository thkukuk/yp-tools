/* Copyright (C) 2001, 2002, 2013, 2014 Thorsten Kukuk
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

#include <getopt.h>
#include <locale.h>
#include <libintl.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include "lib/nicknames.h"
#include "lib/yp_all_host.h"

#ifndef _
#define _(String) gettext (String)
#endif

extern int yp_maplist (const char *, struct ypmaplist **);

static int be_quiet = 0;

/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (void)
{
  fprintf (stdout, "yptest (%s) %s\n", PACKAGE, VERSION);
  fprintf (stdout, gettext ("\
Copyright (C) %s Thorsten Kukuk.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2014");
  fprintf (stdout, _("Written by %s.\n"), "Thorsten Kukuk");
}

static void
print_usage (FILE *stream)
{
  fputs (_("Usage: yptest [-q] [-d domain] [-h hostname] [-m map] [-u user]\n"),
	 stream);
}

static void
print_help (void)
{
  print_usage (stdout);
  fputs (_("yptest - call different NIS routines to test configuration\n\n"),
	 stdout);
  fputs (_("  -d domain      Use 'domain' instead of the default domain\n"),
	 stdout);
  fputs (_("  -h hostname    Query ypserv on 'hostname' instead the current one\n"),
	 stdout);
  fputs (_("  -m map         Use this existing map for tests\n"), stdout);
  fputs (_("  -u user        Use the existing NIS user 'user' for tests\n"),
	 stdout);
  fputs (_("  -q             Be quiet, don't print messages\n"),
	 stdout);
  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (void)
{
  const char *program = "yptest";
  print_usage (stderr);
  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   program, program);
}

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

/* bind to a special host and print the name ypbind running on this host
   is bound to */
static int
print_bindhost (const char *domain, const char *hostname, int vers)
{
  struct ypbind2_resp yp_r2;
  struct ypbind3_resp yp_r3;
  struct timeval tv;
  CLIENT *client;
  int ret;

  if (!hostname)
    hostname = "localhost";

  tv.tv_sec = 5;
  tv.tv_usec = 0;
  client = clnt_create (hostname, YPBINDPROG, vers, "udp");
  if (client == NULL)
    {
      if (!be_quiet)
	fprintf (stderr, "%s\n", yperr_string (YPERR_YPBIND));
      return 1;
    }

  memset (&yp_r2, 0, sizeof (yp_r2));
  memset (&yp_r3, 0, sizeof (yp_r3));
  tv.tv_sec = 15;
  tv.tv_usec = 0;
  if (vers == 1 || vers == 2)
    ret = clnt_call (client, YPBINDPROC_DOMAIN, (xdrproc_t) xdr_domainname,
                     (caddr_t) &domain, (xdrproc_t) xdr_ypbind2_resp,
                     (caddr_t) &yp_r2, tv);
  else
    ret = clnt_call (client, YPBINDPROC_DOMAIN, (xdrproc_t) xdr_domainname,
                     (caddr_t) &domain, (xdrproc_t) xdr_ypbind3_resp,
                     (caddr_t) &yp_r3, tv);

  if (ret != RPC_SUCCESS)
    {
      if (!be_quiet)
	fprintf (stderr, "%s\n", yperr_string (YPERR_YPBIND));
      clnt_destroy (client);
      return 1;
    }
  else
    {
      if (vers == 1 || vers == 2)
	{
	  if (yp_r2.ypbind_status != YPBIND_SUCC_VAL)
	    {
	      if (!be_quiet)
		fprintf (stderr, _("can't yp_bind: Reason: %s\n"),
			 ypbinderr_string (yp_r2.ypbind2_error));
	      clnt_destroy (client);
	      return 1;
	    }

	  if (!be_quiet)
	    printf (_("Used NIS server: %s\n"), inet_ntoa (yp_r2.ypbind2_addr));
	}
      else
        {
          if (yp_r3.ypbind_status != YPBIND_SUCC_VAL)
            {
	      if (!be_quiet)
	        fprintf (stderr, _("can't yp_bind: Reason: %s\n"),
		         ypbinderr_string (yp_r3.ypbind3_error));
              clnt_destroy (client);
              return 1;
            }
            if (!be_quiet)
	      ypbind3_binding_dump (yp_r3.ypbind3_bindinfo);
        }
  }
  clnt_destroy (client);

  return 0;
}


static int
print_data (int status, char *inkey, int inkeylen, char *inval,
            int invallen, char *indata __attribute__ ((unused)))
{
  if (status != YP_TRUE)
    return status;

  if (inkeylen > 0)
    {
      if (inkey[inkeylen - 1] == '\0')
        --inkeylen;
      if (!be_quiet)
	fprintf (stdout, "%*.*s ", inkeylen, inkeylen, inkey);
    }
  if (invallen > 0)
    {
      if (inval[invallen -1] == '\0')
        --invallen;
      if (!be_quiet)
	fprintf (stdout, "%*.*s\n", invallen, invallen, inval);
    }
  else if (!be_quiet)
    fputs ("\n", stdout);

  return 0;
}

int
main (int argc, char **argv)
{
  char *domainname = NULL, *domain = NULL;
  char *hostname = NULL;
  int  result = 0;
  char *map = "passwd.byname";
  char *key = "nobody";
  int  KeyLen;
  char *Value;
  char *Key2;
  int      ValLen;
  int status;
  unsigned int order;
  struct ypall_callback Callback;
  struct ypmaplist *ypml, *y;

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

      c = getopt_long (argc, argv, "d:h:m:u:q?", long_options, &option_index);
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
	case 'm':
	  map = optarg;
	  break;
	case 'u':
	  key = optarg;
	  break;
	case 'q':
	  be_quiet = 1;
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

  if (!be_quiet)
    printf ("Test 1: domainname\n");
  yp_get_default_domain(&domain);
  if (domain == NULL || domain[0] == '\0')
    {
      if (!domainname)
	{
	  if (!be_quiet)
	    fputs (_("ERROR: domainname is not set!\n"), stderr);
	  return 1;
	}

      if (!be_quiet)
	fputs (_("WARNING: domainname is not set!\n"), stderr);
      ++result;
    }
  else if (!be_quiet)
    printf (_("Configured domainname is \"%s\"\n"), domain);

  if (domainname == NULL)
    domainname = domain;
  else if (!be_quiet)
    printf (_("Domainname which will be used due the test: \"%s\"\n"),
	    domainname);

  if (!be_quiet)
    printf ("\nTest 2: ypbind\n");

  if (!be_quiet)
    printf (_("Use Protocol V1: "));
  if (print_bindhost (domainname, hostname, 1))
    fprintf (stderr, _("ypbind procotcol v1 test failed\n"));
  if (!be_quiet)
    printf (_("Use Protocol V2: "));
  if (print_bindhost (domainname, hostname, 2))
    fprintf (stderr, _("ypbind procotcol v2 test failed\n"));
  if (!be_quiet)
    printf (_("Use Protocol V3:\n"));
  if (print_bindhost (domainname, hostname, 3))
    fprintf (stderr, _("ypbind procotcol v3 test failed\n"));

  if (!be_quiet)
    printf ("\nTest 3: yp_match\n");
  KeyLen = strlen (key);
  status = yp_match (domainname, map, key, KeyLen, &Value, &ValLen);
  switch (status)
    {
    case YPERR_SUCCESS:
      if (!be_quiet)
	printf("%*.*s\n", ValLen, ValLen, Value);
      break;
    case YPERR_YPBIND:
      if (!be_quiet)
	fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      if (!be_quiet)
	fprintf (stderr, _("WARNING: %s (Map %s, key %s)\n"),
		 yperr_string (status), map, key);
      ++result;
    }

  if (!be_quiet)
    printf("\nTest 4: yp_first\n");
  status = yp_first(domainname, map, &Key2, &KeyLen, &Value, &ValLen);
  switch (status)
    {
    case YPERR_SUCCESS:
      if (!be_quiet)
	printf("%*.*s %*.*s\n", KeyLen, KeyLen, Key2, ValLen, ValLen, Value);
      break;
    case YPERR_YPBIND:
      if (!be_quiet)
	fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      if (!be_quiet)
	fprintf (stderr, _("WARNING: %s (Map %s)\n"),
		 yperr_string (status), map);
      ++result;
    }

  if (!be_quiet)
    printf ("\nTest 5: yp_next\n");
  if (status != YPERR_SUCCESS)
    if (!be_quiet)
      printf (_("-- skipped --\n"));
  while (status == 0)
    {
      status = yp_next (domainname, map, Key2, KeyLen, &Key2,
			&KeyLen, &Value, &ValLen);
      switch (status)
	{
	case YPERR_SUCCESS:
	  if (!be_quiet)
	    printf("%*.*s %*.*s\n", KeyLen, KeyLen, Key2,
		   ValLen, ValLen, Value);
	  break;
	case YPERR_YPBIND:
	  if (!be_quiet)
	    fprintf (stderr, _("ERROR: No running ypbind\n"));
	  return 1;
	case YPERR_NOMORE:
	  break;
	default:
	  if (!be_quiet)
	    fprintf (stderr, _("WARNING: %s (Map %s)\n"),
		     yperr_string (status), map);
	  ++result;
	}
    }

  if (!be_quiet)
    printf("\nTest 6: yp_master\n");
  status = yp_master (domainname, map, &Key2);
  switch (status)
    {
    case YPERR_SUCCESS:
      if (!be_quiet)
	printf("%s\n", Key2);
      break;
    case YPERR_YPBIND:
      if (!be_quiet)
	fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      if (!be_quiet)
	fprintf (stderr, _("WARNING: %s (Map %s)\n"),
		 yperr_string (status), map);
      ++result;
    }

  if (!be_quiet)
    printf ("\nTest 7: yp_order\n");
  status = yp_order (domainname, map, &order);
  switch (status)
    {
    case YPERR_SUCCESS:
      if (!be_quiet)
	printf ("%d\n", order);
      break;
    case YPERR_YPBIND:
      if (!be_quiet)
	fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      if (!be_quiet)
	fprintf (stderr, _("WARNING: %s (Map %s)\n"),
		 yperr_string (status), map);
      ++result;
    }

  if (!be_quiet)
    printf("\nTest 8: yp_maplist\n");
  ypml = NULL;
  status = yp_maplist (domainname, &ypml);
  switch (status)
    {
    case YPERR_SUCCESS:
      for(y = ypml; y; )
	{
	  ypml = y;
	  if (!be_quiet)
	    printf("%s\n", ypml->map);
	  y = ypml->next;
	}
      break;
    case YPERR_YPBIND:
      if (!be_quiet)
	fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      if (!be_quiet)
	fprintf (stderr, _("WARNING: %s\n"),
		 yperr_string (status));
      ++result;
    }

  if (!be_quiet)
    printf("\nTest 9: yp_all\n");
  Callback.foreach = print_data;
  if (hostname)
    status = yp_all_host (domainname, map, &Callback, hostname);
  else
    status = yp_all(domainname, map, &Callback);
  switch (status)
    {
    case YPERR_SUCCESS:
      break;
    case YPERR_YPBIND:
      if (!be_quiet)
	fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      if (!be_quiet)
	fprintf (stderr, _("WARNING: %s (Map %s)\n"),
		 yperr_string (status), map);
      ++result;
    }

  if (!be_quiet)
    {
      if (result == 0)
	printf (_("All tests passed\n"));
      else
	fprintf (stderr, _("%d tests failed\n"), result);
    }
  return result;
}
