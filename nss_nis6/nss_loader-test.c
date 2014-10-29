/* Copyright (C) 2014 Thorsten Kukuk
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
#include <config.h>
#endif

#include <nss.h>
#include <pwd.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int
main (void)
{
  enum nss_status (*nss_getpwnam_r)(const char *name, struct passwd *pwd,
                                    char *buffer, size_t buflen, int *errnop);
  enum nss_status status;
  char *error;
  void *nss_handle = NULL;
  size_t pwdbuflen = 2048;
  char pwdbuffer[2048];
  struct passwd pw;
  int i;

  nss_handle = dlopen (".libs/libnss_nis6.so.2", RTLD_NOW);
  if (!nss_handle)
    {
      fprintf (stderr, "dlopen(\".libs/libnss_nis6.so.2\") failed:\n%s\n",
	       dlerror ());
      return 1;
    }

  nss_getpwnam_r = dlsym (nss_handle, "_nss_nis6_getpwnam_r");
  if ((error = dlerror ()) != NULL)
    {
      fprintf (stderr, "dlsym(\"_nss_nis6_getpwnam_r\") failed: %s\n",
	       error);
      dlclose (nss_handle);
      return 1;
    }

  /* Get NIS passwd entry... */
  do {
    errno = 0;
    status = (*nss_getpwnam_r)("nobody", &pw, pwdbuffer,
			       pwdbuflen, &errno);
  } while (status == NSS_STATUS_TRYAGAIN && errno == ERANGE);

  if (status != NSS_STATUS_SUCCESS)
    {
      dlclose (nss_handle);
      fprintf (stderr, "nss_getpwnam_r(\"nobody\") failed! (%i)\n",
	       status);
      return 1;
    }

  return 0;
}
