/* Copyright (C) 1998, 1999 Thorsten Kukuk
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

/*
 * This file contains symbols and structures defining the rpc protocol
 * between the NIS clients and the NIS servers.  The servers
 * are the NIS database servers, and the NIS binders.
 */

#ifndef _YP_TOOLS_H
#define _YP_TOOLS_H

#include <rpc/rpc.h>

/* some defines */
#define YPERR_SUCCESS 0                 /* There is no error */
#define YPERR_BADARGS 1                 /* Args to function are bad */
#define YPERR_RPC 2                     /* RPC failure */
#define YPERR_DOMAIN 3                  /* Can't bind to a server with this domain */
#define YPERR_MAP 4                     /* No such map in server's domain */
#define YPERR_KEY 5                     /* No such key in map */
#define YPERR_YPERR 6                   /* Internal yp server or client error */
#define YPERR_RESRC 7                   /* Local resource allocation failure */
#define YPERR_NOMORE 8                  /* No more records in map database */
#define YPERR_PMAP 9                    /* Can't communicate with portmapper */
#define YPERR_YPBIND 10                 /* Can't communicate with ypbind */
#define YPERR_YPSERV 11                 /* Can't communicate with ypserv */
#define YPERR_NODOM 12                  /* Local domain name not set */
#define YPERR_BADDB 13                  /* yp data base is bad */
#define YPERR_VERS 14                   /* YP version mismatch */
#define YPERR_ACCESS 15                 /* Access violation */
#define YPERR_BUSY 16                   /* Database is busy */

#define YPMAXRECORD     1024
#define YPMAXDOMAIN     64 /* XXX orig. yp_prot.h defines ((u_long)256) */
#define YPMAXMAP        64
#define YPMAXPEER       64 /* XXX orig. yp_prot.h defines ((u_long)256) */

#define YPPROG          ((unsigned long)100004)
#define YPVERS          ((unsigned long)2)
#define YPPROC_NULL     ((u_long)0)
#define YPPROC_DOMAIN   ((u_long)1)
#define YPPROC_MASTER   ((u_long)9)
#define YPPROC_ORDER    ((u_long)10)


#define YPBINDPROG              ((unsigned long)100007)
#define YPBINDVERS              ((u_long)2)
#define YPBINDPROC_DOMAIN       ((unsigned long)1)
#define YPBINDPROC_OLDDOMAIN     ((unsigned long)1)
#define YPBINDPROC_SETDOM       ((u_long)2)

/* struct ypall_callback * is the arg which must be passed to yp_all */
struct ypall_callback {
  int (*foreach) (int __status, char *__key, int __keylen,
		  char *__val, int __vallen, char *__data);
  char *data;
};

/*
 * Request parameter structures
 */
typedef struct {
  u_int keydat_len;
  char *keydat_val;
} keydat_t;

typedef struct {
  u_int valdat_len;
  char *valdat_val;
} valdat_t;

struct ypreq_key {
  const char *domain;
  const char *map;
  keydat_t keydat;
};

struct ypreq_nokey {
  char *domain;
  char *map;
};

enum ypstat {
  YP_TRUE = 1,          /* General purpose success code */
  YP_NOMORE = 2,        /* No more entries in map */
  YP_FALSE = 0,         /* General purpose failure code */
  YP_NOMAP = -1,        /* No such map in domain */
  YP_NODOM = -2,        /* Domain not supported */
  YP_NOKEY = -3,        /* No such key in map */
  YP_BADOP = -4,        /* Invalid operation */
  YP_BADDB = -5,        /* Server data base is bad */
  YP_YPERR = -6,        /* NIS server error */
  YP_BADARGS = -7,      /* Request arguments bad */
  YP_VERS = -8,         /* NIS server version mismatch - server can't supply
                           requested service. */
};
typedef enum ypstat ypstat;

struct ypresp_master {
  ypstat status;
  char *master;
};

struct ypresp_order {
  ypstat status;
  u_int ordernum;
};

struct ypmaplist {
  char *map;
  struct ypmaplist *next;
};

/*
 * Response structure and overall result status codes.  Success and failure
 * represent two separate response message types.
 */

enum ypbind_resptype {YPBIND_SUCC_VAL = 1, YPBIND_FAIL_VAL = 2};

struct ypbind_binding {
  struct in_addr ypbind_binding_addr;           /* In network order */
  unsigned short int ypbind_binding_port;       /* In network order */
};

struct ypbind_resp {
  enum ypbind_resptype ypbind_status;
  union {
    u_int ypbind_error;
    struct ypbind_binding ypbind_bindinfo;
  } ypbind_respbody;
};

/* Detailed failure reason codes for response field ypbind_error*/
#define YPBIND_ERR_ERR 1                /* Internal error */
#define YPBIND_ERR_NOSERV 2             /* No bound server for passed domain */
#define YPBIND_ERR_RESC 3               /* System resource allocation failure */

/*
 * Request data structure for ypbind "Set domain" procedure.
 */
struct ypbind_setdom {
  char *ypsetdom_domain;
  struct ypbind_binding ypsetdom_binding;
  u_int ypsetdom_vers;
};

extern int yp_get_default_domain (char **);
extern int yp_first (const char *, const char *, char **,
                     int *, char **, int *);
extern int yp_next (const char *, const char *, const char *,
                    const int, char **, int *, char **, int *);
extern int yp_order (const char *, const char *, unsigned int *);
extern int yp_all (const char *, const char *, const struct ypall_callback *);
extern int yp_maplist (const char *, struct ypmaplist **);
extern int yp_master (const char *, const char *, char **);
extern int yp_match (const char *, const char *, const char *,
		     const int, char **, int *);
extern int ypprot_err (const int);
extern const char *yperr_string (const int);
extern const char *ypbinderr_string (const int);

extern  bool_t ytxdr_ypstat (XDR *, enum ypstat*);
extern  bool_t ytxdr_domainname (XDR *, char **);
extern  bool_t ytxdr_mapname (XDR *, char **);
extern  bool_t ytxdr_peername (XDR *, char **);
extern  bool_t ytxdr_ypreq_nokey (XDR *, struct ypreq_nokey *);
extern  bool_t ytxdr_ypresp_master (XDR *, struct ypresp_master *);
extern  bool_t ytxdr_ypresp_order (XDR *, struct ypresp_order *);
extern  bool_t ytxdr_ypbind_resptype (XDR *, enum ypbind_resptype *);
extern  bool_t ytxdr_ypbind_binding(XDR *, struct ypbind_binding *);
extern  bool_t ytxdr_ypbind_resp(XDR *, struct ypbind_resp *);
extern  bool_t ytxdr_ypbind_setdom(XDR *, struct ypbind_setdom *);

#if 0
extern  bool_t ytxdr_keydat (XDR *, keydat_t *);
extern  bool_t ytxdr_valdat (XDR *, valdat_t *);
extern  bool_t ytxdr_ypmap_parms (XDR *, struct ypmap_parms*);
extern  bool_t ytxdr_ypreq_key (XDR *, ypreq_key*);
extern  bool_t ytxdr_ypresp_val (XDR *, ypresp_val*);
extern  bool_t ytxdr_ypresp_key_val (XDR *, ypresp_key_val*);
extern  bool_t ytxdr_ypresp_all (XDR *, ypresp_all*);
extern  bool_t ytxdr_ypmaplist(XDR *, ypmaplist*);
extern  bool_t ytxdr_ypresp_maplist(XDR *, ypresp_maplist*);

#endif

#endif _YP_TOOLS_H
