/* Copyright (C) 2014 Thorsten Kukuk
   Author: Thorsten Kukuk <kukuk@suse.de>

   This library is free software: you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   in version 2.1 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <rpcsvc/yp_prot.h>

#include "internal.h"

/* This should only be defined on systems with a BSD compatible ypbind */
#ifndef BINDINGDIR
# define BINDINGDIR "/var/yp/binding"
#endif

struct dom_binding
  {
    struct dom_binding *dom_pnext;
    char dom_domain[YPMAXDOMAIN + 1];
    struct sockaddr_in dom_server_addr;
    int dom_socket;
    CLIENT *dom_client;
  };
typedef struct dom_binding dom_binding;

static const struct timeval RPCTIMEOUT = {25, 0};
static const struct timeval UDPTIMEOUT = {5, 0};
static int const MAXTRIES = 2;
// XXX __libc_lock_define_initialized (static, ypbindlist_lock)
static dom_binding *ypbindlist = NULL;


static void
yp_bind_client_create (const char *domain, dom_binding *ysd,
		       struct ypbind2_resp *ypbr)
{
  ysd->dom_server_addr.sin_family = AF_INET;
  ysd->dom_server_addr.sin_port =
    ypbr->ypbind_respbody.ypbind_bindinfo.ypbind_binding_port;
  ysd->dom_server_addr.sin_addr =
    ypbr->ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;

  strncpy (ysd->dom_domain, domain, YPMAXDOMAIN);
  ysd->dom_domain[YPMAXDOMAIN] = '\0';

  ysd->dom_socket = RPC_ANYSOCK;
  ysd->dom_client = clntudp_bufcreate (&ysd->dom_server_addr, YPPROG,
				       YPVERS, UDPTIMEOUT,
				       &ysd->dom_socket,
				       UDPMSGSIZE, UDPMSGSIZE);

  if (ysd->dom_client != NULL)
    {
#ifndef SOCK_CLOEXEC
      /* If the program exits, close the socket */
      if (fcntl (ysd->dom_socket, F_SETFD, FD_CLOEXEC) == -1)
	perror ("fcntl: F_SETFD");
#endif
    }
}

static void
yp_bind_file (const char *domain, dom_binding *ysd)
{
  char path[sizeof (BINDINGDIR) + strlen (domain) + 3 * sizeof (unsigned) + 3];

  snprintf (path, sizeof (path), "%s/%s.%u", BINDINGDIR, domain, YPBINDVERS_2);
  int fd = open (path, O_RDONLY);
  if (fd >= 0)
    {
      /* We have a binding file and could save a RPC call.  The file
	 contains a port number and the YPBIND_RESP record.  The port
	 number (16 bits) can be ignored.  */
      struct ypbind2_resp ypbr;

      if (pread (fd, &ypbr, sizeof (ypbr), 2) == sizeof (ypbr))
	yp_bind_client_create (domain, ysd, &ypbr);

      close (fd);
    }
}

static int
yp_bind_ypbindprog (const char *domain, dom_binding *ysd)
{
  struct sockaddr_in clnt_saddr;
  struct ypbind2_resp ypbr;
  int clnt_sock;
  CLIENT *client;

  clnt_saddr.sin_family = AF_INET;
  clnt_saddr.sin_port = 0;
  clnt_saddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  clnt_sock = RPC_ANYSOCK;
  client = clnttcp_create (&clnt_saddr, YPBINDPROG, YPBINDVERS_2,
			   &clnt_sock, 0, 0);
  if (client == NULL)
    return YPERR_YPBIND;

  /* Check the port number -- should be < IPPORT_RESERVED.
     If not, it's possible someone has registered a bogus
     ypbind with the portmapper and is trying to trick us. */
  if (ntohs (clnt_saddr.sin_port) >= IPPORT_RESERVED)
    {
      clnt_destroy (client);
      return YPERR_YPBIND;
    }

  if (clnt_call (client, YPBINDPROC_DOMAIN,
		 (xdrproc_t) xdr_domainname, (caddr_t) &domain,
		 (xdrproc_t) xdr_ypbind2_resp,
		 (caddr_t) &ypbr, RPCTIMEOUT) != RPC_SUCCESS)
    {
      clnt_destroy (client);
      return YPERR_YPBIND;
    }

  clnt_destroy (client);

  if (ypbr.ypbind_status != YPBIND_SUCC_VAL)
    {
      fprintf (stderr, "YPBINDPROC_DOMAIN: %s\n",
	       ypbinderr_string (ypbr.ypbind_respbody.ypbind_error));
      return YPERR_DOMAIN;
    }
  memset (&ysd->dom_server_addr, '\0', sizeof ysd->dom_server_addr);

  yp_bind_client_create (domain, ysd, &ypbr);

  return YPERR_SUCCESS;
}

