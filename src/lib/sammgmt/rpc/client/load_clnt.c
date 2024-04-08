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

#pragma ident	"$Revision: 1.17 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * load_clnt.c
 *
 * RPC client wrapper for load api
 */

/*
 * get_pending_load_info
 *
 * Function to get information about all pending load information.
 */
int
get_pending_load_info(
ctx_t *ctx,			/* client connection		*/
sqm_lst_t **pending_load_infos	/* return - pending load	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get pending load info";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(pending_load_infos)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_pending_load_info, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*pending_load_infos = (sqm_lst_t *)
	    result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*pending_load_infos));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * clear_load_request
 *
 * cancel a load request
 */
int
clear_load_request(
ctx_t *ctx,	/* client connection		*/
vsn_t vsn,	/* vsn				*/
int index	/* index in preview queue or -1	*/
)
{

	int ret_val;
	clear_load_request_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:clear load request";
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
	strcpy(arg.vsn, vsn);
	arg.index = index;

	SAMRPC_CLNT_CALL(samrpc_clear_load_request, clear_load_request_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
