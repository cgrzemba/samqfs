
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

#pragma ident	"$Revision: 1.17 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * release_clnt.c
 *
 * RPC wrapper for releaser api
 */

/*
 * get_all_rl_fs_directives
 * Function to get all the directives for controlling the releaser.
 *
 * The rl_fs_directive list is populated with global and filesystem specific
 * directives. The directives are read from SAM_CONFIG_PATH/releaser.cmd file
 * If the releaser.cmd does not contain an entry for the directive, then the
 * sqm_lst_t is populated with the default directive value.
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and rl_fs_directive_t is NULL.
 */
int
get_all_rl_fs_directives(
ctx_t *ctx,			/* client connection */
sqm_lst_t **rl_fs_directive_list /* return - list of releaser directives */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get_all_rl_fs_directives()";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rl_fs_directive_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_rl_fs_directives, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*rl_fs_directive_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*rl_fs_directive_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_rl_fs_directive
 * Function to get the release directives for a file system. To get the
 * global directives, use fs_name as 'global'
 *
 * The directives are read from SAM_CONFIG_PATH/releaser.cmd file. If the
 * releaser.cmd file does not contain an entry for the directive, then the
 * list is populated with default directive value
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and rl_fs_directive_t is NULL.
 */
int
get_rl_fs_directive(
ctx_t *ctx,				/* client connection */
const uname_t fs_name,			/* filesystem name */
rl_fs_directive_t **rl_fs_directive	/* return releaser directive */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get_rl_fs_directive()";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rl_fs_directive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fs_name;

	SAMRPC_CLNT_CALL(samrpc_get_rl_fs_directive, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*rl_fs_directive = (rl_fs_directive_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_rl_fs_directive
 * get the default release directives.
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_default_rl_fs_directive(
ctx_t *ctx,				/* client connection */
rl_fs_directive_t **rl_fs_directive	/* return - default directive */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get_default_rl_fs_directive()";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rl_fs_directive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_rl_fs_directive, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*rl_fs_directive = (rl_fs_directive_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * set_rl_fs_directive
 * functions to set releaser directives for a particular filesystem
 *
 * The filesystem name is given as an element of rl_fs_directive_t
 * If the directives for the specified filesystem exist and the field bit in
 * change_flag is set correctly, then only the directive is updated.
 * It is an error if the filesystem name does not exist in the releaser.cmd
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
set_rl_fs_directive(
ctx_t *ctx,				/* client connection	*/
rl_fs_directive_t *rl_fs_directive	/* fs directives	*/
)
{

	int ret_val;
	rl_fs_directive_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set_rl_fs_directive()";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rl_fs_directive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.rl_fs_directive = rl_fs_directive;

	SAMRPC_CLNT_CALL(samrpc_set_rl_fs_directive, rl_fs_directive_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * reset_rl_fs_directive
 * Function to reset releaser directives for a particular filesystem
 *
 * The filesystem name is given as an element of rl_fs_directive_t
 * If the directives for the specified filesystem exists, it is removed from
 * the  releaser.cmd file. It is an error if the filesystem name does not exist
 * in the releaser.cmd
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
reset_rl_fs_directive(
ctx_t *ctx,				/* client connection	*/
rl_fs_directive_t *rl_fs_directive	/* fs directives	*/
)
{

	int ret_val;
	rl_fs_directive_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:reset_rl_fs_directive()";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(rl_fs_directive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.rl_fs_directive = rl_fs_directive;

	SAMRPC_CLNT_CALL(samrpc_reset_rl_fs_directive, rl_fs_directive_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_releasing_fs_list
 * Function to get a list of release_fs_t for filesystems that are
 * doing release job
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and releasing_fs_list is NULL.
 */
int
get_releasing_fs_list(
ctx_t *ctx,			/* client connection */
sqm_lst_t **releasing_fs_list	/* return - list of releasing fs */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get_releasing_fs_list()";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(releasing_fs_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_releasing_fs_list, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*releasing_fs_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*releasing_fs_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

int
release_files(ctx_t *ctx, sqm_lst_t *files, int32_t options,
    int32_t partial_sz, char **job_id) {

	int ret_val;
	strlst_int32_int32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:release files";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(files, job_id)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.strlst = files;
	arg.int1 = options;
	arg.int2 = partial_sz;

	SAMRPC_CLNT_CALL(samrpc_release_files, strlst_int32_int32_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*job_id = (char *)result.samrpc_result_u.result.result_data;


	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	return (ret_val);
}
