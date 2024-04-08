
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

#pragma ident	"$Revision: 1.13 $"

#include "mgmt/sammgmt.h"

/*
 * diskvols_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of diskvols.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_disk_vol_t(
XDR *xdrs,
disk_vol_t *objp)
{

	if (!xdr_vsn_t(xdrs, objp->vol_name))
		return (FALSE);
	if (!xdr_host_t(xdrs, objp->host))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->path))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->set_flags))
		return (FALSE);
	/* added in 4.4 release */
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.2") <= 0)) {

			return (TRUE); /* versions 1.2 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_fsize_t(xdrs, &objp->capacity))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->free_space))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->status_flags))
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
xdr_disk_vol_arg_t(
XDR *xdrs,
disk_vol_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->disk_vol, disk_vol_t);
	return (TRUE);
}
