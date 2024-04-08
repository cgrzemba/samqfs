
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

#pragma ident	"$Revision: 1.14 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/load.h"

/*
 * load_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of load.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_pending_load_info_t(
XDR *xdrs,
pending_load_info_t *objp)
{


	if (!xdr_int(xdrs, &objp->id))
		return (FALSE);
	if (!xdr_u_short(xdrs, &objp->flags))
		return (FALSE);
	if (!xdr_u_short(xdrs, &objp->count))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->robot_equ))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->remote_equ))
		return (FALSE);
	if (!xdr_mtype_t(xdrs, objp->media))
		return (FALSE);
	if (!xdr_u_int(xdrs, &objp->slot))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->ptime))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->age))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->priority))
		return (FALSE);
	if (!xdr_pid_t(xdrs, &objp->pid))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->user))
		return (FALSE);
	if (!xdr_vsn_t(xdrs, objp->vsn))
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
xdr_clear_load_request_arg_t(
XDR *xdrs,
clear_load_request_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_vsn_t(xdrs, objp->vsn))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->index))
		return (FALSE);
	return (TRUE);
}
