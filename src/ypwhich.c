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

#include <getopt.h>
#include <locale.h>
#include <libintl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include "lib/nicknames.h"

/* from ypbind-mt/ypbind.h */
#define YPBINDPROC_OLDDOMAIN 1
extern int yp_maplist (const char *, struct ypmaplist **);

#ifndef _
#define _(String) gettext (String)
#endif

/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (void)
{
  fprintf (stdout, "ypwhich (%s) %s\n", PACKAGE, VERSION);
  fprintf (stdout, gettext ("\
Copyright (C) %s Thorsten Kukuk.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2014");
  /* fprintf (stdout, _("Written by %s.\n"), "Thorsten Kukuk"); */
}

static void
print_usage (FILE *stream)
{
  fputs (_("Usage: ypwhich [-d domain] [[-t] -m [mname]|[-n]|[-Vn] hostname] | -x\n"),
	 stream);
}

static void
print_help (void)
{
  print_usage (stdout);
  fputs (_("ypwhich - return name of NIS server or map master\n\n"), stdout);

  fputs (_("  -d domain      Use 'domain' instead of the default domain\n"),
	 stdout);
  fputs (_("  -m mname       Find the master NIS server for the map 'mname'\n"),
	 stdout);
  fputs (_("  -n             Don't convert addresses to names\n"), stdout);
  fputs (_("  -t             Inhibits map nickname translation\n"), stdout);
  fputs (_("  -V n           Version of ypbind, V3 is default\n"), stdout);
  fputs (_("  -x             Display the map nickname translation table\n"),
	 stdout);
  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (void)
{
  const char *program = "ypwhich";

  print_usage (stderr);
  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   program, program);
}

/* bind to a special host and print the name ypbind running on this host
   is bound to */
static int
print_bindhost (char *hostname, char *domain, int vers, int nflag)
{
  int ret;
  struct timeval tv;
  CLIENT *client;

  client = clnt_create(hostname, YPBINDPROG, vers, "udp");
  if (client == NULL)
    {
      fprintf (stderr, "ypwhich: %s\n", yperr_string (YPERR_YPBIND));
#if 0
      fprintf(stderr, "Error calling clnt_create()\n");
      fprintf(stderr, "PROG: %i\tVERS: %i\tNET: %s\n",
              YPBINDPROG, vers, "upd");
      fprintf(stderr, "clnt_stat: %d\n", rpc_createerr.cf_stat);
      fprintf(stderr, "re_errno: %d\n", rpc_createerr.cf_error.re_errno);
#endif
      return 1;
    }

  tv.tv_sec = 15;
  tv.tv_usec = 0;

  if (vers < 3)
    {
      struct ypbind2_resp yp2_r;
      struct sockaddr_in sa;
      char host[NI_MAXHOST];

      memset (&yp2_r, 0, sizeof (struct ypbind2_resp));

      if (vers == 1)
	ret = clnt_call (client, YPBINDPROC_OLDDOMAIN,
			 (xdrproc_t) xdr_domainname,
			 (caddr_t) &domain, (xdrproc_t) xdr_ypbind2_resp,
			 (caddr_t) &yp2_r, tv);
      else
	ret = clnt_call (client, YPBINDPROC_DOMAIN, (xdrproc_t) xdr_domainname,
			 (caddr_t) &domain, (xdrproc_t) xdr_ypbind2_resp,
			 (caddr_t) &yp2_r, tv);

      if (ret != RPC_SUCCESS)
	{
	  char *err = NULL;

	  asprintf (&err, _("ypwhich: can't call ypbind on '%s'"), hostname);
	  (void) clnt_perror(client, err);
	  free (err);
	  clnt_destroy (client);
	  return 1;
	}
      else
	{
	  if (yp2_r.ypbind_status != YPBIND_SUCC_VAL ||
	      /* that's an ugly hack, but ypbind of Solaris reports for V1 and V2
		 YPBIND_SUCC_VAL if it is bound to a server with IPv6 address. */
	      yp2_r.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port == 0)
	    {
	      fprintf (stderr, _("Error from ypbind on '%s': %s\n"),
		       hostname,
		       ypbinderr_string (yp2_r.ypbind_respbody.ypbind_error));
	      clnt_destroy (client);
	      return 1;
	    }
	}
      clnt_destroy (client);

      sa.sin_family = AF_INET;
      sa.sin_addr =  yp2_r.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;

      if (!nflag && getnameinfo((struct sockaddr *)&sa, sizeof sa,
				host, sizeof host, NULL, 0, 0) == 0)
	printf ("%s\n", host);
      else
	{
	  char straddr[INET_ADDRSTRLEN];
	  inet_ntop(sa.sin_family, &sa.sin_addr, straddr, sizeof(straddr));
	  printf ("%s\n", straddr);
	}
    }
  else /* YPBINDVERS >= 3 */
    {
      struct ypbind3_resp yp3_r;

      memset (&yp3_r, 0, sizeof (struct ypbind3_resp));

      ret = clnt_call (client, YPBINDPROC_DOMAIN, (xdrproc_t) xdr_domainname,
		       (caddr_t) &domain, (xdrproc_t) xdr_ypbind3_resp,
		       (caddr_t) &yp3_r, tv);

      if (ret != RPC_SUCCESS)
	{
	  char *err = NULL;

	  /* if we have a RPC version mismatch, try version 2 */
	  if (ret == RPC_PROGVERSMISMATCH)
	    return print_bindhost (hostname, domain, 2, nflag);

	  asprintf (&err, _("ypwhich: can't call ypbind on '%s'"), hostname);
	  (void) clnt_perror(client, err);
	  free (err);
	  clnt_destroy (client);
	  return 1;
	}
      else
	{
	  if (yp3_r.ypbind_status != YPBIND_SUCC_VAL)
	    {
	      fprintf (stderr, _("Error from ypbind on '%s': %s\n"),
				 hostname,
				 ypbinderr_string (yp3_r.ypbind_respbody.ypbind_error));
	      clnt_destroy (client);
	      return 1;
	    }
	}
      clnt_destroy (client);

      if (!(nflag && yp3_r.ypbind3_nconf && yp3_r.ypbind3_svcaddr)
	  && yp3_r.ypbind3_servername)
	printf ("%s\n", yp3_r.ypbind3_servername);
      else if (yp3_r.ypbind3_nconf && yp3_r.ypbind3_svcaddr)
	{
	  if (nflag)
	    {
	      char namebuf6[INET6_ADDRSTRLEN];

	      printf ("%s\n", taddr2ipstr (yp3_r.ypbind3_nconf,
					   yp3_r.ypbind3_svcaddr,
					   namebuf6, sizeof namebuf6));
	    }
	  else
	    {
	      char hostbuf[NI_MAXHOST];
	      const char *host;

	      host = taddr2host (yp3_r.ypbind3_nconf, yp3_r.ypbind3_svcaddr,
				 hostbuf, sizeof hostbuf);

	      if (host)
		printf ("%s\n", host);
	      else
		{
		  fprintf (stderr, _("ERROR: taddr2host failed!\n"));
		  return 1;
		}
	    }
	}
      else
	{
	  fprintf (stderr, _("Error: no server information gotten from ypbind on '%s'\n"),
		   hostname);
	  return 1;
	}

#if 0 /* Dump struct for debugging */
      if (yp3_r.ypbind3_nconf && yp3_r.ypbind3_svcaddr)
	printf ("ypbind_netbuf: %s\n", taddr2uaddr (yp3_r.ypbind3_nconf,
						    yp3_r.ypbind3_svcaddr));
      if (yp3_r.ypbind3_servername)
	printf ("ypbind_servername: %s\n", yp3_r.ypbind3_servername);
      else
	printf ("ypbind_servername: NULL\n");
      printf ("ypbind_hi_vers: %u\n", (u_int32_t)yp3_r.ypbind3_hi_vers);
      printf ("ypbind_lo_vers: %u\n", (u_int32_t)yp3_r.ypbind3_lo_vers);
#endif
    }
  return 0;
}


int
main (int argc, char **argv)
{
  int dflag = 0, mflag = 0, tflag = 0, Vflag = 0, xflag = 0, hflag = 0, nflag=0;
  char *hostname = NULL, *domainname = NULL, *mname = NULL;
  int ypbind_version = 3;

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

      c = getopt_long (argc, argv, "d:mntV:x?", long_options, &option_index);
      if (c == (-1))
        break;
      switch (c)
        {
	case 'd':
	  dflag = 1;
	  domainname = optarg;
	  break;
	case 'm':
	  mflag = 1;
	  break;
	case 'n':
	  nflag = 1;
	  break;
	case 't':
	  tflag = 1;
	  break;
	case 'V':
	  Vflag = 1;
	  ypbind_version = atoi (optarg);
	  if (ypbind_version < 1 || ypbind_version > 3)
	    {
	      print_error ();
	      return 1;
	    }
	  break;
	case 'x':
	  xflag = 1;
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

  if (argc > 1)
    {
      print_error ();
      return 1;
    }
  else if (argc == 1)
    {
      if (mflag)
	mname = argv[0];
      else
	{
	  hflag = 1;
	  hostname = argv[0];
	}
    }


  if ((xflag && (dflag || mflag || nflag || tflag || Vflag || hflag)) ||
      ((tflag || mflag) && (Vflag || hflag)) || (tflag && !mflag))
    {
      print_error ();
      return 1;
    }

  if (xflag)
    print_nicknames();
  else
    {
      if (domainname == NULL)
	{
	  int error;

	  if ((error = yp_get_default_domain (&domainname)) != 0)
	    {
	      fprintf (stderr, _("%s: can't get local yp domain: %s\n"),
		       "ypwhich", yperr_string (error));
	      return 1;
	    }
	}

      if (mflag)
	{ /* The use has given us the -m flag */
	  char *master;
	  int ret;

	  if (mname)
	    { /* We now the name of the map the user wishes */
	      const char *map;

	      if (!tflag)
		map = getypalias (mname);
	      else
		map = mname;

	      ret = yp_master (domainname, map, &master);
	      switch (ret)
		{
		case YPERR_SUCCESS:
		  printf ("%s\n", master);
		  free (master);
		  break;
		case YPERR_YPBIND:
		  fprintf (stderr, _("No running ypbind\n"));
		  return 1;
		default:
		  fprintf (stderr,
			   _("Can't find master for map \"%s\". Reason: %s\n"),
			   map, yperr_string (ret));
		  return 1;
		}
	    }
	  else
	    { /* Show the master for all maps */
	      struct ypmaplist *ypmap = NULL, *y, *old;

	      ret = yp_maplist (domainname, &ypmap);
	      switch (ret)
		{
		case YPERR_SUCCESS:
		  for (y = ypmap; y;)
		    {
		      ret = yp_master (domainname, y->map, &master);
		      switch (ret)
			{
			case YPERR_SUCCESS:
			  printf ("%s %s\n", y->map, master);
			  free (master);
			  break;
			default:
			  fprintf (stderr,
			      _("Can't find master for map %s. Reason: %s\n"),
				   y->map, yperr_string (ret));
			  break;
			}
		      old = y;
		      y = y->next;
		      free (old);
		    }
		  break;
		case YPERR_YPBIND:
		  fprintf (stderr, _("No running ypbind.\n"));
		  return 1;
		default:
		  fprintf (stderr,
			   _("Can't get map list for domain %s. Reason: %s\n"),
			   domainname, yperr_string (ret));
		  return 1;
		}
	    }
	}
      else
	{
	  if (!hflag)
	    hostname = "localhost";

	  if (print_bindhost (hostname, domainname, ypbind_version, nflag))
	    return 1;
	}
    }

  return 0;
}
