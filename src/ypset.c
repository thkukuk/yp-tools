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

#include <netdb.h>
#include <stdio.h>
#include <getopt.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>

#include "internal.h"

#ifndef _
#define _(String) gettext (String)
#endif

/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (void)
{
  fprintf (stdout, "ypset (%s) %s\n", PACKAGE, VERSION);
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
  fputs (_("Usage: ypset [-d domain] [-h hostname] server\n"), stream);
}

static void
print_help (void)
{
  print_usage (stdout);
  fputs (_("ypset - bind ypbind to a particular NIS server\n\n"), stdout);

  fputs (_("  -d domain      Use 'domain' instead of the default domain\n"),
	 stdout);
  fputs (_("  -h hostname    Set ypbind's binding on 'hostname'\n"), stdout);
  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (void)
{
  const char *program = "ypset";

  print_usage (stderr);
  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   program, program);
}

/* bind to a special host and set a new NIS server */
static int
bind_tohost_v2 (const char *hostname, char *domainname, char *new_server)
{
  struct ypbind2_setdom ypsd;
  const struct timeval tv = {15, 0};
  struct hostent *hp;
  CLIENT *client;
  int port;
  int16_t port16;
  int res;
  struct in_addr server_addr;

  if ((port = htons (getrpcport (new_server, YPPROG, YPPROC_NULL, IPPROTO_UDP)))
      == 0)
    {
      fprintf (stderr, _("%s not running ypserv.\n"), new_server);
      exit (1);
    }

  memset (&ypsd, '\0', sizeof (ypsd));

  if ((hp = gethostbyname2 (new_server, AF_INET)) != NULL)
    memcpy (&ypsd.ypsetdom_binding.ypbind_binding_addr, hp->h_addr_list[0],
	    sizeof (ypsd.ypsetdom_binding.ypbind_binding_addr));
  else if (inet_aton (new_server, &server_addr) == 0)
    {
      fprintf (stderr, _("can't find IPv4 address for %s\n"), new_server);
      exit (1);
    }
  else
    memcpy (&ypsd.ypsetdom_binding.ypbind_binding_addr, &server_addr.s_addr,
	    sizeof (server_addr.s_addr));

  ypsd.ypsetdom_domain = domainname;
  port16 = port;
  memcpy (&ypsd.ypsetdom_binding.ypbind_binding_port, &port16,
	  sizeof (ypsd.ypsetdom_binding.ypbind_binding_port));
  ypsd.ypsetdom_vers = YPVERS;

  client = clnt_create (hostname, YPBINDPROG, YPBINDVERS_2, "udp");
  if (client == NULL)
    {
      fprintf (stderr, _("can't yp_bind: Reason: %s\n"),
               yperr_string (YPERR_YPBIND));
      return YPERR_YPBIND;
    }

  client->cl_auth = authunix_create_default ();

  res = clnt_call (client, YPBINDPROC_SETDOM,
		   (xdrproc_t) xdr_ypbind2_setdom, (caddr_t) &ypsd,
		   (xdrproc_t) xdr_void, NULL, tv);
  if (res)
    {
      fprintf (stderr, _("Cannot ypset for domain %s on host %s.\n"),
               domainname, hostname);
      clnt_perror (client, _("Reason"));
      clnt_destroy (client);
      return YPERR_YPBIND;
    }
  clnt_destroy (client);
  return 0;
}

/* bind to a special host and set a new NIS server */
static int
bind_tohost_v3 (const char *hostname, char *domainname, char *new_server)
{
  const struct timeval tv = {15, 0};
  struct ypbind3_setdom ypsd;
  enum clnt_stat res;
  CLIENT *client;

  client = clnt_create (hostname, YPBINDPROG, YPBINDVERS, "udp");
  /* if V3 protocol does not work, try v2 as fallback */
  if (client == NULL)
    bind_tohost_v2 (hostname, domainname, new_server);
#if 0
    {
      fprintf (stderr, _("can't yp_bind: Reason: %s\n"),
               yperr_string (YPERR_YPBIND));
      return YPERR_YPBIND;
    }
#endif

  memset (&ypsd, '\0', sizeof (ypsd));
  ypsd.ypsetdom_domain = domainname;
  ypsd.ypsetdom_bindinfo = __host2ypbind3_binding (new_server);
  if (ypsd.ypsetdom_bindinfo == NULL)
    {
      fprintf (stderr, _("%s not running ypserv.\n"), new_server);
      exit (1);
    }

  /* Create unix credentials */
  client->cl_auth = authunix_create_default ();

  res = clnt_call (client, YPBINDPROC_SETDOM,
		   (xdrproc_t) xdr_ypbind3_setdom, (caddr_t) &ypsd,
		   (xdrproc_t) xdr_void, NULL, tv);
  if (res)
    {
      fprintf (stderr, _("Cannot ypset for domain %s on host %s.\n"),
               domainname, hostname);
      clnt_perror (client, _("Reason"));
      clnt_destroy (client);
      return YPERR_YPBIND;
    }
  clnt_destroy (client);
  return 0;
}


int
main (int argc, char **argv)
{
  char *hostname = NULL, *domainname = NULL;

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
  else
    {
      char *new_server = argv[0];

      if (domainname == NULL)
	{
	  int error;

	  if ((error = yp_get_default_domain (&domainname)) != 0)
	    {
	      fprintf (stderr, _("%s: can't get local yp domain: %s\n"),
		       "ypset", yperr_string (error));
	      return 1;
	    }
	}

      if (hostname == NULL)
	hostname = "localhost";

      if (bind_tohost_v3 (hostname, domainname, new_server))
	return 1;
    }

  return 0;
}
