/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "yp.h"
#include "ypv1.h"
#ifndef lint
static char rcsid[] = "$OpenBSD: src/usr.sbin/ypserv/ypserv/Attic/ypserv_xdr_v1.c,v 1.3 2002/01/01 21:15:03 deraadt Exp $";
#endif /* not lint */

bool_t
xdr_ypreqtype(xdrs, objp)
	XDR *xdrs;
	ypreqtype *objp;
{
	int32_t *buf;

	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_ypresptype(xdrs, objp)
	XDR *xdrs;
	ypresptype *objp;
{
	int32_t *buf;

	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_yprequest(xdrs, objp)
	XDR *xdrs;
	yprequest *objp;
{
	if (!xdr_ypreqtype(xdrs, &objp->yp_reqtype)) {
		return (FALSE);
	}
	switch (objp->yp_reqtype) {
	case YPREQ_KEY:
		if (!xdr_ypreq_key(xdrs, &objp->yp_reqbody.yp_req_keytype)) {
			return (FALSE);
		}
		break;
	case YPREQ_NOKEY:
		if (!xdr_ypreq_nokey(xdrs, &objp->yp_reqbody.yp_req_nokeytype)) {
			return (FALSE);
		}
		break;
	case YPREQ_MAP_PARMS:
		if (!xdr_ypmap_parms(xdrs, &objp->yp_reqbody.yp_req_map_parmstype)) {
			return (FALSE);
		}
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_ypresponse(xdrs, objp)
	XDR *xdrs;
	ypresponse *objp;
{
	int32_t *buf;

	if (!xdr_ypresptype(xdrs, &objp->yp_resptype)) {
		return (FALSE);
	}
	switch (objp->yp_resptype) {
	case YPRESP_VAL:
		if (!xdr_ypresp_val(xdrs, &objp->yp_respbody.yp_resp_valtype)) {
			return (FALSE);
		}
		break;
	case YPRESP_KEY_VAL:
		if (!xdr_ypresp_key_val(xdrs, &objp->yp_respbody.yp_resp_key_valtype)) {
			return (FALSE);
		}
		break;
	case YPRESP_MAP_PARMS:
		if (!xdr_ypmap_parms(xdrs, &objp->yp_respbody.yp_resp_map_parmstype)) {
			return (FALSE);
		}
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}
