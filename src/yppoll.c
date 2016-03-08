/* Copyright (C) 1998, 1999, 2001, 2014, 2016 Thorsten Kukuk
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

#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>
#include <libintl.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>

#ifndef _
#define _(String) gettext (String)
#endif

#if !defined(HAVE_YPBIND3)
#define ypbind2_resp ypbind_resp
#endif

/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (void)
{
  fprintf (stdout, "yppoll (%s) %s\n", PACKAGE, VERSION);
  fprintf (stdout, gettext ("\
Copyright (C) %s Thorsten Kukuk.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2016");
  /* fprintf (stdout, _("Written by %s.\n"), "Thorsten Kukuk"); */
}

static void
print_usage (FILE *stream)
{
  fputs (_("Usage: yppoll [-h host] [-d domain] mapname\n"),
	 stream);
}

static void
print_help (void)
{
  print_usage (stdout);
  fputs (_("yppoll - return version and master server of a NIS map\n\n"),
	 stdout);

  fputs (_("  -h host        Ask ypserv process at 'host'\n"), stdout);
  fputs (_("  -d domain      Use 'domain' instead of the default domain\n"),
	 stdout);
  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (void)
{
  const char *program = "yppoll";

  print_usage (stderr);
  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   program, program);
}

int
main (int argc, char **argv)
{
  static struct timeval RPCTIMEOUT = {25, 0};
  char *hostname = NULL, *domainname = NULL, *master = NULL;
  int result;
  time_t order;
  CLIENT *client;
  bool_t clnt_res;
  int res1, res2;
  struct ypreq_nokey req;
  struct ypresp_order resp_o;
  struct ypresp_master resp_m;

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

      c = getopt_long (argc, argv, "d:h:?", long_options, &option_index);
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

  if (argc != 1)
    {
      print_error ();
      return 1;
    }

  if (domainname == NULL)
    {
      int error;

      if ((error = yp_get_default_domain (&domainname)) != 0)
	{
	  fprintf (stderr, _("%s: can't get local yp domain: %s\n"),
		   "yppoll", yperr_string (error));
	  return 1;
	}
    }

  if (!hostname)
    {
      int ret;
#if defined(HAVE_YPBIND3)
      struct ypbind3_resp yp3_r;

      memset (&yp3_r, 0, sizeof (struct ypbind3_resp));

      /* ask local ypbind for NIS server */
      ret = rpc_call ("localhost", YPBINDPROG, YPBINDVERS, YPBINDPROC_DOMAIN,
		      (xdrproc_t) xdr_domainname, (caddr_t) &domainname,
		      (xdrproc_t) xdr_ypbind3_resp, (caddr_t) &yp3_r,
		      "udp");
      if (ret == RPC_SUCCESS)
	{
	  if (yp3_r.ypbind_status == YPBIND_SUCC_VAL)
	    hostname = yp3_r.ypbind3_servername;
	  else
	    {
	      fprintf (stderr, _("Domain not bound\n"));
	      return 1;
	    }
	}
      else if (ret == RPC_PROGVERSMISMATCH)
#endif
	{
	  /* Looks like ypbind does not support V3 yet, fallback
	     to V2 */
	  struct ypbind2_resp yp2_r;
	  memset (&yp2_r, 0, sizeof (struct ypbind2_resp));

	  /* ask local ypbind for NIS server */
#if defined (HAVE_YPBIND3)
	  ret = rpc_call ("localhost", YPBINDPROG, YPBINDVERS_2,
			  YPBINDPROC_DOMAIN,
			  (xdrproc_t) xdr_domainname, (caddr_t) &domainname,
			  (xdrproc_t) xdr_ypbind2_resp, (caddr_t) &yp2_r,
			  "udp");
#else
	  ret = rpc_call ("localhost", YPBINDPROG, YPBINDVERS,
			  YPBINDPROC_DOMAIN,
			  (xdrproc_t) xdr_domainname, (caddr_t) &domainname,
			  (xdrproc_t) xdr_ypbind_resp, (caddr_t) &yp2_r,
			  "udp");
#endif
	  if (ret == RPC_SUCCESS)
	    {
	      static char straddr[INET_ADDRSTRLEN];
	      struct sockaddr_in sa;
	      sa.sin_family = AF_INET;
	      sa.sin_addr =
		yp2_r.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;

	      inet_ntop(sa.sin_family, &sa.sin_addr,
			straddr, sizeof(straddr));

	      hostname = straddr;
	    }
	}

      /* Both rpc_call fail */
      if (ret != RPC_SUCCESS)
	{
	  fprintf (stderr, _("Error getting NIS server (%i)\n"), ret);
	  return 1;
	}
    }

  client = clnt_create (hostname, YPPROG, YPVERS, "udp");
  if (client == NULL)
    {
      fprintf (stderr, _("Cannot contact %s, no NIS server running or wrong protocol?\n"),
	       hostname);
#if 0
      /* if we failed, print out all appropriate error messages and exit */
      fprintf(stderr, "Error calling clnt_create()\n");
      fprintf(stderr, "PROG: %lu\tVERS: %lu\tNET: %s\n",
              YPPROG, YPVERS, "udp");
      fprintf(stderr, "clnt_stat: %d\n", rpc_createerr.cf_stat);
      fprintf(stderr, "re_errno: %d\n", rpc_createerr.cf_error.re_errno);
#endif
      return 1;
   }

  result = clnt_call (client, YPPROC_DOMAIN, (xdrproc_t) xdr_domainname,
		      (caddr_t) &domainname,  (xdrproc_t) xdr_bool,
		      (caddr_t) &clnt_res, RPCTIMEOUT);
  if (result != RPC_SUCCESS)
    {
      fprintf (stderr, _("Can't create connection to %s.\n"),
	       hostname ? hostname : "unknown");
      clnt_perror (client, _("Reason"));
      return 1;
    }

  if (clnt_res != TRUE)
    {
      fprintf (stdout, _("Domain %s is not supported by %s.\n"), domainname,
	       hostname ? hostname : "unknown");
      return 1;
    }

  req.domain = domainname;
  req.map = argv[0];
  memset (&resp_o, '\0', sizeof (resp_o));
  res1 = clnt_call (client, YPPROC_ORDER, (xdrproc_t) xdr_ypreq_nokey,
		    (caddr_t) &req, (xdrproc_t) xdr_ypresp_order,
		    (caddr_t) &resp_o, RPCTIMEOUT);
  if (res1 == 0 && resp_o.status != YP_TRUE)
    res1 = ypprot_err (resp_o.status);
  else
    order = resp_o.ordernum;
  xdr_free ((xdrproc_t) xdr_ypresp_order, (char *) &resp_o);

  memset (&resp_m, '\0', sizeof (resp_m));
  res2 =  clnt_call (client, YPPROC_MASTER, (xdrproc_t) xdr_ypreq_nokey,
		     (caddr_t) &req, (xdrproc_t) xdr_ypresp_master,
		     (caddr_t) &resp_m, RPCTIMEOUT);
  if (res2 == 0 && resp_m.status != YP_TRUE)
    res2 = ypprot_err (resp_m.status);
  else
    master = strdup (resp_m.master);
  xdr_free ((xdrproc_t) xdr_ypresp_master, (char *) &resp_m);


  if (res1 && res2)
    fputs (_("Can't get any map parameter information.\n"), stderr);
  else
    fprintf (stdout, _("Domain %s is supported.\n"), domainname);

  if (res1)
    {
      fprintf (stderr, _("Can't get order number for map %s.\n"), argv[0]);
      fprintf (stderr, _("\tReason: %s\n"), yperr_string (res1));
    }
  else
    {
      char *c = strdup (ctime (&order));

      c[strlen(c)-1] = '\0';
      fprintf (stdout, _("Map %s has order number %ld. [%s]\n"), argv[0],
	       (long) order, c);
    }

  if (res2)
    {
      fprintf (stderr, _("Can't get master for map %s.\n"), argv[0]);
      fprintf (stderr, _("\tReason: %s\n"), yperr_string (res2));
    }
  else
    fprintf (stdout, _("The master server is %s.\n"), master);


  return res1 || res2;
}
