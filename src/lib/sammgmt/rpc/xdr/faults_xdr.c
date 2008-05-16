
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident	"$Revision: 1.18 $"

#include "mgmt/sammgmt.h"

/*
 * faults_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of faults.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_fault_state_t(
XDR *xdrs,
fault_state_t *objp)
{


	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_fault_sev_t(
XDR *xdrs,
fault_sev_t *objp)
{


	if (!xdr_enum(xdrs, (enum_t *)objp))
		return (FALSE);
	return (TRUE);
}

bool_t
xdr_fault_attr_t(
XDR *xdrs,
fault_attr_t *objp)
{


	if (!xdr_long(xdrs, &objp->errorID))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->compID))
		return (FALSE);
	if (!xdr_fault_sev_t(xdrs, &objp->errorType))
		return (FALSE);
	if (!xdr_time_t(xdrs, &objp->timestamp))
		return (FALSE);
	if (!xdr_upath_t(xdrs, objp->hostname))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->msg, MAXLINE,
	    sizeof (char), (xdrproc_t)xdr_char))
		return (FALSE);
	if (!xdr_fault_state_t(xdrs, &objp->state))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->library))
		return (FALSE);
	if (!xdr_equ_t(xdrs, &objp->eq))
		return (FALSE);
	return (TRUE);
}

#ifdef	SAMRPC_CLIENT
bool_t
xdr_fault_req_t(
XDR *xdrs,
flt_req_t *objp)
{


	if (!xdr_int(xdrs, &objp->numFaults))
		return (FALSE);
	if (!xdr_fault_sev_t(xdrs, &objp->sev))
		return (FALSE);
	if (!xdr_fault_state_t(xdrs, &objp->state))
		return (FALSE);
	if (!xdr_long(xdrs, &objp->errorID))
		return (FALSE);
	return (TRUE);
}
#endif	/* SAMRPC_CLIENT */

bool_t
xdr_fault_summary_t(
XDR *xdrs,
fault_summary_t *objp)
{


	if (!xdr_int(xdrs, &objp->num_critical_faults))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->num_major_faults))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->num_minor_faults))
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

#ifdef SAMRPC_CLIENT
bool_t
xdr_fault_req_arg_t(
XDR *xdrs,
fault_req_arg_t *objp)
{
	XDR_PTR2CTX(objp->ctx);
	if (!xdr_fault_req_t(xdrs, &objp->fault_req))
		return (FALSE);
	return (TRUE);
}
#endif /* SAMRPC_CLIENT */


bool_t
xdr_fault_errorid_arr_arg_t(
XDR *xdrs,
fault_errorid_arr_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_int(xdrs, &objp->num))
		return (FALSE);
	if (!xdr_vector(xdrs, (char *)objp->errorID, DEFAULTS_MAX,
	    sizeof (long), (xdrproc_t)xdr_long))
		return (FALSE);
	return (TRUE);
}
