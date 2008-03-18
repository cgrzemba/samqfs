
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident	"$Revision: 1.10 $"

#include "mgmt/sammgmt.h"

/*
 * release_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of release.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_age_prio_type(
XDR *xdrs,
age_prio_type *objp)
{


	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_rl_age_priority_t(
XDR *xdrs,
rl_age_priority_t *objp)
{


	if (!xdr_float(xdrs, &objp->access_weight))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->modify_weight))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->residence_weight))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_rl_fs_directive_t(
XDR *xdrs,
rl_fs_directive_t *objp)
{


	if (!xdr_uname_t(xdrs, objp->fs))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->releaser_log))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->size_priority))
		return (FALSE);
	if (!xdr_age_prio_type(xdrs, &objp->type))
		return (FALSE);
	if (!xdr_rl_age_priority_t(xdrs, &objp->age_priority.detailed))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->no_release))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->rearch_no_release))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->display_all_candidates))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->min_residence_age))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->debug_partial))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->list_size))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_release_fs_t(
XDR *xdrs,
release_fs_t *objp)
{

	if (!xdr_uname_t(xdrs, objp->fi_name))
		return (FALSE);
	if (!xdr_ushort_t(xdrs, &objp->fi_low))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->used_pct))
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
xdr_rl_fs_directive_arg_t(
XDR *xdrs,
rl_fs_directive_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->rl_fs_directive, rl_fs_directive_t);
	return (TRUE);
}
