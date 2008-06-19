
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

#pragma ident	"$Revision: 1.29 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * filesystem_svr.c
 *
 * RPC server wrapper for filesystem api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * *************
 *  config APIs
 * *************
 */
samrpc_result_t *
samrpc_get_all_fs_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Getting all fs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get all fs");
	ret = get_all_fs(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_FS_FS_LIST, lst);

	Trace(TR_DEBUG, "Get all fs return[%d] with [%d] fs",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_fs_names_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get fs names");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get fs names");
	ret = get_fs_names(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_FS_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get fs names return[%d] with [%d] names",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_fs_names_all_types_3_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get fs names from vfs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get fs names from vfs");
	ret = get_fs_names_all_types(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_FS_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get fs names return[%d] with [%d] names",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_fs_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	fs_t *fs;

	Trace(TR_DEBUG, "Get fs %s", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get fs");
	ret = get_fs(arg->ctx, arg->str, &fs);

	SAMRPC_SET_RESULT(ret, SAM_FS_FS, fs);

	Trace(TR_DEBUG, "Get fs[%s] return[%d]", arg->str, ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_create_fs_1_svr(
	create_fs_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Creating fs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to create fs");

	SET_FS_TAIL(arg->fs_info);

	ret = create_fs(arg->ctx, arg->fs_info, arg->modify_vfstab);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Create fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_change_mount_options_1_svr(
	change_mount_options_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Change mount options for fs[%s]", arg->fsname);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to change mount options");
	ret = change_mount_options(
	    arg->ctx,
	    arg->fsname,
	    arg->options);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Change mount options return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_change_live_mount_options_1_svr(
	change_mount_options_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *failed_options;

	Trace(TR_DEBUG, "Change live mount options for fs[%s]", arg->fsname);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library change live mount options");
	ret = change_live_mount_options(
	    arg->ctx,
	    arg->fsname,
	    arg->options,
	    &failed_options);

	SAMRPC_SET_RESULT(ret, SAM_FS_FAILED_MOUNT_OPTS_LIST, failed_options);

	Trace(TR_DEBUG, "Change live mount options return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_default_mount_options_1_svr(
	get_default_mount_options_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	mount_options_t *mount_opts;

	Trace(TR_DEBUG, "Get default mount options");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library get default mount options");
	ret = get_default_mount_options(
	    arg->ctx,
	    arg->fs_type,
	    arg->dau_size,
	    arg->uses_stripe_groups,
	    arg->shared,
	    arg->multi_reader,
	    &mount_opts);

	SAMRPC_SET_RESULT(ret, SAM_FS_MOUNT_OPTIONS, mount_opts);

	Trace(TR_DEBUG, "Get default mount options return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_remove_fs_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Remove fs[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library remove fs");
	ret = remove_fs(arg->ctx, arg->str);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Remove fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_samfsck_fs_1_svr(
	fsck_fs_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "samfsck fs[%s]", arg->fsname);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library samfsck fs");
	ret = samfsck_fs(arg->ctx, arg->fsname, arg->logfile, arg->repair);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "samfsck fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_all_samfsck_info_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all samfsck info");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library get all samfsck info");
	ret = get_all_samfsck_info(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_FS_FSCK_LIST, lst);

	Trace(TR_DEBUG, "Get all samfsck info return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_mount_fs_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Mounting fs[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to mount fs");
	ret = mount_fs(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Mount fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_umount_fs_1_svr(
	string_bool_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Unmount fs[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to unmount fs");
	ret = umount_fs(arg->ctx, arg->str, arg->bool);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Unmount fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_grow_fs_1_svr(
	grow_fs_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Grow fs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	/* set the tail of the list to a valid pointer */
	if (arg != NULL) {
		SET_LIST_TAIL(arg->additional_meta_data_disk);
		SET_LIST_TAIL(arg->additional_data_disk);
		SET_LIST_TAIL(arg->additional_striped_group);


		SET_FS_TAIL(arg->fs);
	}

	Trace(TR_DEBUG, "Calling native library to grow fs");
	ret = grow_fs(
	    arg->ctx,
	    arg->fs,
	    arg->additional_meta_data_disk,
	    arg->additional_data_disk,
	    arg->additional_striped_group);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Grow fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_create_fs_and_mount_1_svr(
	create_fs_mount_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	int *failed_in_step = NULL;

	Trace(TR_DEBUG, "Creating fs and mount");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to create fs & mount");

	SET_FS_TAIL(arg->fs_info);

	ret = create_fs_and_mount(arg->ctx,
	    arg->fs_info,
	    arg->mount_at_boot,
	    arg->create_mnt_point,
	    arg->mount);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp, in this case the timestamp is
	 * updated only if return is not -1
	 */

	if (ret == 0 || ret == -1) {
		SAMRPC_UPDATE_TIMESTAMP(ret);
		SAMRPC_SET_RESULT(ret, SAM_VOID, 0);
	} else {
		/*
		 * RPC layer has a common handler for all
		 * results from the api
		 * To handle this special case (return codes
		 * of 1, 2, 3, 4, or 5 indicate partial failure
		 * set ret = -2 and populate the failed_in_step
		 * At the client side, decode this value
		 */
		failed_in_step = (int *)mallocer(sizeof (int));
		*failed_in_step = ret;
		ret = -2;
		SAMRPC_UPDATE_TIMESTAMP(ret);
		SAMRPC_SET_RESULT(ret, SAM_PTR_INT, failed_in_step);
	}

	Trace(TR_DEBUG, "Create fs and mount return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_set_device_state_5_0_svr(
	string_int_intlist_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "set device state");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to set device state");

	ret = set_device_state(arg->ctx, arg->str, arg->num, arg->int_lst);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "set device state return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_equipment_ordinals_3_svr(
	int_list_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	int_list_result_t *res = NULL;
	node_t *node = NULL;
	int *eq = NULL;

	Trace(TR_DEBUG, "Get equipment ordinals");

	res = (int_list_result_t *)mallocer(
	    sizeof (int_list_result_t));
	(void) memset(res, 0, sizeof (int_list_result_t));

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get eq ordinals");

	res->lst = NULL;

	if (arg->lst != NULL) {
		node = arg->lst->head;
		res->lst = lst_create();
		while (node != NULL) {

			eq = (int *)malloc(sizeof (int));
			(void) memset(eq, 0, sizeof (int));

			*eq = *(int *)(node->data);
			lst_append(res->lst, eq);
			node = node->next;
		}
		SET_LIST_TAIL(res->lst);
	}

	ret = get_equipment_ordinals(
	    arg->ctx, arg->num, &(res->lst), &(res->first_free));

	SAMRPC_SET_RESULT(ret, SAM_INT_LIST_RESULT, res);

	Trace(TR_DEBUG, "Get equipment ordinals return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_check_equipment_ordinals_3_svr(
	intlist_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Check equipment ordinals");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to check eq ordinal");

	SET_LIST_TAIL(arg->lst);

	ret = check_equipment_ordinals(arg->ctx, arg->lst);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_MISC, "Check equipment ordinals return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_reset_equipment_ordinals_3_svr(
	reset_eq_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Reset equipment ordinals");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to reset eq ordinal on [%s]",
	    arg->str);

	SET_LIST_TAIL(arg->lst);

	ret = reset_equipment_ordinals(arg->ctx, arg->str, arg->lst);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_MISC, "reset equipment ordinals return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_generic_filesystems_4_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Getting generic filesystems");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get generic filesystems");
	ret = get_generic_filesystems(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get generic fs return[%d] with [%d] fs",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_remove_generic_fs_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Remove fs[%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library remove fs");
	ret = remove_generic_fs(arg->ctx, arg->str1, arg->str2);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);


	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Remove fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_mount_generic_fs_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Mounting generic fs[%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to mount fs");
	ret = mount_generic_fs(arg->ctx, arg->str1, arg->str2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Mount generic fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_nfs_opts_4_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get nfs options for [%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get nfs options");
	ret = get_nfs_opts(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get nfs options return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_set_nfs_opts_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Set nfs options for [%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to set nfs options");
	ret = set_nfs_opts(arg->ctx, arg->str1, arg->str2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set nfs options return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_create_arch_fs_6_svr(
	create_arch_fs_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	int *failed_in_step = NULL;

	Trace(TR_DEBUG, "Creating arch fs");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to create arch fs");

	SET_FS_TAIL(arg->fs_info);
	SET_FS_ARCH_CFG_TAIL(arg->arch_cfg);

	ret = create_arch_fs(arg->ctx,
	    arg->fs_info,
	    arg->mount_at_boot,
	    arg->create_mnt_point,
	    arg->mount,
	    arg->arch_cfg);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp, in this case the timestamp is
	 * updated only if return is not -1
	 */

	if (ret == 0 || ret == -1) {
		SAMRPC_UPDATE_TIMESTAMP(ret);
		SAMRPC_SET_RESULT(ret, SAM_VOID, 0);
	} else {
		/*
		 * RPC layer has a common handler for all
		 * results from the api
		 * To handle this special case (return codes
		 * of 1, 2, 3, 4, or 5 indicate partial failure
		 * set ret = -2 and populate the failed_in_step
		 * At the client side, decode this value
		 */
		failed_in_step = (int *)mallocer(sizeof (int));
		*failed_in_step = ret;
		ret = -2;
		SAMRPC_UPDATE_TIMESTAMP(ret);
		SAMRPC_SET_RESULT(ret, SAM_PTR_INT, failed_in_step);
	}

	Trace(TR_DEBUG, "Create arch fs return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_shrink_release_5_0_svr(
	string_string_int_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "shrink release");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);


	Trace(TR_DEBUG, "Calling native library to shrink release");
	ret = shrink_release(
	    arg->ctx,
	    arg->str1, /* fs_name */
	    arg->int1, /* eq_to_release */
	    arg->str2); /* kv_options */

	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "shrink release return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_shrink_remove_5_0_svr(
	string_string_int_int_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "shrink remove");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);


	Trace(TR_DEBUG, "Calling native library to shrink remove");
	ret = shrink_remove(
	    arg->ctx,
	    arg->str1, /* fs_name */
	    arg->int1, /* eq_to_remove */
	    arg->int2, /* replacement eq */
	    arg->str2); /* kv_options */

	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "shrink remove return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_shrink_replace_device_5_0_svr(
	string_string_int_disk_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "shrink replace device");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);


	Trace(TR_DEBUG, "Calling native library to shrink replace device");
	ret = shrink_replace_device(
	    arg->ctx,
	    arg->str1, /* fs_name */
	    arg->int1, /* eq_to_replace */
	    arg->dsk, /* replacement disk */
	    arg->str2); /* kv_options */

	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "shrink replace device return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_shrink_replace_group_5_0_svr(
	string_string_int_group_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "shrink replace group");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);


	Trace(TR_DEBUG, "Calling native library to shrink replace group");
	ret = shrink_replace_group(
	    arg->ctx,
	    arg->str1, /* fs_name */
	    arg->int1, /* eq_to_replace */
	    arg->grp, /* replacement group */
	    arg->str2); /* kv_options */

	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "shrink replace group return[%d]", ret);
	return (&rpc_result);
}
