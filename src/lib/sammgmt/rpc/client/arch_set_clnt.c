
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

#pragma ident	"$Revision: 1.16 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * arch_set_clnt.c
 *
 * RPC client wrapper for archiver api
 */

/*
 * Get a list of all arch_set_t structures, describing all archive sets.
 * This includes:
 * All defined archive sets (policies and legacy policies),
 * A single no_archive set that may contain many criteria,
 * Default sets for each archiving filesystem.
 *
 * There is no order to the returned list.
 */
int
get_all_arch_sets(
ctx_t *ctx,			/* client connection	*/
sqm_lst_t **arch_set_list	/* return - list of	*/
				/* ar_set_criteria	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all archive sets";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(arch_set_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_arch_sets, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*arch_set_list = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*arch_set_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get the named arch_set structure
 */
int
get_arch_set(
ctx_t *ctx,		/* can be used to read from alternate location */
set_name_t name,	/* name of the set to return */
arch_set_t **set)	/* Malloced return value */
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get arch set";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(name, set)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}
	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)name;

	PTRACE(2, "arg.str = %s", arg.str);

	SAMRPC_CLNT_CALL(samrpc_get_arch_set, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*set = (arch_set_t *)
	    result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Function to create an archive set. The criteria will be added to the
 * end of any file system for which they are specified. Can not be used to
 * create a default arch set. This function will fail if the set already
 * exists.
 */
int
create_arch_set(
ctx_t *ctx,		/* contains optional dump path */
arch_set_t *set)	/* archive set to create */
{
	int ret_val;
	arch_set_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:create archive set";
	char *err_msg;
	enum clnt_stat stat;
	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(set)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}
	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.set = set;

	SAMRPC_CLNT_CALL(samrpc_create_arch_set, arch_set_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);

	return (ret_val);
}

/*
 * This function would modify each of the criteria in the archive set, and the
 * copy params and vsn_map for this function. It is also important to note that
 * Global criteria may be contained in the set, in which case this function
 * can affect all file systems.
 */
int
modify_arch_set(
ctx_t *ctx,		/* contains optional dump path */
arch_set_t *set)	/* archive set to modify */
{

	int ret_val;
	arch_set_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:modify archive set";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(set)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, " %s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.set = (arch_set_t *)set;

	SAMRPC_CLNT_CALL(samrpc_modify_arch_set, arch_set_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * Delete will delete the entire set including copies, criteria,
 * params and vsn associations.
 *
 * If the named set is the allsets pseudo set this function will
 * reset defaults because the allsets set can not really be deleted.
 *
 * If the named set is a default set this function will return the
 * error: SE_CANNOT_DELETE_DEFAULT_SET. It is not possible to delete
 * the default set because it will exist as long as the file system of
 * the same name exists. Users wishing to remove all archiving related
 * traces of an archiving file system from the archiver.cmd should
 * instead use reset_ar_fs_directive.
 */
int
delete_arch_set(
ctx_t *ctx,		/* contains optional dump path */
set_name_t name)	/* the name of the set to delete */
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:delete archive set";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)name;

	PTRACE(3, " %s calling RPC...", func_name);

	SAMRPC_CLNT_CALL(samrpc_delete_arch_set, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
