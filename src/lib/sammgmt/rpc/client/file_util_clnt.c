
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

#pragma ident	"$Id"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * file_util_clnt.c
 *
 * RPC client wrapper for file utilities api
 */

/*
 * list_dir
 * gets list of files contained in that directory
 */
int
list_dir(
	ctx_t 	*ctx,		/* client connection */
	int 	maxentries,	/* input - max entries */
	char 	*filepath,	/* input - dir */
	char 	*restrictions,	/* input - contraints */
	sqm_lst_t **direntries	/* return - list of strings (file names) */
)
{
	int ret_val;
	file_restrictions_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:list dir";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(direntries)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.dumppath = NULL;
	arg.fsname = NULL;
	arg.maxentries = maxentries;
	arg.filepath = filepath;
	arg.restrictions = restrictions;

	SAMRPC_CLNT_CALL(samrpc_list_dir, file_restrictions_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*direntries = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*direntries));

	PTRACE(2, "%s returning with status [%d] with [%d] entries...",
	    func_name, ret_val,
	    (*direntries != NULL) ? (*direntries)->length: -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_file_status
 * gets the details tails about an array of filepaths;
 * their type (file vs directory) and whether they are currently on disk
 * or must be staged before being used.
 */
int
get_file_status(
	ctx_t 	*ctx,		/* client connection */
	sqm_lst_t 	*filepaths,	/* input - list of strings */
	sqm_lst_t 	**filestatus	/* return - list of integers */
)
{
	int ret_val;
	strlst_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get file status";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(filestatus)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.lst = filepaths;

	SAMRPC_CLNT_CALL(samrpc_get_file_status, strlst_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*filestatus = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_file_details
 * gets details about a file
 *
 */
int
get_file_details(
	ctx_t 	*ctx,		/* client connection */
	char 	*fsname,	/* filesystem name */
	sqm_lst_t 	*files,		/* list of files to get details on */
	sqm_lst_t 	**status	/* return - list of strings */
)
{
	int ret_val;
	string_strlst_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get file details";
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
	arg.strlst = files;

	SAMRPC_CLNT_CALL(samrpc_get_file_details, string_strlst_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*status = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

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
 * Create a file if it does not already exist. This function will also
 * create any missing directories. If the file already exists
 * this function will return success.
 *
 * PARAMS:
 *   ctx_t *    IN   - context object
 *   upath_t    IN   - fully qualified file name
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
create_file(
	ctx_t 	*ctx,				/* client connection */
	char	*full_path			/* full path name */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:create file";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(full_path)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = full_path;

	SAMRPC_CLNT_CALL(samrpc_create_file, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * check file exists
 */
int
file_exists(
	ctx_t 	*ctx,				/* client connection */
	upath_t	filepath			/* filepath name */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:file_exists";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(filepath)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)filepath;

	SAMRPC_CLNT_CALL(samrpc_file_exists, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Function to get lines from the tail of a file
 * PARAMS
 *   ctx_t *    IN   - context object
 *   char *	IN   - name of file to tail
 *   howmany	IN   - maximum number of lines to return
 *   char ***	OUT  - malloced array of ptrs to lines in file
 *   char **	OUT  - NULL, not used over rpc
 *
 * format:
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
tail(
	ctx_t 		*ctx,		/* client connection */
	char 		*file,
	uint32_t	*howmany,
	char 		***res,
	char 		**data 		/* ARGSUSED */
)
{
	int ret_val;
	string_uint32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get tail";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(res)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = file;
	arg.u_flag = *howmany;

	SAMRPC_CLNT_CALL(samrpc_tail, string_uint32_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*res = (char **)((int_ptrarr_result_t *)
	    result.samrpc_result_u.result.result_data)->pstr;
	*howmany = (int)((int_ptrarr_result_t *)
	    result.samrpc_result_u.result.result_data)->count;
	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Create directory
 */
int
create_dir(
	ctx_t 	*ctx,				/* client connection */
	upath_t	dir				/* directory name */
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:create dir";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(dir)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)dir;

	SAMRPC_CLNT_CALL(samrpc_create_dir, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
get_txt_file(
ctx_t *ctx,	/* client connection */
char *file,
uint32_t start_at,
uint32_t *howmany,
char ***res,
char **data /* ARGSUSED */
)
{
	int ret_val;
	string_uint32_uint32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get txt file";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(res)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = file;
	arg.u_1 = start_at;
	arg.u_2 = *howmany;

	SAMRPC_CLNT_CALL(samrpc_get_txt_file, string_uint32_uint32_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*res = (char **)((int_ptrarr_result_t *)
	    result.samrpc_result_u.result.result_data)->pstr;
	*howmany = (int)((int_ptrarr_result_t *)
	    result.samrpc_result_u.result.result_data)->count;
	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_file_details
 * gets details about a file
 *
 */
int
get_extended_file_details(
ctx_t 	*ctx,		/* client connection */
sqm_lst_t 	*files,		/* list of files to get details on */
uint32_t which_details,	/* flags indicating which details to return */
sqm_lst_t 	**file_details	/* return - list of strings */
)
{
	int ret_val;
	strlst_uint32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get extended file details";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(files, file_details)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.strlst = files;
	arg.u32 = which_details;

	SAMRPC_CLNT_CALL(samrpc_get_extended_file_details, strlst_uint32_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*file_details = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*file_details));

	PTRACE(2, "%s returning with [%d] and [%d]details strings...",
	    func_name, ret_val,
	    (*file_details != NULL) ? (*file_details)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_file_details
 * gets details about a file
 *
 */
int
get_copy_details(
ctx_t 	*ctx,		/* client connection */
char 	*file_path,	/* full path of file to get details on */
uint32_t which_details,	/* flags indicating which details to return */
sqm_lst_t	**file_details	/* return - list of strings */
)
{
	int ret_val;
	string_uint32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get copy details";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(file_path, file_details)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = file_path;
	arg.u_flag = which_details;

	SAMRPC_CLNT_CALL(samrpc_get_copy_details, string_uint32_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*file_details = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*file_details));

	PTRACE(2, "%s returning with [%d] and [%d]details strings...",
	    func_name, ret_val,
	    (*file_details != NULL) ? (*file_details)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * list_directory
 * gets list of files contained in that directory
 */
int
list_directory(
	ctx_t 		*ctx,		/* client connection */
	int 		maxentries,	/* input - max entries */
	char		*listDir,	/* input - dir */
	char		*startFile,	/* input - file */
	char 		*restrictions,	/* input - contraints */
	uint32_t	*morefiles,	/* output - if more than requested */
	sqm_lst_t 		**direntries	/* return - list of strings */
)
{
	int ret_val;
	file_restrictions_more_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:list directory";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(listDir, direntries)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.dumppath = NULL;
	arg.fsname = NULL;
	arg.maxentries = maxentries;
	arg.dir = listDir;
	arg.file = startFile;
	arg.restrictions = restrictions;
	arg.morefiles = morefiles;

	SAMRPC_CLNT_CALL(samrpc_list_directory, file_restrictions_more_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*direntries = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*direntries));

	PTRACE(2, "%s returning with status [%d] with [%d] entries...",
	    func_name, ret_val,
	    (*direntries != NULL) ? (*direntries)->length: -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * list_and_collect_file_details
 * lists a directory and gets details about files
 *
 */
int
list_and_collect_file_details(
	ctx_t		*ctx,
	char		*fsname,	/* filesystem name */
	char		*snappath,	/* could be NULL if ! Restore */
	char		*startDir,	/* starting point. NULL = rootdir */
	char		*startFile,	/* if continuing, start from here */
	int32_t		howmany,	/* how many to return.  -1 for all */
	uint32_t	which_details,	/* file properties to return */
	char		*restrictions,	/* filtering options */
	uint32_t	*morefiles,	/* does the directory have more? */
	sqm_lst_t		**results	/* list of details strings */
)
{
	int ret_val;
	file_details_arg_t arg;
	file_details_result_t *res;
	samrpc_result_t result;
	char *func_name = "rpc:list and collect file details";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(results)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	*morefiles = 0;
	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fsname = fsname;
	arg.snap = snappath;
	arg.dir = startDir;
	arg.file = startFile;
	arg.howmany = howmany;
	arg.u32 = which_details;
	arg.restrictions = restrictions;
	arg.files = NULL;

	SAMRPC_CLNT_CALL(samrpc_list_and_collect_file_details,
	    file_details_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	res = (file_details_result_t *)
	    result.samrpc_result_u.result.result_data;
	*results = res->list;
	*morefiles = res->more;

	free(res);

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*results));

	PTRACE(2, "%s returning [%d] more = %u and [%d] details strings...",
	    func_name, ret_val, *morefiles,
	    (*results != NULL) ? (*results)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * collect_file_details
 * Returns details for the provided list of files
 *
 */
int
collect_file_details(
	ctx_t		*ctx,
	char		*fsname,	/* filesystem name */
	char		*snappath,	/* could be NULL if ! Restore */
	char		*usedir,	/* diretory containing the files */
	sqm_lst_t		*files,		/* list of files to process */
	uint32_t	which_details,	/* file properties to return */
	sqm_lst_t		**results	/* list of details strings */
)
{
	int ret_val;
	file_details_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:collect file details";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(results)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fsname = fsname;
	arg.snap = snappath;
	arg.dir = usedir;
	arg.file = NULL;
	arg.u32 = which_details;
	arg.howmany = 0;
	arg.restrictions = NULL;
	arg.files = files;

	SAMRPC_CLNT_CALL(samrpc_collect_file_details, file_details_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*results = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*results));

	PTRACE(2, "%s returning with [%d] and [%d]details strings...",
	    func_name, ret_val, (*results != NULL) ? (*results)->length : -1);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
delete_files(
	ctx_t 	*ctx,			/* client connection */
	sqm_lst_t *lst			/* list of absolute paths */
)
{
	int ret_val;
	strlst_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:delete files";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lst)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.lst = lst;

	SAMRPC_CLNT_CALL(samrpc_delete_files, strlst_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
