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

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif
#include <locale.h>
#include <libintl.h>
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
  fprintf (stdout, "ypcat (%s) %s\n", PACKAGE, VERSION);
  fprintf (stdout, gettext ("\
Copyright (C) %s Thorsten Kukuk.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "1998, 1999, 2001");
  fprintf (stdout, _("Written by %s.\n"), "Thorsten Kukuk");
}

static void
print_usage (FILE *stream)
{
  fputs (_("Usage: ypcat [-kt] [-d domain] mapname | -x\n"),
	 stream);
}

static void
print_help (void)
{
  print_usage (stdout);
  fputs (_("ypcat - print values of all keys in a NIS database\n\n"), stdout);

  fputs (_("  -d domain      Use 'domain' instead of the default domain\n"),
	 stdout);
  fputs (_("  -h hostname    Query ypserv on 'hostname' instead the current one\n"),
	 stdout);
  fputs (_("  -k             Display map keys\n"),
	 stdout);
  fputs (_("  -t             Inhibits map nickname translation\n"), stdout);
  fputs (_("  -x             Display the map nickname translation table\n"),
	 stdout);
  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (void)
{
  const char *program = "ypcat";
  print_usage (stderr);
  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   program, program);
}

static int kflag = 0;

static int
print_data (int status, char *inkey, int inkeylen, char *inval,
	    int invallen, char *indata __attribute__ ((unused)))
{
  if (status != YP_TRUE)
    return status;

  if (kflag  && (inkeylen > 0))
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
  int dflag = 0, hflag = 0, mflag = 0, tflag = 0, xflag = 0;
  char *domainname = NULL;
  char *hostname = NULL;

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

      c = getopt_long (argc, argv, "d:h:ktx?", long_options, &option_index);
      if (c == (-1))
        break;
      switch (c)
        {
	case 'd':
	  dflag = 1;
	  domainname = optarg;
	  break;
	case 'h':
	  hflag = 1;
	  hostname = optarg;
	  break;
	case 'k':
	  kflag = 1;
	  break;
	case 't':
	  tflag = 1;
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
  else
    if (argc == 1)
      mflag = 1;

  if ((xflag && (hflag || dflag || mflag || tflag)) || (!xflag && !mflag))
    {
      print_error ();
      return 1;
    }

  if (xflag)
    print_nicknames();
  else
    {
      struct ypall_callback ypcb;
      const char *map;
      int res;

      if (domainname == NULL)
	{
	  int error;

	  if ((error = yp_get_default_domain (&domainname)) != 0)
	    {
	      fprintf (stderr, _("%s: can't get local yp domain: %s\n"),
		       "ypcat", yperr_string (error));
	      return 1;
	    }
	}

      if (!tflag)
	map = getypalias (argv[0]);
      else
	map = argv[0];

      ypcb.foreach = print_data;
      ypcb.data = NULL;

      if (hflag)
	res = yp_all_host (hostname, domainname, map, &ypcb);
      else
	res = yp_all (domainname, map, &ypcb);
      switch (res)
	{
	case YPERR_SUCCESS:
	  break;
	case YPERR_YPBIND:
	  fprintf (stderr, _("No running ypbind\n"));
	  return 1;
	default:
	  fprintf (stderr, _("No such map %s. Reason: %s\n"),
		   map, yperr_string (res));
	  return 1;
	}
    }
  return 0;
}
