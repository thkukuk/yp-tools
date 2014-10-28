
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

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

static void
ypbind3_binding_dump (struct ypbind3_binding *ypb3)
{
  char buf[INET6_ADDRSTRLEN];

  printf ("ypbind_nconf:\n");
  if (ypb3->ypbind_nconf)
    dump_nconf (ypb3->ypbind_nconf, "\t");
  else
    printf ("\tNULL\n");

  printf ("ypbind_svcaddr: %s:%i\n",
          taddr2ipstr (ypb3->ypbind_nconf, ypb3->ypbind_svcaddr,
                       buf, sizeof (buf)),
	  taddr2port (ypb3->ypbind_nconf, ypb3->ypbind_svcaddr));

  printf ("ypbind_servername: ");
  if (ypb3->ypbind_servername)
    printf ("%s\n", ypb3->ypbind_servername);
  else
    printf ("NULL\n");
  printf ("ypbind_hi_vers: %lu\n", (u_long) ypb3->ypbind_hi_vers);
  printf ("ypbind_lo_vers: %lu\n", (u_long) ypb3->ypbind_lo_vers);
}

#undef xdr_rpcvers
#define xdr_rpcvers(xdrs, versp) xdr_u_int32_t(xdrs, versp)

static bool_t
xdr_netconfig(XDR *xdrs, struct netconfig *objp)
{
        if (!xdr_string(xdrs, &objp->nc_netid, ~0))
                return (FALSE);
        if (!xdr_u_int64_t(xdrs, &objp->nc_semantics))
                return (FALSE);
        if (!xdr_u_int64_t(xdrs, &objp->nc_flag))
                return (FALSE);
        if (!xdr_string(xdrs, &objp->nc_protofmly, ~0))
                return (FALSE);
        if (!xdr_string(xdrs, &objp->nc_proto, ~0))
                return (FALSE);
        if (!xdr_string(xdrs, &objp->nc_device, ~0))
                return (FALSE);
        if (!xdr_array(xdrs, (char **)&objp->nc_lookups,
                (u_int32_t *)&objp->nc_nlookups, 100, sizeof (char *),
                (xdrproc_t)xdr_wrapstring))
                return (FALSE);
        return (xdr_vector(xdrs, (char *)objp->nc_unused,
                8, sizeof (u_int32_t), (xdrproc_t)xdr_u_int));
}

static bool_t
__xdr_pointer(XDR *xdrs, char **objpp, u_int32_t obj_size,
              const xdrproc_t xdr_obj)
{
        bool_t more_data;

        more_data = (*objpp != NULL);
        if (!xdr_bool(xdrs, &more_data))
                return (FALSE);
        if (!more_data) {
                *objpp = NULL;
                return (TRUE);
        }
        return (xdr_reference(xdrs, objpp, obj_size, xdr_obj));
}

bool_t
xdr_ypbind3_binding (XDR *xdrs, struct ypbind3_binding *objp)
{
  if (!__xdr_pointer (xdrs, (char **)&objp->ypbind_nconf,
                    sizeof (struct netconfig), (xdrproc_t) xdr_netconfig))
    return FALSE;
  if (!xdr_pointer(xdrs, (char **)&objp->ypbind_svcaddr,
                   sizeof (struct netbuf), (xdrproc_t) xdr_netbuf))
    return FALSE;
  if (!xdr_string(xdrs, &objp->ypbind_servername, ~0))
    return FALSE;
  if (!xdr_rpcvers(xdrs, &objp->ypbind_hi_vers))
    return FALSE;
  return xdr_rpcvers(xdrs, &objp->ypbind_lo_vers);
}

static void
write_ypbind3_binding (struct ypbind3_binding *ypb3)
{
  char path3[MAXPATHLEN + 1];
  FILE *fp;

  sprintf (path3, "binding.3");

  if ((fp = fopen (path3, "wce")) == NULL)
    printf ("fopen (%s): %s", path3, strerror (errno));
  else
    {
      XDR xdrs;
      bool_t status;

      xdrstdio_create (&xdrs, fp, XDR_ENCODE);
      status = xdr_ypbind3_binding (&xdrs, ypb3);
      if (!status)
	{
	  printf ("write of %s failed!", path3);
	  unlink (path3);
	}
      xdr_destroy (&xdrs);
      fclose (fp);
    }
}

static void
read_ypbind3_binding (struct ypbind3_binding *ypb3)
{
  char path[100];
  struct ypbind3_binding res;

  snprintf (path, sizeof (path), "binding.3");

  FILE *in = fopen (path, "rce");
  if (in != NULL)
    {
      bool_t status;

      XDR xdrs;
      xdrstdio_create (&xdrs, in, XDR_DECODE);
      memset(ypb3, 0, sizeof (struct ypbind3_binding));
      status = xdr_ypbind3_binding (&xdrs, ypb3);
      if (!status)
        {
          printf ("read of %s failed!", path);
          unlink (path);
        }
      xdr_destroy (&xdrs);
    }
}

int
main (void)
{
   struct ypbind3_binding *ypb3, res;

   ypb3 = __host2ypbind3_binding ("phex");
   ypbind3_binding_dump (ypb3);
   write_ypbind3_binding (ypb3); 

   printf ("\nRead data\n\n");

   read_ypbind3_binding (&res);
   ypbind3_binding_dump (&res);

   return 0;
}
