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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pwd.h>
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif
#include <ctype.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <libintl.h>
#include <sys/param.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "yppasswd.h"

#if defined (__NetBSD__) || (defined(__GLIBC__) && (__GLIBC__ == 2 && __GLIBC_MINOR__ == 0))
/* <rpc/rpc.h> is missing the prototype */
int getrpcport(char *host, int prognum, int versnum, int proto);
#endif

#ifndef _
#define _(String) gettext (String)
#endif

/* ok, we're using the crack library */
#ifdef USE_CRACKLIB
#ifndef CRACKLIB_DICTPATH
#define CRACKLIB_DICTPATH "/usr/lib/cracklib_dict"
#endif
#endif


/* Name and version of program.  */
/* Print the version information.  */
static void
print_version (const char *program)
{
  fprintf (stdout, "%s (%s) %s\n", program, PACKAGE, VERSION);
  fprintf (stdout, gettext ("\
Copyright (C) %s Thorsten Kukuk.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "1998");
  /*  fprintf (stdout, _("Written by %s.\n"), "Thorsten Kukuk"); */
}

static void
print_usage_pwd (FILE *stream)
{
  fputs (_("Usage: yppasswd [-f] [-l] [-p] [User]\n"), stream);
}

static void
print_usage_chsh (FILE *stream)
{
  fputs (_("Usage: ypchsh [User]\n"), stream);
}

static void
print_usage_chfn (FILE *stream)
{
  fputs (_("Usage: ypchfn [User]\n"), stream);
}

static void
print_help_pwd (void)
{
  print_usage_pwd (stdout);
  fputs (_("yppasswd - change your password in the NIS database\n\n"), stdout);

  fputs (_("  -f             Change GECOS field information\n"), stdout);
  fputs (_("  -l             Change the login shell\n"), stdout);
  fputs (_("  -p             Change the password\n"), stdout);
  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_help_chsh (void)
{
  print_usage_chsh (stdout);
  fputs (_("ypchsh - change the shell in the NIS database\n\n"), stdout);

  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_help_chfn (void)
{
  print_usage_chfn (stdout);
  fputs (_("ypchfn - change the GECOS information in the NIS database\n\n"),
	 stdout);

  fputs (_("  -?, --help     Give this help list\n"), stdout);
  fputs (_("      --usage    Give a short usage message\n"), stdout);
  fputs (_("      --version  Print program version\n"), stdout);
}

static void
print_error (const char *program)
{
  if (strcmp (program, "ypchsh") == 0)
    print_usage_chsh (stderr);
  else if (strcmp (program, "ypchfn") == 0)
    print_usage_chfn (stderr);
  else
    print_usage_pwd (stderr);

  fprintf (stderr,
	   _("Try `%s --help' or `%s --usage' for more information.\n"),
	   program, program);
}

static char *
getnismaster (char *domainname, const char *progname)
{
  char *master;
  int port, err;

  if ((err = yp_master (domainname, "passwd.byname", &master)) != 0)
    {
      fprintf (stderr, _("%s: can't find the master ypserver: %s\n"),
               progname, yperr_string (err));
      return NULL;
    }
  port = getrpcport (master, YPPASSWDPROG, YPPASSWDPROC_UPDATE, IPPROTO_UDP);
  if (port == 0)
    {
      fprintf (stderr, _("%s: yppasswdd not running on NIS master host\n"),
               progname);
      return NULL;
    }
  if (port >= IPPORT_RESERVED)
    {
      fprintf (stderr, _("%s: yppasswd daemon running on illegal port.\n"),
               progname);
      return NULL;
    }

  return master;
}

/* YP result codes. */
static const char *
yp_strerror (int code)
{
  const char *res;

  switch (code)
    {
    case YP_NOMORE:
      res = _("No more");
      break;
    case YP_TRUE:
      res = _("True");
      break;
    case YP_FALSE:
      res = _("False");
      break;
    case YP_NOMAP:
      res = _("No such map");
      break;
    case YP_NODOM:
      res = _("No such domain");
      break;
    case YP_BADOP:
      res = _("Bad operation");
      break;
    case YP_BADDB:
      res = _("Database bad");
      break;
    case YP_BADARGS:
      res = _("Bad arguments");
      break;
    case YP_VERS:
      res = _("Version Mismatch");
      break;
    default:
      res = _("Unknown error");
      break;
    }
  return (res);
}

static char *
val (char *s)
{
  return s ? s : (char *)"";
}

static struct timeval TIMEOUT = {25, 0}; /* total timeout */

static struct passwd *
ypgetpw (char *master, char *domainname, char *name, int uid)
{
  struct ypreq_key key;
  enum clnt_stat res;
  struct ypresp_val resp;
  char buffer[1024], uidbuf[8], *ptr, *keyval;
  static struct passwd pwd;
  char *map;
  CLIENT *clnt;

  clnt = clnt_create (master, YPPROG, YPVERS, "udp");
  clnt->cl_auth = authunix_create_default ();

  if (name == NULL)
    {
      sprintf (uidbuf, "%d", uid);
      keyval = strdup (uidbuf);
      map = strdup ("passwd.byuid");
    }
  else
    {
      keyval = strdup (name);
      map = strdup ("passwd.byname");
    }
  /* Fill in values. */
  memset (&resp, 0, sizeof (struct ypresp_val));
  key.domain = strdup (domainname);
  key.map = map;
  key.keydat.keydat_val = keyval;
  key.keydat.keydat_len = strlen (keyval);

  res = clnt_call (clnt, YPPROC_MATCH, (xdrproc_t) xdr_ypreq_key,
		   (caddr_t) &key, (xdrproc_t) xdr_ypresp_val,
		   (caddr_t) &resp, TIMEOUT);
  free (map);
  free (keyval);

  if (res != RPC_SUCCESS)
    {
      clnt_perrno (res);
      fprintf (stderr, "\n");
      return NULL;
    }

  if (resp.status != 1)
    {
      fprintf (stderr, "%s\n", yp_strerror (resp.status));
      return NULL;
    }

  strncpy (buffer, resp.valdat.valdat_val, resp.valdat.valdat_len);
  buffer[resp.valdat.valdat_len] = '\0';
  free (resp.valdat.valdat_val);

  ptr = buffer;
  pwd.pw_name = strdup (val(strsep (&ptr, ":")));
  if (*ptr != ':')
    pwd.pw_passwd = strdup (val(strsep (&ptr, ":")));
  else
    {
      pwd.pw_passwd = strdup ((char *)"");
      ++ptr;
    }

  pwd.pw_uid = atoi (val (strsep (&ptr, ":")));
  pwd.pw_gid = atoi (val (strsep (&ptr, ":")));
  pwd.pw_gecos = strdup (val (strsep (&ptr, ":")));
  pwd.pw_dir = strdup (val (strsep (&ptr, ":")));
  pwd.pw_shell = strdup (val (strsep (&ptr, ":")));

  auth_destroy (clnt->cl_auth);
  clnt_destroy (clnt);

  return &pwd;
}

static int
newfield (const char *progname, const char *prompt, const char *def,
	  char *field, int size)
{
  char *sp;

  if (def == NULL)
    def = "none";

  printf ("%s [%s]: ", prompt, def);
  fflush (stdout);

  if (fgets (field, size, stdin) == NULL)
    return 1;

  if ((sp = strchr (field, '\n')) != NULL)
    *sp = '\0';

  if (!strcmp (field, ""))
    strcpy (field, def);
  if (!strcmp (field, "none"))
    strcpy (field, "");

  if (strchr (field, ':') != NULL)
    {
      fprintf (stderr, _("%s: no colons allowed in GECOS field... sorry.\n"),
               progname);
      return 1;
    }
  return 0;
}

static char *
getfield (char *gecos, char *field, int size)
{
  char *sp;

  for (sp = gecos; *sp != '\0' && *sp != ','; sp++);
  if (*sp != '\0')
    *sp++ = '\0';

  strncpy (field, gecos, size - 1);
  field[size - 1] = '\0';
  return sp;
}

#ifndef USE_CRACKLIB

/* this function will verify the user's password
 * for some silly things.  If we're using cracklib, then
 * this function is unused.
 */

static int /* return values: 0 = not ok, 1 = ok */
verifypassword (struct passwd *pwd, char *pwdstr, uid_t uid)
{
  char *p, *q;
  int ucase, lcase, other, r;

  if ((strlen (pwdstr) < 6) && uid)
    {
      fputs (_("The password must have at least 6 characters.\n"), stderr);
      return 0;
    }

  other = ucase = lcase = 0;
  for (p = pwdstr; *p; p++)
    {
      ucase = ucase || isupper (*p);
      lcase = lcase || islower (*p);
      other = other || !isalpha (*p);
    }

  if ((!ucase || !lcase) && !other && uid)
    {
      fputs (_("The password must have both upper and lowercase "
	       "letters, or non-letters.\n"), stderr);
      return 0;
    }

  if (pwd->pw_passwd[0]
      && !strncmp (pwd->pw_passwd, crypt (pwdstr, pwd->pw_passwd), 13)
      && uid)
    {
      fputs (_("You cannot reuse the old password.\n"), stderr);
      return 0;
    }

  r = 0;
  for (p = pwdstr, q = pwd->pw_name; *q && *p; q++, p++)
    if (tolower (*p) != tolower (*q))
      {
	r = 1;
	break;
      }

  for (p = pwdstr + strlen (pwdstr) - 1, q = pwd->pw_name;
       *q && p >= pwdstr; q++, p--)
    if (tolower (*p) != tolower (*q))
      {
	r += 2;
	break;
      }

  if (uid && r != 3)
    {
      fputs (_("Please don't use something like your username as password.\n"),
	     stderr);
      return 0;
    }
  return 1;                     /* OK */
}

#endif

int
main (int argc, char **argv)
{
  char *s, *progname, *domainname = NULL, *user = NULL, *master = NULL;
  int f_flag = 0, l_flag = 0, p_flag = 0, error, status;
  struct yppasswd yppwd;
  struct passwd *pwd;
  CLIENT *clnt;
  uid_t uid;

  setlocale (LC_MESSAGES, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  if ((s = strrchr (argv[0], '/')) != NULL)
    progname = s + 1;
  else
    progname = argv[0];

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

      c = getopt_long (argc, argv,
		       (strcmp ("yppasswd", progname) == 0) ? "flp?" : "?",
		       long_options, &option_index);
      if (c == (-1))
        break;
      switch (c)
        {
	case 'f':
	  f_flag = 1;
	  break;
	case 'l':
	  l_flag = 1;
	  break;
	case 'p':
	  p_flag = 1;
	  break;
	case '?':
	  if (strcmp (progname, "ypchsh") == 0)
	    print_help_chsh ();
	  else if (strcmp (progname, "ypchfn") == 0)
	    print_help_chfn ();
	  else
	    print_help_pwd ();
	  return 0;
	case '\255':
	  print_version (progname);
	  return 0;
	case '\254':
	  if (strcmp (progname, "ypchsh") == 0)
	    print_usage_chsh (stdout);
	  else if (strcmp (progname, "ypchfn") == 0)
	    print_usage_chfn (stdout);
	  else
	    print_usage_pwd (stdout);
	  return 0;
	default:
	  if (strcmp (progname, "ypchsh") == 0)
	    print_usage_chsh (stderr);
	  else if (strcmp (progname, "ypchfn") == 0)
	    print_usage_chfn (stderr);
	  else
	    print_usage_pwd (stderr);
	  return 1;
	}
    }

  argc -= optind;
  argv += optind;

  if (strcmp (progname, "ypchsh") == 0)
    l_flag = 1;
  else if (strcmp (progname, "ypchfn") == 0)
    f_flag = 1;

  if (f_flag == 0 && l_flag == 0)
    p_flag = 1;

  if (argc > 1)
    {
      print_error (progname);
      return 1;
    }
  else if (argc == 1)
    user = argv[0];

  if ((error = yp_get_default_domain (&domainname)) != 0)
    {
      fprintf (stderr, _("%s: can't get local yp domain: %s\n"),
               progname, yperr_string (error));
      return 1;
    }

  if ((master = getnismaster (domainname, progname)) == NULL)
    return 1;

  /* Get the passwd struct for the user whose password is to be changed
     directly from the NIS master server without getpwnam () */
  uid = getuid ();
  if ((pwd = ypgetpw (master, domainname, user, uid)) == NULL)
    {
      fprintf (stderr, _("%s: unknown user (uid=%d).\n"), progname, uid);
      return 1;
    }

  if (user != NULL)
    {
      if (pwd->pw_uid != uid && uid != 0)
        {
          fprintf (stderr,
	    _("%s: Only root may change account information for others\n"),
		   progname);
	  return 1;
        }
    }

  /* Initialize password information */
  memset (&yppwd, '\0', sizeof (yppwd));
  yppwd.newpw.pw_passwd = pwd->pw_passwd;
  yppwd.newpw.pw_name = pwd->pw_name;
  yppwd.newpw.pw_uid = pwd->pw_uid;
  yppwd.newpw.pw_gid = pwd->pw_gid;
  yppwd.newpw.pw_gecos = pwd->pw_gecos;
  yppwd.newpw.pw_dir = pwd->pw_dir;
  yppwd.newpw.pw_shell = pwd->pw_shell;
  yppwd.oldpass = (char *)"";

  fprintf (stdout, _("Changing NIS account information for %s on %s.\n"),
	   pwd->pw_name, master);

  /* Get old password */
  if ((pwd->pw_passwd != NULL && strlen (pwd->pw_passwd) > 0) ||
      (pwd->pw_uid != uid))
    {
      char prompt[40];

      if (pwd->pw_uid != uid)
        sprintf (prompt, _("Please enter root password:"));
      else
        sprintf (prompt, _("Please enter %spassword:"),
		 p_flag ? _("old ") : "");
      s = getpass (prompt);

      /* We can't check the password with shadow passwords enabled. We
       * leave the checking to yppasswdd */
      if (uid != 0 && strcmp (pwd->pw_passwd, "x") != 0)
        if (strcmp (crypt (s, pwd->pw_passwd), pwd->pw_passwd))
          {
            fprintf (stderr, _("Sorry.\n"));
	    return 1;
          }
      yppwd.oldpass = strdup (s);
    }

  if (p_flag)
    {
#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')
#ifdef USE_CRACKLIB
      char *error_msg;
#endif /* USE_CRACKLIB */
      char *buf, salt[2], *p = NULL;
      int tries = 0;
      time_t tm;

      buf = (char *) malloc (30);

      printf (_("Changing NIS password for %s on %s.\n"), pwd->pw_name,
	      master);

      buf[0] = '\0';
      while (1)
	{
	  if ( ++tries > 3 )
	    {
	      fputs (_("Too many tries. Aborted.\nPassword unchanged.\n"),
		     stderr);
	      return 1;
	    }

	  p = getpass (_("Please enter new password:"));
	  if (*p == '\0')
	    {
	      printf (_("Password unchanged.\n"));
	      return 1;
	    }

	  strcpy (buf, p);

#ifdef USE_CRACKLIB
	  error_msg = FascistCheck (buf, CRACKLIB_DICTPATH);
	  if (error_msg)
	    {
	      fprintf (stderr, _("Not a valid password: %s.\n"), error_msg);
	      continue;
	    }
#else
	  if (verifypassword (pwd, buf, uid) != 1)
	    continue;
#endif /* USE_CRACKLIB */

	  p = getpass (_("Please retype new password:"));
	  if (strcmp (buf, p) == 0)
	    break;
	  else
	    {
	      printf (_("Mismatch - password unchanged.\n"));
	      return 1;
	    }
	}

      time (&tm);
      salt[0] = bin_to_ascii (tm & 0x3f);
      salt[1] = bin_to_ascii ((tm >> 6) & 0x3f);

      yppwd.newpw.pw_passwd = strdup (crypt (buf, salt));
    }

  if (f_flag)
    {
      char gecos[1024], *sp, new_gecos[1024];
      char name[254], location[254], office[254], phone[254];

      printf (_("\nChanging full name for %s on %s.\n"
	     "To accept the default, simply press return. To enter an empty\n"
		"field, type the word \"none\".\n"),
	      pwd->pw_name, master);

      strncpy (gecos, pwd->pw_gecos, sizeof (gecos));
      sp = getfield (gecos, name, sizeof (name));
      if (newfield (progname, _("Name"), strtok (gecos, ","), name,
		    sizeof (name)))
	return 1;
      sp = getfield (sp, location, sizeof (location));
      if (newfield (progname, _("Location"), location, location,
		    sizeof (location)))
	return 1;
      sp = getfield (sp, office, sizeof (office));
      if (newfield (progname, _("Office Phone"), office, office,
		    sizeof (office)))
	return 1;
      sp = getfield (sp, phone, sizeof (phone));
      if (newfield (progname, _("Home Phone"), phone, phone, sizeof (phone)))
	return 1;
      sprintf (new_gecos, "%s,%s,%s,%s", name, location, office, phone);
      sp = new_gecos + strlen (new_gecos);
      while (*--sp == ',')
	*sp = '\0';

      yppwd.newpw.pw_gecos = strdup (new_gecos);
    }

  if (l_flag)
    {
      char new_shell[PATH_MAX];

      printf (_("\nChanging login shell for %s on %s.\n"
		"To accept the default, simply press return. To use the\n"
		"system's default shell, type the word \"none\".\n"),
	      pwd->pw_name, master);

      if (newfield (progname, _("Login shell"), pwd->pw_shell,
		    new_shell, sizeof (new_shell)))
	return 1;

      yppwd.newpw.pw_shell = strdup (new_shell);
    }

  clnt = clnt_create (master, YPPASSWDPROG, YPPASSWDVERS, "udp");
  clnt->cl_auth = authunix_create_default ();
  memset ((char *) &status, '\0', sizeof (status));
  error = clnt_call (clnt, YPPASSWDPROC_UPDATE,
		     (xdrproc_t) xdr_yppasswd, (char *) &yppwd,
		     (xdrproc_t) xdr_int, (char *) &status, TIMEOUT);

  switch (p_flag + (f_flag << 1) + (l_flag << 2))
    {
    case 1:
      if (error || status)
	{
	  if (error)
	    clnt_perrno (error);
	  else
	    fputs (_("Error while changing the NIS password."), stderr);
	  fprintf (stderr,
		   _("\nThe NIS password has not been changed on %s.\n\n"),
		   master);
	}
      else
	fprintf (stdout, _("\nThe NIS password has been changed on %s.\n\n"),
		 master);
      break;
    case 2:
      if (error || status)
	{
	  if (error)
	    clnt_perrno (error);
	  else
	    fputs (_("Error while changing the GECOS information."), stderr);
	  fprintf (stderr,
		 _("\nThe GECOS information has not been changed on %s.\n\n"),
		   master);
	}
      fprintf (stdout,
	       _("\nThe GECOS information has been changed on %s.\n\n"),
	       master);
      break;
    case 4:
      if (error || status)
	{
	  if (error)
	    clnt_perrno (error);
	  else
	    fputs (_("Error while changing the login shell."), stderr);
	  fprintf (stderr,
		   _("\nThe login shell has not been changed on %s.\n\n"),
		   master);
	}
      fprintf (stdout,
	       _("\nThe login shell has been changed on %s.\n\n"), master);
      break;
    default:
      if (error || status)
	{
	  if (error)
	    clnt_perrno (error);
	  else
	    fputs (_("Error while changing the NIS account information."),
		   stderr);
	  fprintf (stderr,
	   _("\nThe NIS account information has not been changed on %s.\n\n"),
		   master);
	}
      fprintf (stdout,
	       _("\nThe NIS account information has been changed on %s.\n\n"),
	       master);
      break;
    }

  auth_destroy (clnt->cl_auth);
  clnt_destroy (clnt);

  return ((error || status) != 0);
}
