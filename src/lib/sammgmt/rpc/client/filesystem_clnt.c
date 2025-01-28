
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

#pragma ident	"$Revision: 1.36 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * get_all_fs returns both the mounted and unmounted
 * file systems.
 */
int
get_all_fs(
ctx_t *ctx,		/* client connection */
sqm_lst_t **fs_list	/* return - list of file systems */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all fs";
	char *err_msg;
	enum clnt_stat stat;


	/* to fix the tail of the list */
	node_t *node_fs;
	fs_t *fs;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_fs, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*fs_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*fs_list));

	if ((*fs_list) != NULL) {
		node_fs = (*fs_list)->head;

		while (node_fs != NULL) {
			fs = (fs_t *)node_fs->data;

			/* set the tail of all lists in fs_t */
			SET_FS_TAIL(fs);

			node_fs = node_fs->next;
		}
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_fs_names returns  the names of all configured filesystem
 */
int
get_fs_names(
ctx_t *ctx,		/* client connection */
sqm_lst_t **fs_names	/* return - list of file system names */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get fs names";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_names)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_fs_names, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*fs_names = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*fs_names));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_fs_names_all_types returns all device ids found in vfstab
 */
int
get_fs_names_all_types(
ctx_t *ctx,		/* client connection */
sqm_lst_t **fs_names	/* return - list of file system names */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get fs names from vfs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_names)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_fs_names_all_types, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*fs_names = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*fs_names));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get the information about a specific file system.
 */
