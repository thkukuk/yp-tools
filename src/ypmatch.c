/* Copyright (C) 1998 Thorsten Kukuk
   This file is part of the yp-tools.
   Author: Thorsten Kukuk <kukuk@vt.uni-paderborn.de>

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

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif
#include <locale.h>
#include <libintl.h>
#include <string.h>
#include "lib/yp-tools.h"
#include "lib/nicknames.h"

#ifndef _
#define _(String) gettext (String)
#endif

/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (void)
{
  fprintf (stdout, "ypmatch (%s) %s\n", PACKAGE, VERSION);
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
  fputs (_("Usage: ypmatch [-d domain] [-kt] key ... mapname | -x\n"),
	 stream);
}

static void
print_help (void)
{
  print_usage (stdout);
  fputs (_("ypmatch - print the values of one or more keys from a NIS map\n\n"), stdout);

  fputs (_("  -d domain      Use 'domain' instead of the default domain\n"),
	 stdout);
  fputs (_("  -k             Display map keys\n"), stdout);
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
  const char *program = "ypmatch";

  print_usage (stderr);
  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   program, program);
}


int
main (int argc, char **argv)
{
  int dflag = 0, kflag = 0, tflag = 0, xflag = 0;
  char *domainname = NULL;

  setlocale (LC_MESSAGES, "");
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

      c = getopt_long (argc, argv, "d:ktx?", long_options, &option_index);
      if (c == (-1))
        break;
      switch (c)
        {
	case 'd':
	  dflag = 1;
	  domainname = optarg;
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

  if (argc < 2 && !xflag)
    {
      print_error ();
      return 1;
    }

  if (xflag && (dflag || kflag || tflag))
    {
      print_error ();
      return 1;
    }

  if (xflag)
    print_nicknames();
  else
    {
      const char *map;
      int i;

      if (domainname == NULL)
	{
	  int error;

	  if ((error = yp_get_default_domain (&domainname)) != 0)
	    {
	      fprintf (stderr, _("%s: can't get local yp domain: %s\n"),
		       "ymatch", yperr_string (error));
	      return 1;
	    }
	}

      if (!tflag)
	map = getypalias (argv[argc - 1]);
      else
	map = argv[argc - 1];

      for (i = 0; i < (argc - 1); ++i)
	{
	  char *val = NULL;
	  int vallen, res;

	  res = yp_match (domainname, map, argv[i], strlen (argv[i]),
			  &val, &vallen);
	  if (res == YPERR_KEY)
	    res = yp_match (domainname, map,  argv[i], strlen (argv[i]) + 1,
			    &val, &vallen);
	  switch (res)
	    {
	    case YPERR_SUCCESS:
	      if (val[vallen - 1] == '\0')
		--vallen;
	      if (kflag)
		printf ("%s ", argv[i]);
	      printf ("%*.*s\n", vallen, vallen, val);
	      free (val);
	      break;
	    case YPERR_YPBIND:
	      fprintf (stderr, _("No running ypbind\n"));
	      return 1;
	    default:
	      fprintf (stderr, _("Can't match key %s in map %s. Reason: %s\n"),
		       argv[i], map, yperr_string (res));
	      return 1;
	    }
	}
    }
  return 0;
}
