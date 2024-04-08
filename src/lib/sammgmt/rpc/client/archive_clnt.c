
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

#pragma ident	"$Revision: 1.27 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * archive_clnt.c
 *
 * RPC client wrapper for archiver api
 */

/*
 * get_default_ar_set_copy_cfg
 *
 * get the default archive set copy configuration.
 */
int
get_default_ar_set_copy_cfg(
ctx_t *ctx,			/* client connection */
ar_set_copy_cfg_t **copy_cfg	/* return - default archive set copy config */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default archive set copy cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(copy_cfg)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_ar_set_copy_cfg, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*copy_cfg = (ar_set_copy_cfg_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_ar_set_criteria
 *
 * get the default archive set criteria.
 */
int
get_default_ar_set_criteria(
ctx_t *ctx,			/* client connection */
ar_set_criteria_t **criteria	/* return - archive set criteria */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default archive set criteria";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(criteria)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_ar_set_criteria, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*criteria = (ar_set_criteria_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_ar_global_directive
 *
 * get the global directive for archiver.
 */
int
get_ar_global_directive(
ctx_t *ctx,				/* client connection	*/
ar_global_directive_t **ar_global	/* return -		*/
					/* global directive	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get aachiver global directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_global)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_ar_global_directive, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_global = (ar_global_directive_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*ar_global) != NULL) {
		SET_LIST_TAIL((*ar_global)->ar_bufs);
		SET_LIST_TAIL((*ar_global)->ar_max);
		SET_LIST_TAIL((*ar_global)->ar_drives);
		SET_LIST_TAIL((*ar_global)->ar_set_lst);
		SET_LIST_TAIL((*ar_global)->ar_overflow_lst);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * set_ar_global_directive
 *
 * set the global directive for archiver.
 */
int
set_ar_global_directive(
ctx_t *ctx,				/* client connection	*/
ar_global_directive_t *ar_global	/* global directive	*/
)
{

	int ret_val;
	ar_global_directive_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set archiver global directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_global)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}


	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.ar_global = ar_global;

	PTRACE(3, "%s calling RPC...", func_name);

	SAMRPC_CLNT_CALL(samrpc_set_ar_global_directive,
	    ar_global_directive_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_ar_global_directive
 *
 * get the default global directive for archiver.
 */
int
get_default_ar_global_directive(
ctx_t *ctx,				/* client connection	*/
ar_global_directive_t **ar_global	/* return -		*/
					/* global directive	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default archiver global directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_global)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_ar_global_directive, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_global = (ar_global_directive_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*ar_global) != NULL) {
		SET_LIST_TAIL((*ar_global)->ar_bufs);
		SET_LIST_TAIL((*ar_global)->ar_max);
		SET_LIST_TAIL((*ar_global)->ar_drives);
		SET_LIST_TAIL((*ar_global)->ar_set_lst);
		SET_LIST_TAIL((*ar_global)->ar_overflow_lst);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_ar_set_criteria_names
 *
 * get of the names of all ar_set_criterias currently in the config
 * This will include no_archive if there are any no_archive criteria present
 */
int
get_ar_set_criteria_names(
ctx_t *ctx,				/* client connection	*/
sqm_lst_t **ar_set_crit_names		/* return - list of strings */
					/* ar_set_criteria names */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archiver set criteria names";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_crit_names)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_ar_set_criteria_names, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_set_crit_names = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ar_set_crit_names));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_ar_set
 *
 * get a specific archive set. The returned list will include criteria
 * from any file system that includes the set and any global criteria for
 * the set.
 */
int
get_ar_set(
ctx_t *ctx,			/* client connection	*/
const uname_t set_name,		/* archive set name	*/
sqm_lst_t **ar_set_criteria_list	/* return - list of	*/
				/* criteria		*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archive set";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(set_name, ar_set_criteria_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)set_name;

	SAMRPC_CLNT_CALL(samrpc_get_ar_set, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_set_criteria_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ar_set_criteria_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_ar_set_criteria_list
 *
 * get all the archive set criteria for a specific file system.
 * Returns only the criteria assigned directly to the specified file system
 * Global criteria are not returned here. To get any global criteria call
 * this method with the filesytem name GLOBAL_NAME.
 */
int
get_ar_set_criteria_list(
ctx_t *ctx,			/* client connection	*/
const uname_t fs_name,		/* file system name	*/
sqm_lst_t **ar_set_criteria_list	/* return - list of	*/
				/* ar_set_criteria	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archive set criteria list";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, ar_set_criteria_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fs_name;

	SAMRPC_CLNT_CALL(samrpc_get_ar_set_criteria_list, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_set_criteria_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ar_set_criteria_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_ar_set_criteria
 *
 * get all the archive set criteria.
 */
int
get_all_ar_set_criteria(
ctx_t *ctx,			/* client connection	*/
sqm_lst_t **ar_set_criteria_list	/* return - list of	*/
				/* archive set criteria	*/
)
{


	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all archive set criteria";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_criteria_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_ar_set_criteria, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_set_criteria_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ar_set_criteria_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_ar_fs_directive
 *
 * get a specific file systems directive for archiving.
 */
int
get_ar_fs_directive(
ctx_t *ctx,				/* client connection	*/
const uname_t fs_name,			/* file system name	*/
ar_fs_directive_t **ar_fs_directive)	/* return - directive	*/
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archive fs directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name, ar_fs_directive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fs_name;

	SAMRPC_CLNT_CALL(samrpc_get_ar_fs_directive, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_fs_directive = (ar_fs_directive_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*ar_fs_directive) != NULL) {
		SET_LIST_TAIL((*ar_fs_directive)->ar_set_criteria);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_all_ar_fs_directives
 *
 * get all file systems directives for archiving.
 */
int
get_all_ar_fs_directives(
ctx_t *ctx,				/* client connection	*/
sqm_lst_t **ar_fs_directive_list)		/* return - directive	*/
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all archive fs directives";
	char *err_msg;
	enum clnt_stat stat;

	/* fix the tail of the list */
	node_t *node;
	ar_fs_directive_t *fs_directive;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_fs_directive_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_ar_fs_directives, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_fs_directive_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ar_fs_directive_list));

	node = (*ar_fs_directive_list)->head;

	while (node != NULL) {
		fs_directive = (ar_fs_directive_t *)node->data;

		if (fs_directive != NULL) {
			SET_LIST_TAIL(fs_directive->ar_set_criteria);
		}
		node = node->next;
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * set_ar_fs_directive
 *
 * set file system directive for archiving.
 */
int
set_ar_fs_directive(
ctx_t *ctx,				/* client connection	*/
ar_fs_directive_t *ar_fs_directive	/* directive		*/
)
{

	int ret_val;
	ar_fs_directive_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set archive fs directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_fs_directive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}


	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fs_directive = (ar_fs_directive_t *)ar_fs_directive;

	SAMRPC_CLNT_CALL(samrpc_set_ar_fs_directive, ar_fs_directive_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * reset_ar_fs_directive
 *
 * reset file system directive for archiving
 */
int
reset_ar_fs_directive(
ctx_t *ctx,			/* client connection	*/
const uname_t fs_name		/* file system name	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:reset archive fs directive";
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

	SAMRPC_CLNT_CALL(samrpc_reset_ar_fs_directive, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * modify_ar_set_criteria
 *
 * modify an ar_set_criteria_t. The fs_name, set_name and key in
 * the ar_set_criteria are used to identify which criteria to modify
 */
int
modify_ar_set_criteria(
ctx_t *ctx,			/* client connection	*/
ar_set_criteria_t *crit		/* archive set criteria	*/
)
{

	int ret_val;
	ar_set_criteria_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:modify archive set criteria";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(crit)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.crit = crit;

	SAMRPC_CLNT_CALL(samrpc_modify_ar_set_criteria, ar_set_criteria_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}



/*
 * get_default_ar_fs_directive
 *
 * get the default file system directive for archiving.
 */
int
get_default_ar_fs_directive(
ctx_t *ctx,				/* client connection	*/
ar_fs_directive_t **ar_fs_directive	/* return -		*/
						/* archiving directive	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default archive fs directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_fs_directive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_ar_fs_directive, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_fs_directive = (ar_fs_directive_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*ar_fs_directive) != NULL) {
		SET_LIST_TAIL((*ar_fs_directive)->ar_set_criteria);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_ar_set_copy_params
 *
 * get all copy parameters.
 */
int
get_all_ar_set_copy_params(
ctx_t *ctx,				/* client connection	*/
sqm_lst_t **ar_set_copy_params_list	/* return - list of	*/
						/* copy parameters	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all archive set copy params";
	char *err_msg;
	enum clnt_stat stat;

	/* fix tail of the list */
	node_t *node;
	ar_set_copy_params_t *copy_params;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_copy_params_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	SAMRPC_CLNT_CALL(samrpc_get_all_ar_set_copy_params, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	*ar_set_copy_params_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ar_set_copy_params_list));
	node = (*ar_set_copy_params_list)->head;

	while (node != NULL) {
		copy_params = (ar_set_copy_params_t *)node->data;

		if (copy_params != NULL) {
			SET_LIST_TAIL(copy_params->priority_lst);
		}
		node = node->next;
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_ar_set_copy_params_names
 *
 * get copy parameters names.
 */
int
get_ar_set_copy_params_names(
ctx_t *ctx,			/* client connection	*/
sqm_lst_t **ar_set_copy_names	/* return - list of	*/
					/* copy parameter names	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archive set copy params names";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_copy_names)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_ar_set_copy_params_names, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_set_copy_names = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ar_set_copy_names));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_ar_set_copy_params
 *
 * get all copy parameters for a specific archive set.
 */
int
get_ar_set_copy_params(
ctx_t *ctx,					/* client connection */
const uname_t ar_set_copy_name,			/* archive set name */
ar_set_copy_params_t **ar_set_copy_parameters	/* return - copy parameters */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archive set copy params";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_copy_name, ar_set_copy_parameters)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)ar_set_copy_name;

	SAMRPC_CLNT_CALL(samrpc_get_ar_set_copy_params, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_set_copy_parameters = (ar_set_copy_params_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*ar_set_copy_parameters) != NULL) {
		SET_LIST_TAIL((*ar_set_copy_parameters)->priority_lst);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_ar_set_copy_params_for_ar_set
 *
 * get the copy parameters for all copies defined for this archive set
 */
int
get_ar_set_copy_params_for_ar_set(
ctx_t *ctx,				/* client connection */
const char *ar_set_name,		/* archive set name */
sqm_lst_t **ar_set_copy_params_list	/* return - list of copy parameters */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archive set copy params for ar set";
	char *err_msg;
	enum clnt_stat stat;

	/* fix the tail */
	node_t *node;
	ar_set_copy_params_t *copy_params;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_name, ar_set_copy_params_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)ar_set_name;

	SAMRPC_CLNT_CALL(samrpc_get_ar_set_copy_params_for_ar_set,
	    string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*ar_set_copy_params_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*ar_set_copy_params_list));

	node = (*ar_set_copy_params_list)->head;
	while (node != NULL) {
		copy_params = (ar_set_copy_params_t *)node->data;

		if (copy_params != NULL) {
			SET_LIST_TAIL(copy_params->priority_lst);
		}
		node = node->next;
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * set_ar_set_copy_params
 *
 * set copy parameters.
 */
int
set_ar_set_copy_params(
ctx_t *ctx,					/* client connection	*/
ar_set_copy_params_t *ar_set_copy_parameters	/* return - copy parameters */
)
{

	int ret_val;
	ar_set_copy_params_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set archive set copy params";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_copy_parameters)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.copy_params = (ar_set_copy_params_t *)ar_set_copy_parameters;

	SAMRPC_CLNT_CALL(samrpc_set_ar_set_copy_params,
	    ar_set_copy_params_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * reset_ar_set_copy_params
 *
 * reset copy parameters
 */
int
reset_ar_set_copy_params(
ctx_t *ctx,			/* client connection		*/
const uname_t ar_set_copy_name	/* archive set copy name	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:reset archive set copy params";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_copy_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)ar_set_copy_name;

	SAMRPC_CLNT_CALL(samrpc_reset_ar_set_copy_params, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_ar_set_copy_params
 *
 * get the default copy parameters.
 */
int
get_default_ar_set_copy_params(
ctx_t *ctx,				/* client connection	*/
ar_set_copy_params_t **parameters	/* return - copy params */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default archive set copy params";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(parameters)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_default_ar_set_copy_params, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*parameters = (ar_set_copy_params_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*parameters) != NULL) {
		SET_LIST_TAIL((*parameters)->priority_lst);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_vsn_pools
 *
 * get all the vsn pools.
 */
int
get_all_vsn_pools(
ctx_t *ctx,			/* client connection		*/
sqm_lst_t **vsn_pool_list		/* return - list of vsn pools	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all vsn pools";
	char *err_msg;
	enum clnt_stat stat;

	/* to fix the tail of the list */
	node_t *node;
	vsn_pool_t *vsn_pool;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn_pool_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	SAMRPC_CLNT_CALL(samrpc_get_all_vsn_pools, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vsn_pool_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*vsn_pool_list));
	node = (*vsn_pool_list)->head;
	while (node != NULL) {
		vsn_pool = (vsn_pool_t *)node->data;
		if (vsn_pool != NULL) {
			SET_LIST_TAIL(vsn_pool->vsn_names);
		}
		node = node->next;
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_vsn_pool
 *
 * get a specific vsn pool.
 */
int
get_vsn_pool(
ctx_t *ctx,			/* client connection	*/
const uname_t pool_name,	/* vsn pool name	*/
vsn_pool_t **vsn_pool		/* return - vsn pool	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get vsn pool";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(pool_name, vsn_pool)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)pool_name;

	SAMRPC_CLNT_CALL(samrpc_get_vsn_pool, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vsn_pool = (vsn_pool_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*vsn_pool) != NULL) {
		SET_LIST_TAIL((*vsn_pool)->vsn_names);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * add_vsn_pool
 *
 * add a vsn pool.
 */
int
add_vsn_pool(
ctx_t *ctx,			/* client connection	*/
const vsn_pool_t *vsn_pool	/* vsn pool		*/
)
{

	int ret_val;
	vsn_pool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add vsn pool";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn_pool)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.pool = (vsn_pool_t *)vsn_pool;

	SAMRPC_CLNT_CALL(samrpc_add_vsn_pool, vsn_pool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * modify_vsn_pool
 *
 * modify vsn pool
 */
int
modify_vsn_pool(
ctx_t *ctx,			/* client connection	*/
const vsn_pool_t *vsn_pool	/* vsn pool		*/
)
{

	int ret_val;
	vsn_pool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:modify vsn pool";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn_pool)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.pool = (vsn_pool_t *)vsn_pool;

	SAMRPC_CLNT_CALL(samrpc_modify_vsn_pool, vsn_pool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * remove_vsn_pool
 *
 * remove vsn pool
 */
int
remove_vsn_pool(
ctx_t *ctx,			/* client connection	*/
const uname_t pool_name		/* vsn pool name	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove vsn pool";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(pool_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)pool_name;

	SAMRPC_CLNT_CALL(samrpc_remove_vsn_pool, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_vsn_copy_maps
 *
 * get all vsn and archive set copy associations.
 */
int
get_all_vsn_copy_maps(
ctx_t *ctx,			/* client connection	*/
sqm_lst_t **vsn_map_list		/* return - list of vsn */
				/* and archive set copy association */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all vsn copy maps";
	char *err_msg;
	enum clnt_stat stat;

	/* to fix the tail of the list */
	node_t *node;
	vsn_map_t *vsn_map;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn_map_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	SAMRPC_CLNT_CALL(samrpc_get_all_vsn_copy_maps, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vsn_map_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*vsn_map_list));

	node = (*vsn_map_list)->head;
	while (node != NULL) {
		vsn_map = (vsn_map_t *)node->data;
		if (vsn_map != NULL) {
			SET_LIST_TAIL(vsn_map->vsn_names);
			SET_LIST_TAIL(vsn_map->vsn_pool_names);
		}
		node = node->next;
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_vsn_copy_map
 *
 * get a specific vsn and archive set copy association given the copy name.
 */
int
get_vsn_copy_map(
ctx_t *ctx,			/* client connection	*/
const uname_t ar_set_copy_name,	/* copy name		*/
vsn_map_t **vsn_map		/* return - vsn and	*/
				/* archive set copy association */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get vsn copy map";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_copy_name, vsn_map)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)ar_set_copy_name;

	SAMRPC_CLNT_CALL(samrpc_get_vsn_copy_map, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vsn_map = (vsn_map_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	if ((*vsn_map) != NULL) {
		SET_LIST_TAIL((*vsn_map)->vsn_names);
		SET_LIST_TAIL((*vsn_map)->vsn_pool_names);
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * add_vsn_copy_map
 *
 * add a vsn and archive set copy association.
 */
int
add_vsn_copy_map(
ctx_t *ctx,			/* client connection	*/
const vsn_map_t *vsn_map	/* vsn and archive set	*/
				/* copy association	*/
)
{

	int ret_val;
	vsn_map_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add vsn copy map";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn_map)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.map = (vsn_map_t *)vsn_map;

	SAMRPC_CLNT_CALL(samrpc_add_vsn_copy_map, vsn_map_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * modify_vsn_copy_map
 *
 * modify vsn and archive set copy association
 */
int
modify_vsn_copy_map(
ctx_t *ctx,			/* client connection	*/
const vsn_map_t *vsn_map	/* vsn and archive set copy association	*/
)
{

	int ret_val;
	vsn_map_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:modify vsn copy map";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn_map)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.map = (vsn_map_t *)vsn_map;

	SAMRPC_CLNT_CALL(samrpc_modify_vsn_copy_map, vsn_map_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * remove_vsn_copy_map
 *
 * remove vsn and archive set copy association
 */
int
remove_vsn_copy_map(
ctx_t *ctx,			/* client connection	*/
const uname_t ar_set_copy_name	/* copy name		*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove vsn copy map";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ar_set_copy_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)ar_set_copy_name;

	PTRACE(3, " %s calling RPC...", func_name);

	SAMRPC_CLNT_CALL(samrpc_remove_vsn_copy_map, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_default_ar_drive_directive
 *
 * get the default drive directive for archiving for a library.
 */
int
get_default_ar_drive_directive(
ctx_t *ctx,			/* client connection		*/
uname_t lib_name,		/* library name			*/
boolean_t global,		/* is it global directive	*/
drive_directive_t **drive	/* return - drive directive	*/
)
{

	int ret_val;
	string_bool_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get default archive drive directive";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(lib_name, drive)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)lib_name;
	arg.bool = global;

	SAMRPC_CLNT_CALL(samrpc_get_default_ar_drive_directive,
	    string_bool_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*drive = (drive_directive_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * ar_run
 *
 * Causes the archiver to run for the named file system. This function
 * overrides any wait commands that have previously been applied to the
 * filesystem through the archiver.cmd file
 */
int
ar_run(
ctx_t *ctx,			/* client connection	*/
uname_t fs_name			/* file system name	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:archiver run";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fs_name;

	SAMRPC_CLNT_CALL(samrpc_ar_run, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * ar_run_all
 *
 * start the archiver on all filesystems overriding the 'wait' commands that
 * have been previously applied through the archiver.cmd file
 */
int
ar_run_all(
ctx_t *ctx	/* client connection	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:run archiver for all fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_ar_run_all, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * ar_stop_all
 *
 * Immediately stop archiving for all fs
 */
int
ar_stop_all(
ctx_t *ctx	/* client connection	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:ar stop all";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_ar_stop_all, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * ar_stop
 *
 * Immediately stop archiving for fs. For a more graceful stop see ar_idle
 */
int
ar_stop(
ctx_t *ctx,			/* client connection	*/
uname_t fs_name			/* file system name	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:ar stop";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fs_name;

	SAMRPC_CLNT_CALL(samrpc_ar_stop, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * ar_idle
 *
 * idle archiving for fs. This function stops archiving for the named file
 * system at the next convienent point. e.g. at the end of the current tar file
 */
int
ar_idle(
ctx_t *ctx,			/* client connection	*/
uname_t fs_name			/* file system name	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:ar idle";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fs_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fs_name;

	SAMRPC_CLNT_CALL(samrpc_ar_idle, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * ar_idle_all
 *
 * idle all archiving. This function stops all archiving at the next convienent
 * point. e.g. at the end of the current tar files
 */
int
ar_idle_all(
ctx_t *ctx	/* client connection */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:ar idle all";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_ar_idle_all, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * ar_restart_all
 *
 * Interupt the archiver and restart it for all filesystems.
 * This occurs immediately without regard to the state of the archiver.
 * It may terminate media copy operations and therefore waste space on media.
 * This should be done with caution.
 *
 * The capability to restart archiving for a single file system does not exist
 * in samfs.
 */
int
ar_restart_all(
ctx_t *ctx	/* client connection */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:ar restart all";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_ar_restart_all, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_archiverd_state
 *
 * get the status of the archiverd daemon
 */
int
get_archiverd_state(
ctx_t *ctx,			/* client connection		*/
struct ArchiverdState **status	/* return - status of archiverd	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archiverd state";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(status)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_archiverd_state, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*status = (struct ArchiverdState *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_all_arfind_state
 *
 * get the status of the arfind processes for each archiving file system.
 */
int
get_all_arfind_state(
ctx_t *ctx,			/* client connection	*/
sqm_lst_t **arfind_status		/* return - list of	*/
					/* status of all arfind processes */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all arfind state";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(arfind_status)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_arfind_state, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*arfind_status = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_arfind_state
 *
 * get the status of the arfind process for a given file system
 * GUI note: This contains the status for the archive management page
 */
int
get_arfind_state(
ctx_t *ctx,			/* client connection	*/
uname_t fsname,			/* file system name	*/
ar_find_state_t **status	/* return -		*/
				/* status of arfind process */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get arfind state";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, status)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;

	SAMRPC_CLNT_CALL(samrpc_get_arfind_state, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*status = (ar_find_state_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_archreqs
 *
 * get a list of arch reqs for the named file system.
 * arch reqs contain information about each of the copy processes for an
 * archive set within a file system.
 */
int
get_archreqs(
ctx_t *ctx,			/* client connection	*/
uname_t fsname,			/* file system name	*/
sqm_lst_t **archreqs		/* return - list of	*/
				/* archive request	*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get archreqs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, archreqs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)fsname;

	SAMRPC_CLNT_CALL(samrpc_get_archreqs, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*archreqs = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_archreqs
 *
 * get a list of all arch reqs
 * arch reqs contain information about each of the copy processes for an
 * archive set within a file system.
 */
int
get_all_archreqs(
ctx_t *ctx,			/* client connection	*/
sqm_lst_t **archreqs		/* return - list of	*/
					/* archiver requests	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all archreqs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(archreqs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_archreqs, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*archreqs = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * activate_archiver_cfg
 *
 * must be called after changes are made to the archiver configuration. It
 * first checks the configuration to make sure it is valid. If the
 * configuration is valid it is made active by signaling sam-fsd.
 *
 * returns:
 * 0  indicates successful execution, no errors or warnings encountered.
 * -1 indicates an internal error.
 * -2 indicates errors encountered in config
 * -3 indicates warnings encountered in config
 */
int
activate_archiver_cfg(
ctx_t *ctx,			/* client connection */
sqm_lst_t **err_warn_list	/* return - list of errs/warnings */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:activate archiver cfg";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(err_warn_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_activate_archiver_cfg, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*err_warn_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * sets in_use to true if the named pool is in use. If the pool is in use
 * the function also populates the uname used_by with a string containing
 * the name of an archive copy that uses the pool. There could be many that
 * use it but only one gets returned.
 *
 * in_use and used_by are return parameters.
 */
int is_pool_in_use(
ctx_t *ctx,
const uname_t pool_name,	/* pool name */
boolean_t *in_use,		/* return - true if pool in use, else false */
uname_t used_by			/* return - archive set name using the pool */
)
{

	int ret_val;
	string_arg_t arg;
	pool_use_result_t *res;
	samrpc_result_t result;
	char *func_name = "rpc:is pool in use?";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(pool_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)pool_name;

	SAMRPC_CLNT_CALL(samrpc_is_pool_in_use, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	res = (pool_use_result_t *)result.samrpc_result_u.result.result_data;

	*in_use = res->in_use;
	strncpy(used_by, res->used_by, sizeof (uname_t));

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * if the named group is a valid unix group, is_valid will be set to true.
 * if any errors are encountered -1 is returned.
 */
int is_valid_group(
ctx_t *ctx,
uname_t group,	/* group name */
boolean_t *is_valid	/* return - true if group is valid, else false */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:is group valid?";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(group)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)group;

	SAMRPC_CLNT_CALL(samrpc_is_valid_group, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*is_valid = *((boolean_t *)result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * if the named user is a valid unix user, is_valid will be set to true.
 * if any errors are encountered -1 is returned.
 */
int is_valid_user(
ctx_t *ctx,
uname_t user,	/* user name */
boolean_t *is_valid	/* return - true if user is valid, else false */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:is user valid?";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(user)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)user;

	SAMRPC_CLNT_CALL(samrpc_is_valid_user, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*is_valid = *((boolean_t *)result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * ar_rerun_all
 *
 * Restart the archiver on all filesystems overriding the 'wait' commands that
 * have been previously applied through the archiver.cmd file
 */
int
ar_rerun_all(
ctx_t *ctx	/* client connection	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:rerun archiver for all fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_ar_rerun_all, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
archive_files(ctx_t *ctx, sqm_lst_t *files, int32_t options, char **job_id) {

	int ret_val;
	strlst_int32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:archive files";
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
	arg.int32 = options;

	SAMRPC_CLNT_CALL(samrpc_archive_files, strlst_int32_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*job_id = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	return (ret_val);
}
/*
 * get utilization of archive copies sorted by usage
 * This can be used to get copies with low free space
 *
 * Input:
 *      int count       - n copies with top usage
 *
 * Returns a list of formatted strings
 *      name=copy name
 *      type=mediatype
 *      free=freespace in kbytes
 *	capacity=in kybtes
 *	usage=%
 */
int
get_copy_utilization(
ctx_t *ctx,		/* ARGSUSED */
int count,		/* input - n copies with top usage */
sqm_lst_t **strlst)	/* return - list of formatted strings */
{

	int ret_val;
	int_int_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get copy utilization";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(strlst)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.int1 = count;
	arg.int2 = 0; /* NOT USED */

	SAMRPC_CLNT_CALL(samrpc_get_copy_utilization, int_int_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*strlst = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
