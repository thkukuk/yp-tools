/* Copyright (C) 1998, 1999 Thorsten Kukuk
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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <locale.h>
#include <libintl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "getline.h"
#include "nicknames.h"

#ifndef _
#define _(String) gettext (String)
#endif

#ifndef N_
#define N_(String) String
#endif

struct ypalias
{
  char *alias, *name;
  struct ypalias *next;
};

static struct ypalias *ypaliases = NULL;

static void
load_nicknames (void)
{
  FILE *fp;
  char *line;
  size_t len;
  int i = 0;

  /* Open the nickname file.  */
  fp = fopen (NICKNAMEFILE, "r");
  if (fp == NULL)
    {
      fprintf (stderr, _("nickname file %s does not exist.\n"), NICKNAMEFILE);
      return;
    }

  line = NULL;
  len = 0;
  do
    {
      struct ypalias *ptr;
      ssize_t n;
      char *cp;

      n = yp_getline (&line, &len, fp);
      if (n < 0)
        break;
      ++i;
      if (line[n - 1] == '\n')
        line[n - 1] = '\0';
      cp = strchr (line, '#');
      if (cp != NULL)
        *cp = '\0';

      /* If the line is blank it is ignored.  */
      if (line[0] == '\0')
        continue;

      /* Each line contains an alias and the full name */
      cp = line;
      while (!isspace ((int)*cp) && *cp != '\0')
	++cp;
      if (*cp == '\0')
	{
	  fprintf (stderr, _("Bogus entry in line %d: %s\n"), i, line);
	  continue;
	}
      *cp = '\0';
      ++cp;
      while (isspace ((int)*cp) && *cp != '\0')
	++cp;
      if (*cp == '\0')
	{
	  fprintf (stderr, _("Bogus entry in line %d: %s\n"), i, line);
	  continue;
	}
      ptr = calloc (1, sizeof (struct ypalias));
      ptr->next = ypaliases;
      ptr->alias = strdup (line);
      ptr->name = strdup (cp);
      ypaliases = ptr;
   }
  while (!feof (fp));

  /* Free the buffer.  */
  free (line);
  /* Close configuration file.  */
  fclose (fp);
}

const char *
getypalias (const char *alias)
{
  struct ypalias *ptr;

  if (ypaliases == NULL)
    load_nicknames ();

  ptr = ypaliases;

  while (ptr != NULL)
    {
      if (strcmp (alias, ptr->alias) == 0)
	return ptr->name;
      ptr = ptr->next;
    }
  return alias;
}

void
print_nicknames (void)
{
  struct ypalias *ptr;

  if (ypaliases == NULL)
    load_nicknames ();

  ptr = ypaliases;

  while (ptr != NULL)
    {
      fprintf (stdout, _("Use \"%s\"\tfor map \"%s\"\n"), ptr->alias, ptr->name);
      ptr = ptr->next;
    }
}
