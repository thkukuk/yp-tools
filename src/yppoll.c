/* Copyright (C) 1998, 1999, 2001, 2014 Thorsten Kukuk
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
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/yp.h>
#include <rpcsvc/ypclnt.h>

#ifndef _
#define _(String) gettext (String)
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
"), "1998");
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
  static struct timeval UDPTIMEOUT = {5, 0};
  char *hostname = NULL, *domainname = NULL, *master = NULL;
  struct sockaddr_in clnt_saddr;
  int clnt_sock, result;
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

  if (hostname == NULL)
    {
      struct ypbind_resp ypbr;
      struct hostent *hent;

      memset (&clnt_saddr, '\0', sizeof clnt_saddr);
      clnt_saddr.sin_family = AF_INET;
      clnt_saddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
      clnt_sock = RPC_ANYSOCK;
      client = clnttcp_create (&clnt_saddr, YPBINDPROG, YPBINDVERS_2,
			       &clnt_sock, 0, 0);
      if (client == NULL)
	{
	  fputs (_("Couldn't get NIS server"), stderr);
	  fputs (_(" - ypbind not running ?\n"),
		 stderr);
	  return 1;
	}
      if (clnt_call (client, YPBINDPROC_DOMAIN,
		     (xdrproc_t) xdr_domainname, (caddr_t) &domainname,
		     (xdrproc_t) xdr_ypbind_resp,
		     (caddr_t) &ypbr, RPCTIMEOUT) != RPC_SUCCESS)
	{
	  clnt_perror (client, _("Couldn't get NIS server"));
	  clnt_destroy (client);
	  close (clnt_sock);
	  return 1;
	}

      clnt_destroy (client);
      close (clnt_sock);
      if (ypbr.ypbind_status != YPBIND_SUCC_VAL)
	{
	  fputs (_("Couldn't get NIS server"), stderr);
	  fputs (": ", stderr);
	  fputs (ypbinderr_string (ypbr.ypbind_resp_u.ypbind_error),
		 stderr);
	  fputs ("\n", stderr);

	  return 1;
	}
      memset (&clnt_saddr, '\0', sizeof (clnt_saddr));
      clnt_saddr.sin_family = AF_INET;
      memcpy (&clnt_saddr.sin_port,
	      &ypbr.ypbind_resp_u.ypbind_bindinfo.ypbind_binding_port,
	      sizeof (clnt_saddr.sin_port));
      memcpy (&clnt_saddr.sin_addr.s_addr,
	      &ypbr.ypbind_resp_u.ypbind_bindinfo.ypbind_binding_addr,
	      sizeof (clnt_saddr.sin_addr.s_addr));

      hent = gethostbyaddr ((char *)&clnt_saddr.sin_addr.s_addr,
			    sizeof (clnt_saddr.sin_addr.s_addr), AF_INET);
      if (hent)
	{
          hostname = strdup (hent->h_name);
      	}
    }
  else
    {
      struct hostent *hent;

      hent = gethostbyname (hostname);
      if (!hent)
	{
	  switch (h_errno)
	    {
	    case HOST_NOT_FOUND:
	      fprintf (stderr, _("Unknown host: %s\n"), hostname);
	      break;
	    case TRY_AGAIN:
	      fprintf (stderr, _("Host name lookup failure\n"));
	      break;
	    case NO_DATA:
	      fprintf (stderr, _("No address associated with name: %s\n"),
		       hostname);
	      break;
	    case NO_RECOVERY:
	      fprintf (stderr, _("Unknown server error\n"));
	      break;
	    default:
	      fprintf (stderr, _("gethostbyname: Unknown error\n"));
	      break;
	    }
	  return 1;
	}
      clnt_saddr.sin_family = AF_INET;
      memcpy (&clnt_saddr.sin_addr.s_addr,hent->h_addr_list[0],
	      hent->h_length);
      clnt_saddr.sin_port = htons (pmap_getport (&clnt_saddr,
						 YPPROG, YPVERS,
						 IPPROTO_UDP));
    }

  clnt_sock = RPC_ANYSOCK;
  client = clntudp_create (&clnt_saddr, YPPROG, YPVERS, UDPTIMEOUT,
			   &clnt_sock);
  if (client == NULL)
    {
      fprintf (stderr, _("Can't create connection to %s.\n"), hostname ? hostname : "unknown");
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
  if (res1 == 0 && resp_o.stat != YP_TRUE)
    res1 = ypprot_err (resp_o.stat);
  else
    order = resp_o.ordernum;
  xdr_free ((xdrproc_t) xdr_ypresp_order, (char *) &resp_o);

  memset (&resp_m, '\0', sizeof (resp_m));
  res2 =  clnt_call (client, YPPROC_MASTER, (xdrproc_t) xdr_ypreq_nokey,
		     (caddr_t) &req, (xdrproc_t) xdr_ypresp_master,
		     (caddr_t) &resp_m, RPCTIMEOUT);
  if (res2 == 0 && resp_m.stat != YP_TRUE)
    res2 = ypprot_err (resp_m.stat);
  else
    master = strdup (resp_m.peer);
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
