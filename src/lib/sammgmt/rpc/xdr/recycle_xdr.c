
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

#pragma ident	"$Revision: 1.13 $"

#include "mgmt/sammgmt.h"

/*
 * recycle_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of recycle.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */

bool_t
xdr_no_rc_vsns_t(
XDR *xdrs,
no_rc_vsns_t *objp)
{


	if (!xdr_mtype_t(xdrs, objp->media_type))
		return (FALSE);
	XDR_PTR2LST(objp->vsn_exp, string_list);
	return (TRUE);
}

bool_t
xdr_rc_param_t(
XDR *xdrs,
rc_param_t *objp)
{


	if (!xdr_int(xdrs, &objp->hwm))
		return (FALSE);
	if (!xdr_fsize_t(xdrs, &objp->data_quantity))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->ignore))
		return (FALSE);
	if (!xdr_uname_t(xdrs, objp->email_addr))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->mingain))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->vsncount))
		return (FALSE);
	if (!xdr_uint32_t(xdrs, &objp->change_flag))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->mail))
		return (FALSE);
#ifdef SAMRPC_CLIENT
	if (xdrs->x_op == XDR_DECODE || xdrs->x_op == XDR_ENCODE) {
		if ((xdrs->x_public != NULL) &&
		    (strcmp(xdrs->x_public, "1.3.3") <= 0)) {

			return (TRUE); /* versions 1.3.3 or lower */
		}
	}
#endif /* samrpc_client */
	if (!xdr_int(xdrs, &objp->minobs))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_rc_robot_cfg_t(
XDR *xdrs,
rc_robot_cfg_t *objp)
{


	if (!xdr_upath_t(xdrs, objp->robot_name))
		return (FALSE);
	if (!xdr_rc_param_t(xdrs, &objp->rc_params))
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
xdr_no_rc_vsns_arg_t(
XDR *xdrs,
no_rc_vsns_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->no_recycle_vsns, no_rc_vsns_t);
	return (TRUE);
}

bool_t
xdr_rc_robot_cfg_arg_t(
XDR *xdrs,
rc_robot_cfg_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->rc_robot_cfg, rc_robot_cfg_t);
	return (TRUE);
}

bool_t
xdr_rc_upath_arg_t(
XDR *xdrs,
rc_upath_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);
	if (!xdr_upath_t(xdrs, objp->path))
		return (FALSE);
	return (TRUE);
}