int
get_fs(
ctx_t *ctx,	/* client connection */
uname_t fsname,	/* file system name */
fs_t **fs	/* return - file system info */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, fs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;

	SAMRPC_CLNT_CALL(samrpc_get_fs, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*fs = (fs_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail of all lists within fs_t
	 */
	SET_FS_TAIL((*fs));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * create_fs() is for creating a file system. This single method can
 * be used to create all types of file system.
 *
 * The type of the file system to be created can be derived from the
 * list of devices provided in fs data structure. Same thing for
 * mount options.
 *
 * The caller of this function can initialize the structures in
 * one of two ways.
 *
 * Each disk_t can contain only the au_t info. In this case
 * all eq_ordinals and device types will be selected by the api. Ordinals
 * will be the selected on a lowest available basis. Data disks will all
 * be md devices. Striped groups will take the lowest available striped
 * group id.
 *
 * Alternatively the base_info member of each disk can be set up to indicate
 * the equipment type and equipment ordinal of the device.
 *
 */
int
create_fs(
ctx_t *ctx,		/* client connection */
fs_t *fs_info,		/* file system info */
boolean_t modify_vfstab	/* should the vfstab be modified */
)
{
	int ret_val;
	create_fs_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:create fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_info)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fs_info = fs_info;
	arg.modify_vfstab = modify_vfstab;

	SAMRPC_CLNT_CALL(samrpc_create_fs, create_fs_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * change the mount options of a file system.
 */
int
change_mount_options(
ctx_t *ctx,			/* client connection */
uname_t fsname,			/* file system name */
mount_options_t *options	/* mount options */
)
{
	int ret_val;
	change_mount_options_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:change mount options";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, options)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strcpy(arg.fsname, fsname);
	arg.options = (mount_options_t *)options;

	SAMRPC_CLNT_CALL(samrpc_change_mount_options,
	    change_mount_options_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * change the mount options of a live file system.
 */
int
change_live_mount_options(
ctx_t *ctx,			/* client connection */
uname_t fsname,			/* file system name */
mount_options_t *options,	/* mount options */
sqm_lst_t **failed_options	/* return - list of failed mount options */
)
{
	int ret_val;
	change_mount_options_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:change live mount options";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, options, failed_options)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strcpy(arg.fsname, fsname);
	arg.options = (mount_options_t *)options;

	SAMRPC_CLNT_CALL(samrpc_change_live_mount_options,
	    change_mount_options_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*failed_options =
	    (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*failed_options));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get the default mount options for a file system of the type described.
 */
int
get_default_mount_options(
ctx_t *ctx,			/* client connection */
devtype_t fs_type,		/* file system type */
int dau_size,			/* dau size */
boolean_t uses_stripe_groups,	/* stripe */
boolean_t shared,		/* shared fs */
boolean_t multi_reader,		/* multi-reader fs */
mount_options_t **params	/* return - mount options */
)
{
	int ret_val;
	get_default_mount_options_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default mount options";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_type, params)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strcpy(arg.fs_type, fs_type);
	arg.dau_size = dau_size;
	arg.uses_stripe_groups = uses_stripe_groups;
	arg.shared = shared;
	arg.multi_reader = multi_reader;

	SAMRPC_CLNT_CALL(samrpc_get_default_mount_options,
	    get_default_mount_options_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*params = (mount_options_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Function to remove file systems from SAM-FS/QFS's control.
 * The named file system will have its devices removed from the mcf,
 * entries removed from vfstab, and mount options removed from samfs.cmd
 */
int
remove_fs(
ctx_t *ctx,	/* client connection */
uname_t fsname	/* file system name */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;

	SAMRPC_CLNT_CALL(samrpc_remove_fs, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Function to check consistency of a file system.
 */
int
samfsck_fs(
ctx_t *ctx,		/* client connection */
uname_t fsname,		/* file system name */
upath_t logfile,	/* path to log file */
boolean_t repair	/* repair the file system */
)
{
	int ret_val;
	fsck_fs_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:samfsck fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strcpy(arg.fsname, fsname);
	strcpy(arg.logfile, logfile);
	arg.repair = repair;

	SAMRPC_CLNT_CALL(samrpc_samfsck_fs, fsck_fs_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_all_samfsck_info
 */
int
get_all_samfsck_info(
ctx_t *ctx,		/* client connection */
sqm_lst_t **fsck_info	/* return - list of fsck info */
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all samfsck info";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsck_info)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_samfsck_info, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*fsck_info = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*fsck_info));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Function to mount a file system.
 */
int
mount_fs(
ctx_t *ctx,		/* client connection */
upath_t fsname		/* file system name */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:mount fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;

	SAMRPC_CLNT_CALL(samrpc_mount_fs, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Function to unmount a file system.
 */
int
umount_fs(
ctx_t *ctx,	/* client connection */
upath_t fsname, /* file system name */
boolean_t force	/* forcefully umount. for future use. must be B_FALSE in 4.1 */
)
{
	int ret_val;
	string_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:umount fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;
	arg.bool = force;

	SAMRPC_CLNT_CALL(samrpc_umount_fs, string_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * A file system can be grown only when it is unmounted and drives related
 * to archiving the file system are idle.
 */
int
grow_fs(
ctx_t *ctx,					/* client connection */
fs_t *fs,					/* file system info */
sqm_lst_t *additional_meta_data_disk,
sqm_lst_t *additional_data_disk,
sqm_lst_t *additional_striped_group
)
{
	int ret_val;
	grow_fs_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:grow fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fs = fs;
	arg.additional_meta_data_disk = (sqm_lst_t *)additional_meta_data_disk;
	arg.additional_data_disk = (sqm_lst_t *)additional_data_disk;
	arg.additional_striped_group = (sqm_lst_t *)additional_striped_group;

	SAMRPC_CLNT_CALL(samrpc_grow_fs, grow_fs_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * create_fs_and_mount
 *
 * This function was requested to perform all of the operations that the GUI
 * performs. It will create the filesystem fs, conditionally setting
 * mount_at_boot in the /etc/vfstab. Also it will conditionally create
 * the mount point if it does not exist and mount the filesystem
 * after creation.
 *
 * If the license for the system on which the command is executed is for samfs
 * or sam-qfs, archive_meta will be set to off for the filesystem in the
 * archiver.cmd file.
 *
 * This function's return value indicates which step of this multistep
 * operation failed.
 *
 * The steps are as follows:
 * -1 initial checks failed.
 * 1. create the filesystem- this includes setting mount at boot.
 * 2. create the mount point directory.
 * 3. setup the archiver.cmd file.
 * 4. activate the archiver.cmd file.
 * 5. mount the filesystem.
 *
 * If warnings are detected in the archiver.cmd file this function continues on
 * and returns success if the subsequent steps are successful. If errors are
 * detected an error will be returned.
 */
int
create_fs_and_mount(
ctx_t *ctx,		/* client connection */
fs_t *fs_info,		/* file system info */
boolean_t mount_at_boot,	/* modify vfstab to mount at boot time */
boolean_t create_mnt_point,	/* create mount point */
boolean_t mount		/* mount fs */
)
{
	int ret_val;
	create_fs_mount_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:create fs and mount";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_info)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fs_info = fs_info;
	arg.mount_at_boot = mount_at_boot;
	arg.create_mnt_point = create_mnt_point;
	arg.mount = mount;

	SAMRPC_CLNT_CALL(samrpc_create_fs_and_mount, create_fs_mount_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);
	if (result.status == 0 || result.status == -1) {
		ret_val = result.status;
	} else {
		ret_val =  *((int *)
		    result.samrpc_result_u.result.result_data);

		free(result.samrpc_result_u.result.result_data);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
create_arch_fs(
ctx_t *ctx,		/* client connection */
fs_t *fs_info,		/* file system info */
boolean_t mount_at_boot,	/* modify vfstab to mount at boot time */
boolean_t create_mnt_point,	/* create mount point */
boolean_t mount,		/* mount fs */
fs_arch_cfg_t *arch_cfg		/* default arch set config info */
)
{
	int ret_val;
	create_arch_fs_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:create arch fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_info)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fs_info = fs_info;
	arg.mount_at_boot = mount_at_boot;
	arg.create_mnt_point = create_mnt_point;
	arg.mount = mount;
	arg.arch_cfg = arch_cfg;

	SAMRPC_CLNT_CALL(samrpc_create_arch_fs, create_arch_fs_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);
	if (result.status == 0 || result.status == -1) {
		ret_val = result.status;
	} else {
		ret_val =  *((int *)
		    result.samrpc_result_u.result.result_data);

		free(result.samrpc_result_u.result.result_data);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
set_device_state(ctx_t *ctx, char *fs_name, dstate_t new_state,
    sqm_lst_t *eqs) {

	int ret_val;
	string_int_intlist_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set device state";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, eqs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fs_name;
	arg.num = new_state;
	arg.int_lst = eqs;

	SAMRPC_CLNT_CALL(samrpc_set_device_state, string_int_intlist_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}



/*
 * get equipment ordinals
 * If there are insufficent available equipment
 * ordinals an error will be returned.
 *
 * It is important to note that in_use is an input and output argument.
 * first_eq is an output argument.
 *
 * in_use when created by this function will be in increasing
 * order. If it is created in any other manner it must still be in
 * increasing order.
 */
int
get_equipment_ordinals(
ctx_t *ctx,		/* client connection */
int eqs_needed,		/* number of ordinals that are needed */
sqm_lst_t **in_use,	/* ordinals that are in use */
int **first_free	/* first free eq */
)
{
	int ret_val;
	int_list_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get equipment ordinals";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(in_use, first_free)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.num = eqs_needed;
	arg.lst = *in_use;

	SAMRPC_CLNT_CALL(samrpc_get_equipment_ordinals, int_list_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);
	ret_val = result.status;

	*in_use = (sqm_lst_t *)((int_list_result_t *)
	    result.samrpc_result_u.result.result_data)->lst;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*in_use));

	*first_free = ((int_list_result_t *)
	    result.samrpc_result_u.result.result_data)->first_free;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * check equipment ordinals
 *
 * This function can be called to check if a list of equipment ordinals
 * are available for use. If any of the ordinals are already in use an
 * exception will be returned
 */
int
check_equipment_ordinals(
ctx_t *ctx,	/* client connection */
sqm_lst_t *eqs	/* list of equipment ordinals to check */
)
{
	int ret_val;
	intlist_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:check equipment ordinals";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(eqs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.lst = eqs;

	SAMRPC_CLNT_CALL(samrpc_check_equipment_ordinals, intlist_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);
	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * reset equipment ordinals
 *
 * In a shared setup, while adding clients, it is possible that the eq
 * ordinals on the client are already in use. This function is called
 * to reset the equipment ordinals on all the MDS/Pot-MDS and SC.
 *
 */
int
reset_equipment_ordinals(
ctx_t *ctx,	/* client connection */
char *fsname,	/* name of shared filesystem */
sqm_lst_t *eqs	/* list of equipment ordinals to reset */
)
{
	int ret_val;
	reset_eq_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:reset equipment ordinals";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, eqs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fsname;
	arg.lst = eqs;

	SAMRPC_CLNT_CALL(samrpc_reset_equipment_ordinals, reset_eq_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);
	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_generic_filesystems
 *
 * DESCRIPTION:
 * get_generic_filesystems returns a list of key-value pairs
 * describing the file system
 * PARAMS:
 *   ctx_t *	  IN   - context object
 *   char *	  IN   - filesystem type names
 *   sqm_lst_t **	  OUT  - a list of strings
 *
 * format of key-value pairs:
 *  name=<name>,
 *  mountPoint=<mntPt>,
 *  type=<ufs|zfs|...>
 *  state=mounted|umounted,
 *  capacity=<capacity>,
 *  availSpace=<space available>
 *
 * RETURNS:
 *   success  -	 0
 *   error    -	 -1
 */
int
get_generic_filesystems(
ctx_t *ctx,		/* client connection */
char *filter,		/* filesystem type names */
sqm_lst_t **fs_list	/* return - list of strings describing file systems */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get generic fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = filter;

	SAMRPC_CLNT_CALL(samrpc_get_generic_filesystems, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*fs_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*fs_list));

	PTRACE(2, "%s returning with status [%d] and [%d] fs...",
	    func_name, ret_val, (*fs_list != NULL) ? (*fs_list)->length : -1);

	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Function to mount a file system.
 */
int
mount_generic_fs(
ctx_t *ctx,		/* client connection */
char *fsname,		/* file system name */
char *type
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:mount generic fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = (char *)fsname;
	arg.str2 = (char *)type;

	SAMRPC_CLNT_CALL(samrpc_mount_generic_fs, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Function to remove generic file systems
 */
int
remove_generic_fs(
ctx_t *ctx,	/* client connection */
char *fsname,	/* file system name */
char *type
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove generic fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = (char *)fsname;
	arg.str2 = (char *)type;

	SAMRPC_CLNT_CALL(samrpc_remove_generic_fs, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_nfs_opts
 *
 * DESCRIPTION:
 * get nfs opts returns a list of key-value pairs
 * describing the nfs optiosn
 *
 * format of key-value pairs:
 *
 * RETURNS:
 *   success  -	 0
 *   error    -	 -1
 */
int
get_nfs_opts(
ctx_t *ctx,		/* client connection */
char *mount_point,	/* filesystem mount point */
sqm_lst_t **opts	/* return - list of strings describing nfs options */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get nfs opts";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(opts)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = mount_point;

	SAMRPC_CLNT_CALL(samrpc_get_nfs_opts, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*opts = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*opts));

	PTRACE(2, "%s returning with status [%d] and [%d] opts...",
	    func_name, ret_val, (*opts != NULL) ? (*opts)->length : -1);

	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * set_nfs_opts
 *
 * DESCRIPTION:
 * set nfs opts sets the nfs options for a mount point
 *
 * RETURNS:
 *   success  -	 0
 *   error    -	 -1
 */
int
set_nfs_opts(
ctx_t *ctx,		/* client connection */
char *mount_point,	/* filesystem mount point */
char *opts		/* nfs options */
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set nfs opts";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(mount_point)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = mount_point;
	arg.str2 = opts;

	SAMRPC_CLNT_CALL(samrpc_set_nfs_opts, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);

	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
shrink_release(ctx_t *ctx, char *fs_name, int eq_to_release, char *kv_options) {

	int ret_val;
	string_string_int_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:shrink release";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fs_name;
	arg.str2 = kv_options;
	arg.int1 = eq_to_release;

	SAMRPC_CLNT_CALL(samrpc_shrink_release, string_string_int_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
shrink_remove(ctx_t *ctx, char *fs_name, int eq_to_remove, int replacement_eq,
    char *kv_options) {
	int ret_val;
	string_string_int_int_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:shrink remove";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fs_name;
	arg.str2 = kv_options;
	arg.int1 = eq_to_remove;
	arg.int2 = replacement_eq;

	SAMRPC_CLNT_CALL(samrpc_shrink_remove, string_string_int_int_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
shrink_replace_device(ctx_t *ctx, char *fs_name, int eq_to_replace,
    disk_t *replacement, char *kv_options) {

	int ret_val;
	string_string_int_disk_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:shrink replace device";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, replacement)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fs_name;
	arg.str2 = kv_options;
	arg.int1 = eq_to_replace;
	arg.dsk = replacement;

	SAMRPC_CLNT_CALL(samrpc_shrink_replace_device,
	    string_string_int_disk_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
shrink_replace_group(ctx_t *ctx, char *fs_name, int eq_to_replace,
    striped_group_t *replacement, char *kv_options) {

	int ret_val;
	string_string_int_group_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:shrink replace group";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, replacement)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fs_name;
	arg.str2 = kv_options;
	arg.int1 = eq_to_replace;
	arg.grp = replacement;

	SAMRPC_CLNT_CALL(samrpc_shrink_replace_group,
	    string_string_int_group_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}



/*
 * Function to mount a shared file system on multiple clients.
 */
int
mount_clients(
ctx_t *ctx,		/* client connection */
char *fsname,		/* file system name */
char *clients[],
int client_count)
{
	int ret_val;
	str_cnt_strarray_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:mount clients";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, clients)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;
	arg.cnt = (unsigned int)client_count;
	arg.array = clients;

	SAMRPC_CLNT_CALL(samrpc_mount_clients, str_cnt_strarray_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Function to unmount a shared file system on multiple clients.
 */
int
unmount_clients(
ctx_t *ctx,		/* client connection */
char *fsname,		/* file system name */
char *clients[],
int client_count)
{
	int ret_val;
	str_cnt_strarray_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:unmount clients";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, clients)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;
	arg.cnt = (unsigned int)client_count;
	arg.array = clients;

	SAMRPC_CLNT_CALL(samrpc_unmount_clients, str_cnt_strarray_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * change the mount options of a file system.
 */
int
change_shared_fs_mount_options(
ctx_t *ctx,			/* client connection */
char *fs_name,			/* file system name */
char *clients[],
int client_count,
mount_options_t *options	/* mount options */
)
{
	int ret_val;
	str_cnt_strarray_mntopts_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:change shared fs mount options";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, clients, options)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fs_name;
	arg.array = clients;
	arg.cnt = (unsigned int)client_count;
	arg.mo = options;

	SAMRPC_CLNT_CALL(samrpc_change_shared_fs_mount_options,
	    str_cnt_strarray_mntopts_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
get_shared_fs_summary_status(
ctx_t *ctx,
char *fs_name,
sqm_lst_t **lst)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get_shared_fs_summary_status";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fs_name;

	SAMRPC_CLNT_CALL(samrpc_get_shared_fs_summary_status, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*lst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*lst));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
