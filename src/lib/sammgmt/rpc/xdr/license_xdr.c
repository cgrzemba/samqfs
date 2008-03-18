
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

#pragma ident	"$Revision: 1.12 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/license.h"

/*
 * license_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of license.h in a machine-independent form
 */

bool_t
xdr_license_info_t(
XDR *xdrs,
license_info_t *objp)
{
	if (!xdr_int(xdrs, &objp->type))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->expire))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->fsType))
		return (FALSE);
	if (!xdr_uint_t(xdrs, &objp->hostid))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->feature_flags))
		return (FALSE);
	XDR_PTR2LST(objp->media_list, md_license_list);
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
xdr_string_string_arg_t(
XDR *xdrs,
string_string_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str1, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->str2, ~0))
		return (FALSE);
	return (TRUE);
}
