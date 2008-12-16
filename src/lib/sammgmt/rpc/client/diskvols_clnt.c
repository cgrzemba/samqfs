
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

#pragma ident	"$Revision: 1.18 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"
/*
 * diskvols_clnt.c
 *
 * RPC client wrapper for diskvols api
 */

/*
 * get_disk_vol
 * Function to get one disk_vol_t based on vsn name
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and disk_vol is NULL.
 */
int
get_disk_vol(
ctx_t *ctx,		/* client connection		*/
const char *vol_name,	/* volume name			*/
disk_vol_t **disk_vol	/* return - disk_vol		*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get disk vol";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vol_name, disk_vol)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)vol_name;

	SAMRPC_CLNT_CALL(samrpc_get_disk_vol, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*disk_vol = (disk_vol_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_disk_vols
 * Function to get a list of all configured diskvols
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_all_disk_vols(
ctx_t *ctx,		/* client connection		*/
sqm_lst_t **vol_list	/* return - list of diskvol	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all disk vols";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vol_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_disk_vols, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*vol_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*vol_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_clients
 * Function to get a list of all trusted clients
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
get_all_clients(
ctx_t *ctx,		/* client connection		*/
sqm_lst_t **clients	/* return - list of clients	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all clients";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(clients)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_clients, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*clients = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*clients));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * add_disk_vol
 * function to add a diskvol
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
add_disk_vol(
ctx_t *ctx,			/* client connection	*/
disk_vol_t *disk_vol		/* disk vol entry	*/
)
{

	int ret_val;
	disk_vol_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add disk vol";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(disk_vol)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.disk_vol = disk_vol;

	SAMRPC_CLNT_CALL(samrpc_add_disk_vol, disk_vol_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * remove_disk_vol
 * function to remove a diskvol
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
remove_disk_vol(
ctx_t *ctx,			/* client connection	*/
const vsn_t vsn			/* vsn			*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove disk vol";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vsn)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)vsn;

	SAMRPC_CLNT_CALL(samrpc_remove_disk_vol, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * add_client
 * function to add a client
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
add_client(
ctx_t *ctx,			/* rpc client connection	*/
host_t new_client		/* sam client			*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:add client";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(new_client)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)new_client;

	SAMRPC_CLNT_CALL(samrpc_add_client, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * remove_client
 * function to remove a client
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
remove_client(
ctx_t *ctx,			/* rpc client connection	*/
host_t new_client		/* sam client			*/
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:remove client";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(new_client)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)new_client;

	SAMRPC_CLNT_CALL(samrpc_remove_client, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * set_disk_vol_flags
 * function to set flags for a volume
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
set_disk_vol_flags(
ctx_t *ctx,		/* rpc client connection */
char *vol_name,	/* volume name */
uint32_t flag		/* flags to be set */
)
{

	int ret_val;
	string_uint32_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:set flags for volume";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(vol_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)vol_name;
	arg.u_flag = flag;

	SAMRPC_CLNT_CALL(samrpc_set_disk_vol_flags, string_uint32_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
