/* Copyright (C) 1998, 1999, 2001 Thorsten Kukuk
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
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif
#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lib/yp-tools.h"

#if defined (__NetBSD__) || (defined(__GLIBC__) && (__GLIBC__ == 2 && __GLIBC_MINOR__ == 0))
/* <rpc/rpc.h> is missing the prototype */
int getrpcport(char *host, int prognum, int versnum, int proto);
#endif

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
"), "1998");
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
  fputs (_("  -h hostname    Set ypbind's binding on ´hostname´\n"), stdout);
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

/* bind to a special host and search the ypserv data */
static int
bind_tohost (struct sockaddr_in *sock_in, char *domainname, char *server,
	     char *host)
{
  struct ypbind_setdom ypsd;
  const struct timeval tv = {15, 0};
  struct hostent *hp;
  CLIENT *client;
  int sock, port;
  int res;
  struct in_addr server_addr;

  if ((port = htons (getrpcport (server, YPPROG, YPPROC_NULL, IPPROTO_UDP)))
      == 0)
    {
      fprintf (stderr, _("%s not running ypserv.\n"), server);
      exit (1);
    }

  memset (&ypsd, '\0', sizeof (ypsd));

  if ((hp = gethostbyname (server)) != NULL)
    memcpy (&ypsd.ypsetdom_binding.ypbind_binding_addr, hp->h_addr_list[0],
	    sizeof (ypsd.ypsetdom_binding.ypbind_binding_addr));
  else if (inet_aton (server, &server_addr) == 0)
    {
      fprintf (stderr, _("can't find address for %s\n"), server);
      exit (1);
    }
  else
    memcpy (&ypsd.ypsetdom_binding.ypbind_binding_addr, &server_addr.s_addr,
	    sizeof (server_addr.s_addr));

  ypsd.ypsetdom_domain = domainname;
  ypsd.ypsetdom_binding.ypbind_binding_port = port;
  ypsd.ypsetdom_vers = YPVERS;

  sock = RPC_ANYSOCK;
  client = clntudp_create (sock_in, YPBINDPROG, YPBINDVERS, tv, &sock);
  if (client == NULL)
    {
      fprintf (stderr, _("can't yp_bind: Reason: %s\n"),
               yperr_string (YPERR_YPBIND));
      return YPERR_YPBIND;
    }

  client->cl_auth = authunix_create_default ();

  res = clnt_call (client, YPBINDPROC_SETDOM,
		   (xdrproc_t) ytxdr_ypbind_setdom, (caddr_t) &ypsd,
		   (xdrproc_t) xdr_void, NULL, tv);
  if (res)
    {
      fprintf (stderr, _("Cannot ypset for domain %s on host %s.\n"),
               domainname, host);
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
      struct sockaddr_in sock_in;
      char *server = argv[0];

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

      memset (&sock_in, '\0', sizeof (sock_in));
      sock_in.sin_family = AF_INET;
      if (hostname != NULL)
	{

	  if (inet_aton (hostname, &sock_in.sin_addr) == 0)
	    {
	      struct hostent *hent = gethostbyname (hostname);

	      if (hent == NULL)
		{
		  fprintf (stderr, _("ypset: host %s unknown\n"), hostname);
		  return 1;
		}
	      memcpy (&sock_in.sin_addr, hent->h_addr_list[0],
		      sizeof (sock_in.sin_addr));
	    }
	}
      else
	{
	  sock_in.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
	}
      if (bind_tohost (&sock_in, domainname, server, hostname))
	return 1;
    }

  return 0;
}
