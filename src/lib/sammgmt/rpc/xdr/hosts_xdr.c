
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

#pragma ident	"$Revision: 1.8 $"

#include "mgmt/sammgmt.h"

/*
 * hosts_xdr.c
 *
 * This file contains the XDR External Data Representation library functions
 * to represent data structures of hosts.h in a machine-independent form
 */
/*
 * ******************
 *  basic structures
 * ******************
 */
bool_t
xdr_host_info_t(
XDR *xdrs,
host_info_t *objp)
{


	if (!xdr_string(xdrs, (char **)&objp->host_name, ~0))
		return (FALSE);

	XDR_PTR2LST(objp->ip_addresses, string_list);

	if (!xdr_int(xdrs, &objp->server_priority))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->mounted_hosts))
		return (FALSE);
	if (!xdr_boolean_t(xdrs, &objp->current_server))
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
xdr_string_host_info_arg_t(
XDR *xdrs,
string_host_info_arg_t *objp)
{


	XDR_PTR2CTX(objp->ctx);

	if (!xdr_string(xdrs, (char **)&objp->fs_name, ~0))
		return (FALSE);

	XDR_PTR2STRUCT(objp->h, host_info_t);

	return (TRUE);
}
