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
#pragma ident   "$Revision: 1.3 $"


#include "mgmt/sammgmt.h"

static struct {
	dispatch_id_t	id;
	int		rpc_id;
	char		*func_name;
	bool_t		(*converter)(XDR *, void *);
	size_t		struct_sz;
} cmd_map[] = {
	{ 0, samrpc_mount_fs, "mount", xdr_string_arg_t,
		sizeof (string_arg_t)},
	{ 1, samrpc_umount_fs, "umount", xdr_string_bool_arg_t,
		sizeof (string_bool_arg_t)},
	{ 2, samrpc_change_mount_options, "change mount options",
		xdr_change_mount_options_arg_t,
		sizeof (change_mount_options_arg_t)},
	{ 3, samrpc_create_arch_fs, "create fs", xdr_create_arch_fs_arg_t,
		sizeof (create_arch_fs_arg_t)},
	{ 4, samrpc_grow_fs, "grow fs", xdr_grow_fs_arg_t,
		sizeof (grow_fs_arg_t)},
	{ 5, samrpc_remove_fs, "remove fs", xdr_string_arg_t,
		sizeof (string_arg_t)},
	{ 6, samrpc_set_advanced_network_cfg, "set advanced network config",
		xdr_string_strlst_arg_t, sizeof (string_strlst_arg_t)},
};


bool_t xdr_op_req_t(
XDR *xdrs,
op_req_t *objp) {

	if (!xdr_enum(xdrs, (enum_t *)&objp->func_id))
		return (FALSE);
	if (!xdr_string(xdrs, (char **)&objp->job_id, ~0))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->host_num))
		return (FALSE);

	if (objp->func_id < 0 && objp->func_id <= MAX_DISPATCH_ID) {
		return (FALSE);
	} else if (!xdr_pointer(xdrs, (char **)&objp->args,
	    cmd_map[objp->func_id].struct_sz,
	    (xdrproc_t)cmd_map[objp->func_id].converter)) {
		return (FALSE);
	}

	return (TRUE);
}


bool_t
xdr_handle_request_arg_t(
XDR *xdrs,
handle_request_arg_t *objp)
{

	XDR_PTR2CTX(objp->ctx);
	XDR_PTR2STRUCT(objp->req, op_req_t);

	return (TRUE);
}