static int
__yp_bind (const char *domain, dom_binding **ypdb)
{
  dom_binding *ysd = NULL;
  int is_new = 0;

  if (domain == NULL || domain[0] == '\0')
    return YPERR_BADARGS;

  ysd = *ypdb;
  while (ysd != NULL)
    {
      if (strcmp (domain, ysd->dom_domain) == 0)
	break;
      ysd = ysd->dom_pnext;
    }

  if (ysd == NULL)
    {
      is_new = 1;
      ysd = (dom_binding *) calloc (1, sizeof *ysd);
      if (ysd == NULL)
	return YPERR_RESRC;
    }

  /* Try binding dir at first if we have no binding */
  if (ysd->dom_client == NULL)
    yp_bind_file (domain, ysd);

  if (ysd->dom_client == NULL)
    {
      int retval = yp_bind_ypbindprog (domain, ysd);
      if (retval != YPERR_SUCCESS)
	{
	  if (is_new)
	    free (ysd);
	  return retval;
	}
    }

  if (ysd->dom_client == NULL)
    {
      if (is_new)
	free (ysd);
      return YPERR_YPSERV;
    }

  if (is_new)
    {
      ysd->dom_pnext = *ypdb;
      *ypdb = ysd;
    }

  return YPERR_SUCCESS;
}

static void
__yp_unbind (dom_binding *ydb)
{
  clnt_destroy (ydb->dom_client);
  free (ydb);
}

int
yp_bind (const char *indomain)
{
  int status;

  // XXX __libc_lock_lock (ypbindlist_lock);

  status = __yp_bind (indomain, &ypbindlist);

  // XXX __libc_lock_unlock (ypbindlist_lock);

  return status;
}

static void
yp_unbind_locked (const char *indomain)
{
  dom_binding *ydbptr, *ydbptr2;

  ydbptr2 = NULL;
  ydbptr = ypbindlist;

  while (ydbptr != NULL)
    {
      if (strcmp (ydbptr->dom_domain, indomain) == 0)
	{
	  dom_binding *work;

	  work = ydbptr;
	  if (ydbptr2 == NULL)
	    ypbindlist = ypbindlist->dom_pnext;
	  else
	    ydbptr2 = ydbptr->dom_pnext;
	  __yp_unbind (work);
	  break;
	}
      ydbptr2 = ydbptr;
      ydbptr = ydbptr->dom_pnext;
    }
}

void
yp_unbind (const char *indomain)
{
  // XXX __libc_lock_lock (ypbindlist_lock);

  yp_unbind_locked (indomain);

  // XXX __libc_lock_unlock (ypbindlist_lock);

  return;
}

static int
__ypclnt_call (u_long prog, xdrproc_t xargs, caddr_t req,
	       xdrproc_t xres, caddr_t resp, dom_binding **ydb,
	       int print_error)
{
  enum clnt_stat result;

  result = clnt_call ((*ydb)->dom_client, prog,
		      xargs, req, xres, resp, RPCTIMEOUT);

  if (result != RPC_SUCCESS)
    {
      /* We don't print an error message, if we try our old,
	 cached data. Only print this for data, which should work.  */
      if (print_error)
	clnt_perror ((*ydb)->dom_client, "do_ypcall: clnt_call");

      return YPERR_RPC;
    }

  return YPERR_SUCCESS;
}

int
do_ypcall (const char *domain, u_long prog, xdrproc_t xargs,
	   caddr_t req, xdrproc_t xres, caddr_t resp)
{
  dom_binding *ydb;
  int status;
  int saved_errno = errno;

  status = YPERR_YPERR;

  // XXX __libc_lock_lock (ypbindlist_lock);
  ydb = ypbindlist;
  while (ydb != NULL)
    {
      if (strcmp (domain, ydb->dom_domain) == 0)
	{
          if (__yp_bind (domain, &ydb) == 0)
	    {
	      /* Call server, print no error message, do not unbind.  */
	      status = __ypclnt_call (prog, xargs, req, xres,
				      resp, &ydb, 0);
	      if (status == YPERR_SUCCESS)
	        {
		  // XXX __libc_lock_unlock (ypbindlist_lock);
		  errno = saved_errno;
	          return status;
	        }
	    }
	  /* We use ypbindlist, and the old cached data is
	     invalid. unbind now and create a new binding */
	  yp_unbind_locked (domain);

	  break;
	}
      ydb = ydb->dom_pnext;
    }
  // XXX __libc_lock_unlock (ypbindlist_lock);

  /* First try with cached data failed. Now try to get
     current data from the system.  */
  ydb = NULL;
  if (__yp_bind (domain, &ydb) == 0)
    {
      status = __ypclnt_call (prog, xargs, req, xres,
			      resp, &ydb, 1);
      __yp_unbind (ydb);
    }

  /* If we support binding dir data, we have a third chance:
     Ask ypbind.  */
  if (status != YPERR_SUCCESS)
    {
      ydb = calloc (1, sizeof (dom_binding));
      if (ydb != NULL && yp_bind_ypbindprog (domain, ydb) == YPERR_SUCCESS)
	{
	  status = __ypclnt_call (prog, xargs, req, xres,
				  resp, &ydb, 1);
	  __yp_unbind (ydb);
	}
      else
	free (ydb);
    }

  errno = saved_errno;

  return status;
}

