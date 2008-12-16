
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

#pragma ident	"$Revision: 1.15 $"

#include "mgmt/sammgmt.h"

/*
 * restore_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of restoreui.h in a machine-independent form
 */

/*
 * FUNCTION PARAMETERS
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
xdr_int_int_arg_t(
XDR *xdrs,
int_int_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_int(xdrs, &objp->int1))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->int2))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_string_strlst_arg_t(
XDR *xdrs,
string_strlst_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	XDR_PTR2LST(objp->strlst, string_list);
	return (TRUE);
}

bool_t
xdr_string_string_strlst_arg_t(
XDR *xdrs,
string_string_strlst_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->str, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->str2, ~0))
		return (FALSE);
	XDR_PTR2LST(objp->strlst, string_list);
	return (TRUE);
}

bool_t
xdr_version_details_arg_t(
XDR *xdrs,
version_details_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->fsname, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->dumpname, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->filepath, ~0))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_file_restrictions_arg_t(
XDR *xdrs,
file_restrictions_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->fsname, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->dumppath, ~0))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->maxentries))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->filepath, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->restrictions, ~0))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_file_restrictions_more_arg_t(
XDR *xdrs,
file_restrictions_more_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->fsname, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->dumppath, ~0))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->maxentries))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->dir, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->file, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->restrictions, ~0))
		return (FALSE);
	XDR_PTR2STRUCT(objp->morefiles, int);
	return (TRUE);
}

bool_t
xdr_strlst_arg_t(
XDR *xdrs,
strlst_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2LST(objp->lst, string_list);
	return (TRUE);
}

bool_t
xdr_restore_inodes_arg_t(
XDR *xdrs,
restore_inodes_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_string(xdrs, (char **)&objp->fsname, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->dumppath, ~0))
		return (FALSE);
	XDR_PTR2LST(objp->filepaths, string_list);
	XDR_PTR2LST(objp->destinations, string_list);
	XDR_PTR2LST(objp->copies, int_list);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.4.1") < 0)) {
			return (TRUE);
		}
	}
#endif /* SAMRPC_CLIENT */
	/* conflict resolution added in 1.4.1 */
	if (!xdr_enum(xdrs, (enum_t *)&objp->replace))
		return (FALSE);
	return (TRUE);
}
