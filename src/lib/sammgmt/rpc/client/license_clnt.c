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

#pragma ident	"$Revision: 1.19 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"
#include "pub/mgmt/license.h"

/*
 * license_clnt.c
 *
 * RPC client wrapper for license api
 */

/*
 * Get the license type:
 *	 NON_EXPIRING, EXPIRING, DEMO, QFS_TRIAL, or QFS_SPECIAL
 */
int
get_license_type(
ctx_t *ctx	/* client connection */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get license type";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_license_type, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning license type [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get the license expiration date if the license type is EXPIRING
 * (in seconds since 1970)
 */
int
get_expiration_date(
ctx_t *ctx	/* client connection */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get expiration date";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_expiration_date, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning expiration date [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get the SAMFS type: QFS, SAM-FS, or SAM-QFS
 */
int
get_samfs_type(
ctx_t *ctx	/* client connection */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get samfs type";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_samfs_type, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning samfs type [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get the total number of licensed slots for the given
 * robot_type and media_type pair
 */
int
get_licensed_media_slots(
ctx_t *ctx,		/* client connection	*/
const char *robot_name,	/* robot type		*/
const char *media_name	/* media_type		*/
)
{

	int ret_val;
	string_string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get licensed media slots";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(robot_name, media_name)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str1 = (char *)robot_name;
	arg.str2 = (char *)media_name;

	SAMRPC_CLNT_CALL(samrpc_get_licensed_media_slots, string_string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning licensed slots [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get a list of licensed media_types (strings)
 */
int
get_licensed_media_types(
ctx_t *ctx,		/* client connection */
sqm_lst_t **media_list	/* return - list of media types */
)
{

	int ret_val;
	ctx_arg_t arg;
	char *func_name = "rpc:get licensed media types";
	char *err_msg;
	enum clnt_stat stat;
	samrpc_result_t result;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(media_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_licensed_media_types, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*media_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*media_list));

	PTRACE(2, "%s returning list of media types [size = %d]...",
	    func_name, (*media_list)->length);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * Get license info
 */
int
get_license_info(
ctx_t *ctx,		/* client connection */
license_info_t **info	/* return - struct license info */
)
{

	int ret_val;
	ctx_arg_t arg;
	char *func_name = "rpc:get license info";
	char *err_msg;
	enum clnt_stat stat;
	samrpc_result_t result;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(info)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_license_info, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*info = (license_info_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning license info[%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