/* Like do_ypcall, but translate the status value if necessary.  */
int
do_ypcall_tr (const char *domain, u_long prog, xdrproc_t xargs,
	      caddr_t req, xdrproc_t xres, caddr_t resp)
{
  int status = do_ypcall (domain, prog, xargs, req, xres, resp);
  if (status == YPERR_SUCCESS)
    /* We cast to ypresp_val although the pointer could also be of
       type ypresp_key_val or ypresp_master or ypresp_order or
       ypresp_maplist.  But the stat element is in a common prefix so
       this does not matter.  */
    status = ypprot_err (((struct ypresp_val *) resp)->status);
  return status;
}

/* XXX move into own C file */

struct ypresp_all_data
{
  unsigned long status;
  void *data;
  int (*foreach) (int status, char *key, int keylen,
                  char *val, int vallen, char *data);
};

static bool_t
__xdr_ypresp_all (XDR *xdrs, struct ypresp_all_data *objp)
{
  while (1)
    {
      struct ypresp_all resp;

      memset (&resp, '\0', sizeof (struct ypresp_all));
      if (!xdr_ypresp_all (xdrs, &resp))
        {
          xdr_free ((xdrproc_t) xdr_ypresp_all, (char *) &resp);
          objp->status = YP_YPERR;
          return FALSE;
        }
      if (resp.more == 0)
        {
          xdr_free ((xdrproc_t) xdr_ypresp_all, (char *) &resp);
          objp->status = YP_NOMORE;
          return TRUE;
        }

      switch (resp.ypresp_all_u.val.status)
        {
        case YP_TRUE:
          {
            char key[resp.ypresp_all_u.val.keydat.keydat_len + 1];
            char val[resp.ypresp_all_u.val.valdat.valdat_len + 1];
            int keylen = resp.ypresp_all_u.val.keydat.keydat_len;
            int vallen = resp.ypresp_all_u.val.valdat.valdat_len;

            /* We are not allowed to modify the key and val data.
               But we are allowed to add data behind the buffer,
               if we don't modify the length. So add an extra NUL
               character to avoid trouble with broken code. */
            objp->status = YP_TRUE;
	    memcpy (key, resp.ypresp_all_u.val.keydat.keydat_val,
		    keylen);
	    key[keylen] = '\0';
	    memcpy (val, resp.ypresp_all_u.val.valdat.valdat_val,
		    vallen);
	    val[vallen] = '\0';

            xdr_free ((xdrproc_t) xdr_ypresp_all, (char *) &resp);
            if ((*objp->foreach) (objp->status, key, keylen,
                                  val, vallen, objp->data))
              return TRUE;
          }
          break;
        default:
          objp->status = resp.ypresp_all_u.val.status;
          xdr_free ((xdrproc_t) xdr_ypresp_all, (char *) &resp);
          /* Sun says we don't need to make this call, but must return
             immediately. Since Solaris makes this call, we will call
             the callback function, too. */
          (*objp->foreach) (objp->status, NULL, 0, NULL, 0, objp->data);
          return TRUE;
        }
    }
}


int
yp_all (const char *indomain, const char *inmap,
        const struct ypall_callback *incallback)
{
  struct ypreq_nokey req;
  dom_binding *ydb = NULL;
  int try, res;
  enum clnt_stat result;
  struct sockaddr_in clnt_sin;
  CLIENT *clnt;
  struct ypresp_all_data data;
  int clnt_sock;
  int saved_errno = errno;

  if (indomain == NULL || indomain[0] == '\0'
      || inmap == NULL || inmap[0] == '\0')
    return YPERR_BADARGS;

  try = 0;
  res = YPERR_YPERR;

  while (try < MAXTRIES && res != YPERR_SUCCESS)
    {
      if (__yp_bind (indomain, &ydb) != 0)
        {
	  errno = saved_errno;
          return YPERR_DOMAIN;
        }

      clnt_sock = RPC_ANYSOCK;
      clnt_sin = ydb->dom_server_addr;
      clnt_sin.sin_port = 0;

      /* We don't need the UDP connection anymore.  */
      __yp_unbind (ydb);
      ydb = NULL;

      clnt = clnttcp_create (&clnt_sin, YPPROG, YPVERS, &clnt_sock, 0, 0);
      if (clnt == NULL)
        {
          errno = saved_errno;
          return YPERR_PMAP;
        }
      req.domain = (char *) indomain;
      req.map = (char *) inmap;

      data.foreach = incallback->foreach;
      data.data = (void *) incallback->data;

      result = clnt_call (clnt, YPPROC_ALL, (xdrproc_t) xdr_ypreq_nokey,
                          (caddr_t) &req, (xdrproc_t) __xdr_ypresp_all,
                          (caddr_t) &data, RPCTIMEOUT);

      if (result != RPC_SUCCESS)
        {
          /* Print the error message only on the last try.  */
          if (try == MAXTRIES - 1)
            clnt_perror (clnt, "yp_all: clnt_call");
          res = YPERR_RPC;
        }
      else
        res = YPERR_SUCCESS;

      clnt_destroy (clnt);

      if (res == YPERR_SUCCESS && data.status != YP_NOMORE)
        {
	  errno = saved_errno;
          return ypprot_err (data.status);
        }
      ++try;
    }

  errno = saved_errno;

  return res;
}
