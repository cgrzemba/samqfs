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

#pragma ident	"$Revision: 1.14 $"

#include "mgmt/sammgmt.h"

/*
 * archive_sets_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of archive_sets.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */

typedef ar_set_copy_params_t *ar_set_copy_params_ptr_t;

bool_t
xdr_ar_set_copy_params_ptr_t(
XDR *xdrs,
ar_set_copy_params_ptr_t *objp)
{
	if (!xdr_pointer(xdrs, (char **)objp,
	    sizeof (ar_set_copy_params_t),
	    (xdrproc_t)xdr_ar_set_copy_params_t))
		return (FALSE);
	return (TRUE);
}

typedef vsn_map_t *vsn_map_ptr_t;

bool_t
xdr_vsn_map_ptr_t(
XDR *xdrs,
vsn_map_ptr_t *objp)
{
	if (!xdr_pointer(xdrs, (char **)objp,
	    sizeof (vsn_map_t), (xdrproc_t)xdr_vsn_map_t))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_set_type_t(
XDR *xdrs,
set_type_t *objp)
{
	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_arch_set_t(
XDR *xdrs,
arch_set_t *objp)
{
	if (!xdr_set_name_t(xdrs, objp->name))
		return (FALSE);
	if (!xdr_set_type_t(xdrs, &objp->type))
		return (FALSE);
	XDR_PTR2LST(objp->criteria, ar_set_criteria_list);
	if (!xdr_vector(xdrs, (char *)objp->copy_params, 5,
	    sizeof (ar_set_copy_params_ptr_t),
	    (xdrproc_t)xdr_ar_set_copy_params_ptr_t))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->vsn_maps, 5,
	    sizeof (vsn_map_ptr_t),
	    (xdrproc_t)xdr_vsn_map_ptr_t))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->rearch_copy_params, 5,
	    sizeof (ar_set_copy_params_ptr_t),
	    (xdrproc_t)xdr_ar_set_copy_params_ptr_t))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->rearch_vsn_maps, 5,
	    sizeof (vsn_map_ptr_t),
	    (xdrproc_t)xdr_vsn_map_ptr_t))
		return (FALSE);

#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.2") <= 0)) {

			return (TRUE); /* versions 1.5.2 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_string(xdrs, (char **)&objp->description, ~0))
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
xdr_arch_set_arg_t(
XDR *xdrs,
arch_set_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->set, arch_set_t);
	return (TRUE);
}

bool_t
xdr_str_critlst_arg_t(
XDR *xdrs,
str_critlst_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2LST(objp->critlst, ar_set_criteria_list);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	return (TRUE);
}
