
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident	"$Revision: 1.17 $"
#include "sam/types.h"
#include "mgmt/util.h"
#include "mgmt/sammgmt.h"

/*
 * media_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of media.h in a machine-independent form
 */
bool_t
xdr_vsnpool_property_t(
XDR *xdrs,
vsnpool_property_t *objp)
{

	if (!xdr_upath_t(xdrs, objp->name))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->number_of_vsn))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->capacity))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->free_space))
		return (FALSE);
	XDR_PTR2LST(objp->catalog_entry_list, catalog_entry_list);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.5.6") <= 0)) {

			return (TRUE); /* versions 1.5.6 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_mtype_t(xdrs, objp->media_type))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_diskvsnpool_property_t(
XDR *xdrs,
vsnpool_property_t *objp)
{

	if (!xdr_upath_t(xdrs, objp->name))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->number_of_vsn))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->capacity))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->free_space))
		return (FALSE);
	XDR_PTR2LST(objp->catalog_entry_list, disk_vol_list);
	return (TRUE);
}

bool_t
xdr_reserve_option_t(
XDR *xdrs,
reserve_option_t *objp)
{

	if (!xdr_time32_t(xdrs, &objp->CerTime))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->CerAsname))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->CerOwner))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->CerFsname))
		return (FALSE);
	return (TRUE);
}

/*
 * function parameters
 *
 *
 * TI-RPC allows a single parameter to be passed from client to server.
 * If more than one parameter is required, the components can be combined
 * into a structure that is counted as a single element
 * Information passed from server to client is passed as the  function's
 * return value. Information cannot be passed back from server to client
 * through the parameter list
 */
bool_t
xdr_equ_range_arg_t(
XDR *xdrs,
equ_range_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->start_slot_no))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->end_slot_no))
		return (FALSE);
	if (!xdr_vsn_sort_key_t(xdrs, &objp->sort_key))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->ascending))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_reserve_arg_t(
XDR *xdrs,
reserve_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->slot))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->partition))
		return (FALSE);
	XDR_PTR2STRUCT(objp->reserve_option, reserve_option_t);
	return (TRUE);
}
