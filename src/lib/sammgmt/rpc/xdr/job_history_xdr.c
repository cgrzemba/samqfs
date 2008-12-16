
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

#pragma ident	"$Revision: 1.11 $"

#include "mgmt/sammgmt.h"

/*
 * job_history_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of job_history.h in a machine-independent form
 *
 */
/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_job_type_t(
XDR *xdrs,
job_type_t *objp)
{


	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_job_hdr_t(
XDR *xdrs,
job_hdr_t *objp)
{


	if (!xdr_int(xdrs, &objp->jobID))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->jobType))
		return (FALSE);


	return (TRUE);
}

bool_t
xdr_job_hist_t(
XDR *xdrs,
job_hist_t *objp)
{

	if (!xdr_job_hdr_t(xdrs, &objp->job_hdr))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->lastRan))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->fn.fileName))
		return (FALSE);


	return (TRUE);
}

/*
 *
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
xdr_job_type_arg_t(
XDR *xdrs,
job_type_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_job_type_t(xdrs, &objp->job_type))
		return (FALSE);


	return (TRUE);
}

bool_t
xdr_job_hist_arg_t(
XDR *xdrs,
job_hist_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	if (!xdr_job_type_t(xdrs, &objp->job_type))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->fsname))
		return (FALSE);


	return (TRUE);
}
