/* Copyright (C) 2001 Thorsten Kukuk
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
#include <rpc/rpc.h>
#ifdef HAVE_RPC_CLNT_SOC_H
#include <rpc/clnt_soc.h>
#endif
#include "lib/yp-tools.h"
#include "lib/nicknames.h"
#include "lib/yp_all_host.h"

#ifndef _
#define _(String) gettext (String)
#endif

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
"), "2001");
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

/* bind to a special host and print the name ypbind running on this host
   is bound to */
static int
print_bindhost (char *domain, struct sockaddr_in *socka_in, int vers)
{
  struct hostent *host = NULL;
  struct ypbind_resp yp_r;
  struct timeval tv;
  CLIENT *client;
  int sock, ret;
  struct in_addr saddr;

  sock = RPC_ANYSOCK;
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  client = clntudp_create (socka_in, YPBINDPROG, vers, tv, &sock);
  if (client == NULL)
    {
      fprintf (stderr, "%s\n", yperr_string (YPERR_YPBIND));
      return 1;
    }

  tv.tv_sec = 15;
  tv.tv_usec = 0;
  if (vers == 1)
    ret = clnt_call (client, YPBINDPROC_OLDDOMAIN,
                     (xdrproc_t) ytxdr_domainname,
                     (caddr_t) &domain, (xdrproc_t) ytxdr_ypbind_resp,
                     (caddr_t) &yp_r, tv);
  else
    ret = clnt_call (client, YPBINDPROC_DOMAIN, (xdrproc_t) ytxdr_domainname,
                     (caddr_t) &domain, (xdrproc_t) ytxdr_ypbind_resp,
                     (caddr_t) &yp_r, tv);

  if (ret != RPC_SUCCESS)
    {
      fprintf (stderr, "%s\n", yperr_string (YPERR_YPBIND));
      clnt_destroy (client);
      return 1;
    }
  else
    {
      if (yp_r.ypbind_status != YPBIND_SUCC_VAL)
        {
          fprintf (stderr, _("can't yp_bind: Reason: %s\n"),
                   ypbinderr_string (yp_r.ypbind_respbody.ypbind_error));
          clnt_destroy (client);
          return 1;
        }
    }
  clnt_destroy (client);

  saddr = yp_r.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;
  host = gethostbyaddr ((char *) &saddr, sizeof (saddr), AF_INET);
  if (host)
    printf (_("Used NIS server: %s\n"), host->h_name);
  else
    printf (_("Used NIS server: %s\n"), inet_ntoa (saddr));
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
      fprintf (stdout, "%*.*s ", inkeylen, inkeylen, inkey);
    }
  if (invallen > 0)
    {
      if (inval[invallen -1] == '\0')
        --invallen;
      fprintf (stdout, "%*.*s\n", invallen, invallen, inval);
    }
  else
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
  int order;
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

  printf ("Test 1: domainname\n");
  yp_get_default_domain(&domain);
  if (domain == NULL || domain[0] == '\0')
    {
      if (!domainname)
	{
	  fputs (_("ERROR: domainname is not set!\n"), stderr);
	  return 1;
	}

      fputs (_("WARNING: domainname is not set!\n"), stderr);
      ++result;
    }
  else
    printf (_("Configured domainname is \"%s\"\n"), domain);

  if (domainname == NULL)
    domainname = domain;
  else
    printf (_("Domainname which will be used due the test: \"%s\"\n"),
	    domainname);

  printf ("\nTest 2: ypbind\n");
  if (hostname)
    {
      struct sockaddr_in sock_in;
      struct hostent *host;

      memset (&sock_in, 0, sizeof sock_in);
      sock_in.sin_family = AF_INET;

      if (inet_aton (hostname, &sock_in.sin_addr) == 0)
	{
	  host = gethostbyname (hostname);
	  if (!host)
	    {
	      fprintf (stderr, _("yptest: host %s unknown\n"),
		       hostname);
	      return 1;
	    }
	  memcpy (&sock_in.sin_addr, host->h_addr_list[0],
		  sizeof (sock_in.sin_addr));

	  if (print_bindhost (domainname, &sock_in, 2))
	    return 1;
	}
    }
  else
    {
      struct sockaddr_in sock_in;

      memset (&sock_in, 0, sizeof sock_in);
      sock_in.sin_family = AF_INET;
      sock_in.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

      if (print_bindhost (domainname, &sock_in, 2))
	return 1;
    }

  printf ("\nTest 3: yp_match\n");
  KeyLen = strlen (key);
  status = yp_match (domainname, map, key, KeyLen, &Value, &ValLen);
  switch (status)
    {
    case YPERR_SUCCESS:
      printf("%*.*s\n", ValLen, ValLen, Value);
      break;
    case YPERR_YPBIND:
      fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      fprintf (stderr, _("WARNING: %s (Map %s, key %s)\n"),
	       yperr_string (status), map, key);
      ++result;
    }

  printf("\nTest 4: yp_first\n");
  status = yp_first(domainname, map, &Key2, &KeyLen, &Value, &ValLen);
  switch (status)
    {
    case YPERR_SUCCESS:
      printf("%*.*s %*.*s\n", KeyLen, KeyLen, Key2, ValLen, ValLen, Value);
      break;
    case YPERR_YPBIND:
      fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      fprintf (stderr, _("WARNING: %s (Map %s)\n"),
	       yperr_string (status), map);
      ++result;
    }

  printf ("\nTest 5: yp_next\n");
  if (status != YPERR_SUCCESS)
    printf (_("-- skipped --\n"));
  while (status == 0)
    {
      status = yp_next (domainname, map, Key2, KeyLen, &Key2,
			&KeyLen, &Value, &ValLen);
      switch (status)
	{
	case YPERR_SUCCESS:
	  printf("%*.*s %*.*s\n", KeyLen, KeyLen, Key2,
		 ValLen, ValLen, Value);
	  break;
	case YPERR_YPBIND:
	  fprintf (stderr, _("ERROR: No running ypbind\n"));
	  return 1;
	case YPERR_NOMORE:
	  break;
	default:
	  fprintf (stderr, _("WARNING: %s (Map %s)\n"),
		   yperr_string (status), map);
	  ++result;
	}
    }

  printf("\nTest 6: yp_master\n");
  status = yp_master (domainname, map, &Key2);
  switch (status)
    {
    case YPERR_SUCCESS:
      printf("%s\n", Key2);
      break;
    case YPERR_YPBIND:
      fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      fprintf (stderr, _("WARNING: %s (Map %s)\n"),
	       yperr_string (status), map);
      ++result;
    }

  printf ("\nTest 7: yp_order\n");
  status = yp_order (domainname, map, &order);
  switch (status)
    {
    case YPERR_SUCCESS:
      printf ("%d\n", order);
      break;
    case YPERR_YPBIND:
      fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      fprintf (stderr, _("WARNING: %s (Map %s)\n"),
	       yperr_string (status), map);
      ++result;
    }

  printf("\nTest 8: yp_maplist\n");
  ypml = NULL;
  status = yp_maplist (domainname, &ypml);
  switch (status)
    {
    case YPERR_SUCCESS:
      for(y = ypml; y; )
	{
	  ypml = y;
	  printf("%s\n", ypml->map);
	  y = ypml->next;
	}
      break;
    case YPERR_YPBIND:
      fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      fprintf (stderr, _("WARNING: %s\n"),
	       yperr_string (status));
      ++result;
    }

  printf("\nTest 8: yp_all\n");
  Callback.foreach = print_data;
  if (hostname)
    status = yp_all_host (hostname, domainname, map, &Callback);
  else
    status = yp_all(domainname, map, &Callback);
  switch (status)
    {
    case YPERR_SUCCESS:
      break;
    case YPERR_YPBIND:
      fprintf (stderr, _("ERROR: No running ypbind\n"));
      return 1;
    default:
      fprintf (stderr, _("WARNING: %s (Map %s)\n"),
	       yperr_string (status), map);
      ++result;
    }

  return result;
}
