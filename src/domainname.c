/* Copyright (C) 1998, 1999, 2001 Thorsten Kukuk
   This file is part of the yp-tools.
   Author: Thorsten Kukuk <kukuk@suse.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif
#include <locale.h>
#include <libintl.h>
#include <rpcsvc/ypclnt.h>

#ifndef _
#define _(String) gettext (String)
#endif

/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (const char *progname)
{
  fprintf (stdout, "%s (%s) %s\n", progname, PACKAGE, VERSION);
  fprintf (stdout, gettext ("\
Copyright (C) %s Thorsten Kukuk.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "1998");
  /* fprintf (stdout, _("Written by %s.\n"), "Thorsten Kukuk"); */
}

static void
print_usage (FILE *stream, const char *progname)
{
  fprintf (stream, _("Usage: %s [domain]\n"), progname);
}

static void
print_help (void)
{
  print_usage (stdout, "domainname");
  fputs (_("domainname - set or display name of current domain\n\n"), stdout);

  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_help_yp (const char *progname)
{
  print_usage (stdout, progname);
  fprintf (stderr,
	   _("%s - set or display name of current NIS domain\n\n"),
	   progname);

  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (const char *progname)
{
  print_usage (stderr, progname);
  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   progname, progname);
}

int
main (int argc, char **argv)
{
  char *progname, *s;
  int use_nis = 0;

  setlocale (LC_MESSAGES, "");
  setlocale (LC_CTYPE, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  if ((s = strrchr (argv[0], '/')) != NULL)
    progname = s + 1;
  else
    progname = argv[0];

  if (strcmp (progname, "ypdomainname") == 0 ||
      strcmp (progname, "nisdomainname") == 0)
    use_nis = 1;

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

      c = getopt_long (argc, argv, "?", long_options, &option_index);
      if (c == (-1))
        break;
      switch (c)
        {
	case '?':
	  if (use_nis)
	    print_help_yp (progname);
	  else
	    print_help ();
	  return 0;
	case '\255':
	  print_version (progname);
	  return 0;
	case '\254':
	  print_usage (stdout, progname);
	  return 0;
	default:
	  print_usage (stderr, progname);
	  return 1;
	}
    }

  argc -= optind;
  argv += optind;

  if (argc > 1)
    {
      print_error (progname);
      return 1;
    }
  else if (argc == 1)
    {
      char buf[strlen (argv[0]) + 1];
      char *p;

      strcpy (buf, argv[0]);
      if ((p = strchr (buf, '\n')) != NULL)
	*p = 0;
      if (setdomainname (buf, strlen (buf)) < 0)
	{
	  perror ("setdomainname");
	  return 1;
	}
    }
  else
    {
      if (use_nis)
	{
	  char *domainname;
	  int error;

	  if ((error = yp_get_default_domain (&domainname)) != 0)
	    {
	      fputs (yperr_string (error), stderr);
	      fputs ("\n", stderr);
	      return 1;
	    }
	  fputs (domainname, stdout);
	}
      else
	{
	  char buf[256];
	  if (getdomainname (buf, 256) < 0)
	    {
	      perror ("getdomainname");
	      return 1;
	    }
	  fputs (buf, stdout);
	}
      fputs ("\n", stdout);
    }

  return 0;
}
