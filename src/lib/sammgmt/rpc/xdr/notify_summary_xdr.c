
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident	"$Revision: 1.21 $"

#include "mgmt/sammgmt.h"

/*
 * notify_summary_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of notify_summary.h in a
 * machine-independent form
 */
bool_t
xdr_notf_summary_t(
XDR *xdrs,
notf_summary_t *objp)
{
	if (!xdr_vector(xdrs, (char *)objp->admin_name, NOTF_LEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->emailaddr, NOTF_LEN,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.2") <= 0)) {

			if (!xdr_vector(xdrs, (char *)objp->notf_subj_arr, 4,
			    sizeof (boolean_t),
			    (xdrproc_t)xdr_boolean_t))
				return (FALSE);
			return (TRUE); /* versions 1.2 or lower */
		}
	}
#endif /* samrpc_client */

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.1") <= 0)) {

			if (!xdr_vector(xdrs, (char *)objp->notf_subj_arr, 6,
			    sizeof (boolean_t), (xdrproc_t)xdr_boolean_t))
				return (FALSE);
			return (TRUE); /* versions 1.5.1 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_vector(xdrs, (char *)objp->notf_subj_arr, SUBJ_MAX, /* 13 */
	    sizeof (boolean_t), (xdrproc_t)xdr_boolean_t))
		return (FALSE);
	return (TRUE);
}

/*
 * *********************
 *  function parameters
 * *********************
 *
 * TI-RPC allows a single parameter to be passed from client to server.
 * If more than one parameter is required, the components can be combined
 * into a structure that is counted as a single element
 * Information passed from server to client is passed as the  function's
 * return value. Information cannot be passed back from server to client
 * through the parameter list
 */

bool_t
xdr_notify_summary_arg_t(
XDR *xdrs,
notify_summary_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->notf_summ, notf_summary_t);
	return (TRUE);
}

bool_t
xdr_mod_notify_summary_arg_t(
XDR *xdrs,
mod_notify_summary_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_upath_t(xdrs, objp->oldemail))
		return (FALSE);
	XDR_PTR2STRUCT(objp->notf_summ, notf_summary_t);
	return (TRUE);
}

bool_t
xdr_get_email_addrs_arg_t(
XDR *xdrs,
get_email_addrs_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_enum(xdrs, (enum_t *)&objp->subj_wanted))
		return (FALSE);
	return (TRUE);
}
