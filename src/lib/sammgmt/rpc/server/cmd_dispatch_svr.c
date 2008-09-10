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
#pragma ident   "$Revision: 1.2 $"

#include <stdlib.h>
#include "mgmt/cmd_dispatch.h"
#include "mgmt/sammgmt.h"
#include "mgmt/config/cfg_fs.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;



samrpc_result_t *
samrpc_handle_request_5_0_svr(
handle_request_arg_t *hr_arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int	ret	= -1;


	Trace(TR_MISC, "Handle request...");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to handle request");


	/*
	 * Always use the ctx from the handle_request_arg ignoring the
	 * one in the included argument.
	 */
	switch (hr_arg->req->func_id) {
	case CMD_MOUNT: {
		string_arg_t *mo_arg =
		    (string_arg_t *)hr_arg->req->args;

		ret = mount_fs(hr_arg->ctx, mo_arg->str);
		break;
	}
	case CMD_UMOUNT: {
		string_bool_arg_t *um_arg =
		    (string_bool_arg_t *)hr_arg->req->args;
		ret = umount_fs(hr_arg->ctx, um_arg->str, um_arg->bool);
		break;
	}
	case CMD_CHANGE_MOUNT_OPTIONS: {
		change_mount_options_arg_t *cm_arg =
		    (change_mount_options_arg_t *)hr_arg->req->args;
		ret = change_mount_options(hr_arg->ctx,
		    cm_arg->fsname,
		    cm_arg->options);
		break;
	}
	case CMD_CREATE_ARCH_FS: {
		create_arch_fs_arg_t *cafs_arg =
		    (create_arch_fs_arg_t *)hr_arg->req->args;
		SET_FS_TAIL(cafs_arg->fs_info);
		SET_FS_ARCH_CFG_TAIL(cafs_arg->arch_cfg);
		ret = create_arch_fs(hr_arg->ctx,
		    cafs_arg->fs_info,
		    cafs_arg->mount_at_boot,
		    cafs_arg->create_mnt_point,
		    cafs_arg->mount,
		    cafs_arg->arch_cfg);

		/*
		 * create_arch_fs has a number of error returns.
		 * flatten them out to success|failure for this
		 * use.
		 */
		if (ret != 0) {
			ret = -1;
		}
		break;
	}
	case CMD_GROW_FS: {
		grow_fs_arg_t *gfs_arg = (grow_fs_arg_t *)hr_arg->req->args;

		SET_LIST_TAIL(gfs_arg->additional_meta_data_disk);
		SET_LIST_TAIL(gfs_arg->additional_data_disk);
		SET_LIST_TAIL(gfs_arg->additional_striped_group);
		SET_FS_TAIL(gfs_arg->fs);

		ret = grow_fs(hr_arg->ctx,
		    gfs_arg->fs,
		    gfs_arg->additional_meta_data_disk,
		    gfs_arg->additional_data_disk,
		    gfs_arg->additional_striped_group);

		break;
	}
	case CMD_REMOVE_FS: {
		string_arg_t *rmfs_arg = (string_arg_t *)hr_arg->req->args;
		ret = remove_fs(hr_arg->ctx, rmfs_arg->str);
		break;
	}
	case CMD_SET_ADV_NET_CONFIG: {
		string_strlst_arg_t *advnet_arg =
		    (string_strlst_arg_t *)hr_arg->req->args;

		SET_LIST_TAIL(advnet_arg->strlst);
		ret = set_advanced_network_cfg(hr_arg->ctx, advnet_arg->str,
		    advnet_arg->strlst);
		break;
	}
	case CMD_ADD_SOSD: {
		add_storage_node_arg_t *asn =
		    (add_storage_node_arg_t *)hr_arg->req->args;

		SET_FS_TAIL(asn->fs);

		ret = add_sosd_on_node(asn->ctx, asn->fs_name, asn->node_name,
		    asn->node_ip, asn->fs, asn->node_data);
		break;
	}
	default:
		ret = -1;
		samerrno = SE_FUNCTION_NOT_SUPPORTED;
		snprintf(samerrmsg, MAX_MSG_LEN,  GetCustMsg(samerrno),
		    hr_arg->req->func_id);
	}

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_MISC, "Handle request return[%d]", ret);
	return (&rpc_result);
}
