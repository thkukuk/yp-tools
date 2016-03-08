/* Copyright (C) 2014, 2016 Thorsten Kukuk
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

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <libintl.h>
#include <locale.h>
#include <getopt.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>

#ifndef _
#define _(String) gettext (String)
#endif

#ifndef BINDINGDIR
# define BINDINGDIR "/var/yp/binding"
#endif

#if !defined(HAVE_YPBIND3)
#define ypbind2_resp ypbind_resp
#endif

/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (void)
{
  fprintf (stdout, "yp_dump_binding (%s) %s\n", PACKAGE, VERSION);
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
  fputs (_("Usage: yp_dump_binding [-d domain]\n"), stream);
}

static void
print_help (void)
{
  print_usage (stdout);
  fputs (_("yp_dump_binding - print the used NIS servers from binding directory\n\n"),
	 stdout);
  fputs (_("  -d domain      Use 'domain' instead of the default domain\n"),
         stdout);
  fputs (_("  -p path        Use 'path' instead of the default binding directory\n"),
	 stdout);
  fputs (_(" -v version     Only dump binding information of this ypbind protocol version\n"),
	 stdout);
  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (void)
{
  const char *program = "yp_dump_binding";
  print_usage (stderr);
  fprintf (stderr,
           _("Try `%s --help' or `%s --usage' for more information.\n"),
           program, program);
}

#if defined(HAVE_YPBIND3)
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
#endif

static void
dump_binding (const char *dir, const char *domain, int version)
{
  char path[sizeof (dir) + strlen (domain) + 3 * sizeof (unsigned) + 3];

  snprintf (path, sizeof (path), "%s/%s.%u", dir, domain, version);

#if defined(HAVE_YPBIND3)
  if (version == 3)
    {
      FILE *in = fopen (path, "rce");
      if (in != NULL)
	{
	  struct ypbind3_binding ypb3;
	  bool_t status;

	  XDR xdrs;
	  xdrstdio_create (&xdrs, in, XDR_DECODE);
	  memset (&ypb3, 0, sizeof (ypb3));
	  status = xdr_ypbind3_binding (&xdrs, &ypb3);
	  xdr_destroy (&xdrs);

	  if (!status)
	    fprintf (stderr, _("Error reading %s\n"), path);
	  else
	    {
	        char buf[INET6_ADDRSTRLEN];

		printf ("Dump of %s:\n", path);
		printf ("\typbind_nconf:\n");
		if (ypb3.ypbind_nconf)
		  dump_nconf (ypb3.ypbind_nconf, "\t\t");
		else
		  printf ("\t\tNULL\n");

		printf ("\typbind_svcaddr: %s:%i\n",
			taddr2ipstr (ypb3.ypbind_nconf, ypb3.ypbind_svcaddr,
				     buf, sizeof (buf)),
			taddr2port (ypb3.ypbind_nconf, ypb3.ypbind_svcaddr));

		printf ("\typbind_servername: ");
		if (ypb3.ypbind_servername)
		  printf ("%s\n", ypb3.ypbind_servername);
		else
		  printf ("\tNULL\n");
		printf ("\typbind_hi_vers: %lu\n", (u_long) ypb3.ypbind_hi_vers);
		printf ("\typbind_lo_vers: %lu\n", (u_long) ypb3.ypbind_lo_vers);
	    }
	  xdr_free ((xdrproc_t)xdr_ypbind3_binding, &ypb3);
	  fclose (in);
	}
      else
	fprintf (stderr, _("Error opening %s: %m\n"), path);
    }
  else
#endif
    {
      int fd;

      fd = open (path, O_RDONLY);
      if (fd >= 0)
	{
	  struct ypbind2_resp ypbr;

	  if (pread (fd, &ypbr, sizeof (ypbr), 2) == sizeof (ypbr))
	    {
	      char straddr[INET_ADDRSTRLEN];
	      struct sockaddr_in sa;
	      sa.sin_family = AF_INET;
	      sa.sin_addr =  ypbr.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;
	      inet_ntop(sa.sin_family, &sa.sin_addr, straddr, sizeof(straddr));

	      printf ("Dump of %s:\n", path);
	      printf ("\tAddress: %s\n", straddr);
	      printf ("\tPort: %i\n", ntohs (ypbr.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port));
	    }
	  else
	    fprintf (stderr, _("Error reading %s: %m\n"), path);

	  close (fd);
	}
      else
	fprintf (stderr, _("Error opening %s: %m\n"), path);
    }
}

int
main (int argc, char **argv)
{
  char *bindingdir = BINDINGDIR;
  char *domainname = NULL;
  int vers = 0;

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

      c = getopt_long (argc, argv, "d:p:v:?", long_options, &option_index);
      if (c == (-1))
        break;
      switch (c)
        {
        case 'd':
          domainname = optarg;
          break;
	case 'p':
	  bindingdir = optarg;
	  break;
	case 'v':
	  vers = atoi (optarg);
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

  if (domainname == NULL)
    {
      int error;

      if ((error = yp_get_default_domain (&domainname)) != 0)
	{
	  fprintf (stderr, _("%s: can't get local yp domain: %s\n"),
		   "yp_dump_binding", yperr_string (error));
	  return 1;
	}
    }

  if (vers == 0 || vers == 1)
    dump_binding (bindingdir, domainname, 1);
  if (vers == 0 || vers == 2)
    dump_binding (bindingdir, domainname, 2);
  if (vers == 0 || vers == 3)
    dump_binding (bindingdir, domainname, 3);

  return 0;
}
