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

#pragma ident	"$Revision: 1.19 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * set_csd_params
 * specifies when metadata dumps are to be taken for the specified filesystem.
 * The parameters are set with a single string containing keyword=value pairs.
 */
int
set_csd_params(
ctx_t *ctx,		/* client connection */
char *fsname,	/* input - file system name */
char *parameters	/* input - keyword-value pairs of params */
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set csd params";
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
	arg.str1 = fsname;
	arg.str2 = parameters;

	SAMRPC_CLNT_CALL(samrpc_set_csd_params, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_csd_params
 * gets the current information residing in /etc/opt/SUNWsamfs/csd.cmd,
 * specifying when and where metadata dumps are to be taken for the specified
 * file structure.
 * If the file structure is not known or metadata dumping has not been
 * specified for it, parameters is an empty string
 */
int
get_csd_params(
ctx_t *ctx,	/* client connection */
char *fsname,	/* input - filesystem */
char **parameters	/* return - key-value pairs */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get cds params";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, parameters)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;

	SAMRPC_CLNT_CALL(samrpc_get_csd_params, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*parameters = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d][%s]...",
	    func_name, ret_val, (*parameters != NULL) ? *parameters: "NULL");
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * list_dumps
 * gets list of dumps known to the restore process
 *
 */
int
list_dumps(
ctx_t *ctx,		/* client connection */
char *fsname,	/* filesystem */
sqm_lst_t **dumps		/* return - list of strings */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:list dumps";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, dumps)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fsname;

	SAMRPC_CLNT_CALL(samrpc_list_dumps, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*dumps = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*dumps));

	PTRACE(2, "%s returning with status [%d] and [%d] dumps...",
	    func_name, ret_val, (*dumps != NULL) ? (*dumps)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * list_dumps_by_dir
 * gets list of dumps known to the restore process in a specified directory
 *
 */
int
list_dumps_by_dir(
ctx_t *ctx,		/* client connection */
char *fsname,		/* filesystem */
char *usepath,		/* directory specification */
sqm_lst_t **dumps		/* return - list of strings */
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:list dumps by dir";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, dumps)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fsname;
	arg.str2 = usepath;

	SAMRPC_CLNT_CALL(samrpc_list_dumps_by_dir, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*dumps = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*dumps));

	PTRACE(2, "%s returning with status [%d] and [%d] dumps...",
	    func_name, ret_val, (*dumps != NULL) ? (*dumps)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_dump_status
 * gets detailed status about one or more dumps
 *
 */
int
get_dump_status(
ctx_t *ctx,		/* client connection */
char *fsname,		/* filesystem */
sqm_lst_t *dumps,		/* list of dumps */
sqm_lst_t **status		/* return - list of strings */
)
{
	int ret_val;
	string_strlst_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get dump status";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, status)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fsname;
	arg.strlst = dumps;

	SAMRPC_CLNT_CALL(samrpc_get_dump_status, string_strlst_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*status = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*status));

	PTRACE(2, "%s returning with [%d] and [%d]status strings...",
	    func_name, ret_val, (*status != NULL) ? (*status)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_dump_status_by_dir
 * gets detailed status about one or more dumps in a specified directory
 *
 */
int
get_dump_status_by_dir(
ctx_t *ctx,		/* client connection */
char *fsname,		/* filesystem */
char *usepath,		/* directory specification */
sqm_lst_t *dumps,		/* list of dumps */
sqm_lst_t **status		/* return - list of strings */
)
{
	int ret_val;
	string_string_strlst_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get dump status by dir";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, status)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fsname;
	arg.str2 = usepath;
	arg.strlst = dumps;

	SAMRPC_CLNT_CALL(samrpc_get_dump_status_by_dir,
	    string_string_strlst_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*status = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*status));

	PTRACE(2, "%s returning with [%d] and [%d]status strings...",
	    func_name, ret_val, (*status != NULL) ? (*status)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * decompress_dump
 * decompress dump
 *
 */
int
decompress_dump(
ctx_t *ctx,	/* client connection */
char *fsname,	/* input - fsname */
char *dumppath,	/* input - dumppath */
char **id	/* return - unique id identifing this task */
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:decompress dump";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(id)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fsname;
	arg.str2 = dumppath;

	SAMRPC_CLNT_CALL(samrpc_decompress_dump, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*id = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with [%d] and id [%s]...",
	    func_name, ret_val, (*id != NULL) ? (*id) : "NULL");
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * cleanup_dump
 * Clean up clutter created by decompress_dump
 *
 */
int
cleanup_dump(
ctx_t *ctx,	/* client connection */
char *fsname,	/* input - fsname */
char *dumppath	/* input - dumppath */
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:cleanup dump";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	if (ISNULL(fsname, dumppath)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fsname;
	arg.str2 = dumppath;

	SAMRPC_CLNT_CALL(samrpc_cleanup_dump, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with [%d] ...",	func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
/*
 * delete_dump
 * Delete all files associated with a metadata snapshot
 */
int
delete_dump(
	ctx_t *ctx,	/* client connection */
	char *fsname,	/* input - fsname */
	char *dumppath	/* input - dumppath */
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:delete dump";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	if (ISNULL(fsname, dumppath)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fsname;
	arg.str2 = dumppath;

	SAMRPC_CLNT_CALL(samrpc_delete_dump, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with [%d] ...",	func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * take_dump
 * request taking a samfsdump of a file system in real time, rather
 * than periodic dumps described by csd_params
 * This request is asynchronous, the samfsdump is started in the background
 * and the call returns. An unique id is returned to track the process
 *
 */
int
take_dump(
ctx_t *ctx,	/* client connection */
char *fsname,	/* input - fsname */
char *dumppath,	/* input - dumppath */
char **id	/* return - unique id identifing this task */
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:take dump";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(id)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fsname;
	arg.str2 = dumppath;

	SAMRPC_CLNT_CALL(samrpc_take_dump, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*id = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with [%d] and id [%s]...",
	    func_name, ret_val, (*id != NULL) ? (*id) : "NULL");
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * list_versions
 */
int
list_versions(
ctx_t *ctx,	/* client connection */
char *fsname,	/* input - filesystem name */
char *dumppath,	/* input - dumppath */
int maxentries,	/* input - max entries */
char *filepath,	/* input - filepath */
char *restrictions,	/* input - contraints */
sqm_lst_t **versions	/* return - list of strings */
)
{
	int ret_val;
	file_restrictions_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:list versions";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(versions)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fsname = fsname;
	arg.dumppath = dumppath;
	arg.maxentries = maxentries;
	arg.filepath = filepath;
	arg.restrictions = restrictions;

	SAMRPC_CLNT_CALL(samrpc_list_versions, file_restrictions_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*versions = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*versions));

	PTRACE(2, "%s returning with status [%d] with [%d] entries...",
	    func_name, ret_val, (*versions != NULL) ? (*versions)->length: -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_version_details
 * gets details about a specific version of a file
 *
 */
int
get_version_details(
ctx_t *ctx,	/* client connection */
char *fsname,	/* input - filesystem name */
char *dumppath,	/* input - dumppath */
char *filepath,	/* input - filepath */
sqm_lst_t **details /* return - list of strings */
)
{
	int ret_val;
	version_details_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get version details";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(details)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fsname = fsname;
	arg.dumpname = dumppath;
	arg.filepath = filepath;

	SAMRPC_CLNT_CALL(samrpc_get_version_details, version_details_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);
	ret_val = result.status;
	*details = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*details));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * search_versions
 */
int
search_versions(
ctx_t *ctx,					/* client connection */
char *fsname,	/* filesystem name */
char *dumppath,
int maxentries,	/* input - max entries */
char *filepath,	/* input - filepath */
char *restrictions,	/* input - contraints */
char **id /* return - unique id identifying the search */
)
{
	int ret_val;
	file_restrictions_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:search versions";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(id)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fsname = fsname;
	arg.dumppath = dumppath;
	arg.maxentries = maxentries;
	arg.filepath = filepath;
	arg.restrictions = restrictions;

	SAMRPC_CLNT_CALL(samrpc_search_versions, file_restrictions_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*id = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d] with id [%s]...",
	    func_name, ret_val, (*id != NULL) ? (*id): "NULL");
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_search_results
 * get the results of a previously issued search
 *
 */
int
get_search_results(
ctx_t *ctx,	/* client connection */
char *fsname,	/* input - filesystem */
sqm_lst_t **results	/* return - list of path strings */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get search results";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, results)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;

	SAMRPC_CLNT_CALL(samrpc_get_search_results, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*results = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d] with [%d] results...",
	    func_name, ret_val,
	    (*results != NULL) ? (*results)->length: -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * restore_inodes
 * requests that a set of old inodes be restored
 *
 * staging is optional (copies specifies the archive copy number to be staged)
 */
int
restore_inodes(
ctx_t *ctx,	/* client connection */
char *fsname,	/* input - fsname */
char *dumppath,	/* input - list of integers */
sqm_lst_t *filepaths,	/* input - list of strings describing the inode */
sqm_lst_t *destinations, /* input - list of strings */
sqm_lst_t *copies,	/* input - list of integers */
replace_t replace,	/* input - how to resolve conflicts */
char **id		/* return - unique id to track the restore */
)
{
	int ret_val;
	restore_inodes_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:restore inodes";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fsname = fsname;
	arg.dumppath = dumppath;
	arg.filepaths = filepaths;
	arg.destinations = destinations;
	arg.copies = copies;
	arg.replace = replace;

	SAMRPC_CLNT_CALL(samrpc_restore_inodes, restore_inodes_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*id = (char *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * set_snapshot_locked
 * Make snapshot undeletable
 */
int
set_snapshot_locked(
	ctx_t *ctx,	/* client connection */
	char *snapname
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set snapshot locked";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	if (ISNULL(snapname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = snapname;

	SAMRPC_CLNT_CALL(samrpc_set_snapshot_locked, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with [%d] ...",	func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * clear_snapshot_locked
 * Make locked snapshot deletable
 */
int
clear_snapshot_locked(
	ctx_t *ctx,	/* client connection */
	char *snapname
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:clear snapshot locked";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	if (ISNULL(snapname)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = snapname;

	SAMRPC_CLNT_CALL(samrpc_clear_snapshot_locked, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with [%d] ...",	func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_indexed_snapshot_directories()
 * gets list of directories with snapshot indices.
 *
 */
int
get_indexed_snapshot_directories(
ctx_t *ctx,		/* client connection */
char *fsname,		/* filesystem */
sqm_lst_t **dirlist 	/* return - list of strings */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get_indexed_snapshot_directories";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, dirlist)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fsname;

	SAMRPC_CLNT_CALL(samrpc_get_indexed_snapshot_directories,
	    string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*dirlist = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*dirlist));

	PTRACE(2, "%s returning with status [%d] and [%d] dirs...",
	    func_name, ret_val, (*dirlist != NULL) ? (*dirlist)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_indexed_snapshots
 * Gets a list of indexed snapshots for a filesystem.  Sorted in
 * descending time order (newest first).
 *
 * If snapdir is NULL, returns information from all known directories.
 */
int
get_indexed_snapshots(
	ctx_t *ctx,	/* client connection */
	char *fsname,	/* input - fsname */
	char *snapdir,	/* input - directory to look for snaps.  Optional */
	sqm_lst_t **snaplist 	/* return - list of strings */
)
{
	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get_indexed_snapshots";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	if (ISNULL(fsname, snaplist)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = fsname;
	arg.str2 = snapdir;

	SAMRPC_CLNT_CALL(samrpc_get_indexed_snapshots, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*snaplist = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*snaplist));

	PTRACE(2, "%s returning with status [%d] and [%d] snapshots...",
	    func_name, ret_val, (*snaplist != NULL) ?
	    (*snaplist)->length : -1);

	PTRACE(2, "%s returning with [%d] ...",	func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
