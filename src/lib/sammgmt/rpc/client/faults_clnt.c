
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

#pragma ident	"$Revision: 1.26 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * faults_clnt.c
 *
 * RPC client wrapper for faults api
 */
#define	INITIALIZE_ARRAY(array, len) \
	{ int i; \
		for (i = 0; i < len; i++) { \
			array[i] = 0; \
		} \
	}
/*
 * get_faults
 * deprecated in API_VERSION 1.5.0. Use get_all_faults instead
 * Function to get all the faults which match the request arguments
 *
 * The faults_list is populated with  the faults
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and faults_list is NULL.
 */
int
get_faults(
ctx_t *ctx,		/* client connection		*/
flt_req_t fault_req,	/* fault request		*/
sqm_lst_t **faults_list	/* return - list of fault	*/
)
{
	int ret_val;
	fault_req_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get faults";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(faults_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.fault_req = fault_req;

	SAMRPC_CLNT_CALL(samrpc_get_faults, fault_req_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*faults_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail of the list
	 */
	SET_LIST_TAIL((*faults_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_faults_by_lib
 * Function to get all the faults by library
 *
 * The faults_list is populated with  the faults
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and faults_list is NULL.
 */
int
get_faults_by_lib(
ctx_t *ctx,		/* client connection		*/
uname_t library,	/* Tape library family-set name */
sqm_lst_t **faults_list	/* return - list of fault	*/
)
{
	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get faults by lib";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(faults_list, library)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = (char *)library;

	SAMRPC_CLNT_CALL(samrpc_get_faults_by_lib, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*faults_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail of the list
	 */
	SET_LIST_TAIL((*faults_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_faults_by_eq
 * Function to get all the faults by eq
 *
 * The faults_list is populated with  the faults
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and faults_list is NULL.
 */
int
get_faults_by_eq(
ctx_t *ctx,		/* client connection		*/
equ_t eq,		/* Tape library eq */
sqm_lst_t **faults_list	/* return - list of fault	*/
)
{
	int ret_val;
	equ_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get faults by eq";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(faults_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.eq = eq;

	SAMRPC_CLNT_CALL(samrpc_get_faults_by_eq, equ_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*faults_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail of the list
	 */
	SET_LIST_TAIL((*faults_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * is_faults_gen_status_on
 * Function to get the status of fault generation
 *
 * The status of SNMP events generation is dependent on setting
 * "alerts = on" in defaults.conf
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 */
int
is_faults_gen_status_on(
ctx_t *ctx,		/* client connection		*/
boolean_t *faults_gen_status	/* status of fault generation	*/
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:is faults gen status on";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(faults_gen_status)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_is_faults_gen_status_on, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*faults_gen_status = *((boolean_t *)
	    result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/* update_fault is deprecated in API_VERSION 1.5.0 */

/*
 * delete_faults
 * function to delete the fault from FAULTLOG
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * Only DEFAULT_MAX (700) faults can be deleted at one time, the others
 * are ignored
 */
int
delete_faults(
ctx_t *ctx,		/* client connection	*/
int num,		/* number of faults to delete */
long errorID[]		/* array of errorID identifying faults to be deleted */
)
{

	int ret_val;
	fault_errorid_arr_arg_t *arg;
	samrpc_result_t result;
	char *func_name = "rpc:Delete fault";
	char *err_msg;
	enum clnt_stat stat;
	int n, n_max;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg = (fault_errorid_arr_arg_t *)mallocer(
	    sizeof (fault_errorid_arr_arg_t)
	    + ((num - 1) * sizeof (long)));

	memset(arg, 0, sizeof (fault_errorid_arr_arg_t));

	arg->ctx = ctx;
	arg->num = num;

	INITIALIZE_ARRAY(arg->errorID, DEFAULTS_MAX);
	n_max = (num < DEFAULTS_MAX) ? num : DEFAULTS_MAX;

	for (n = 0; n < n_max; n++) {

		arg->errorID[n] = errorID[n];
	}
	if ((stat = samrpc_delete_faults_1(arg, &result,
	    ctx->handle->clnt)) != RPC_SUCCESS) {
		SET_RPC_ERROR(ctx, func_name, stat);
	}

	free(arg);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * ack_faults
 * function to mark the faults as acknowledged
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * Only DEFAULT_MAX (700) faults can be acknowledged at one time, the others
 * are ignored
 */
int
ack_faults(
ctx_t *ctx,		/* client connection	*/
int num,		/* number of faults to ACK */
long errorID[]		/* array of errorID identifying faults to be ACK */
)
{

	int ret_val;
	fault_errorid_arr_arg_t *arg;
	samrpc_result_t result;
	char *func_name = "rpc:Acknowledge fault";
	char *err_msg;
	enum clnt_stat stat;
	int n, n_max;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg = (fault_errorid_arr_arg_t *)mallocer(
	    sizeof (fault_errorid_arr_arg_t)
	    + ((num - 1) * sizeof (long)));

	memset(arg, 0, sizeof (fault_errorid_arr_arg_t));

	arg->ctx = ctx;
	arg->num = num;

	INITIALIZE_ARRAY(arg->errorID, DEFAULTS_MAX);
	n_max = (num < DEFAULTS_MAX) ? num : DEFAULTS_MAX;

	for (n = 0; n < n_max; n++) {

		arg->errorID[n] = errorID[n];
	}
	if ((stat = samrpc_ack_faults_1(arg, &result,
	    ctx->handle->clnt)) != RPC_SUCCESS) {
		SET_RPC_ERROR(ctx, func_name, stat);
	}

	free(arg);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get fault summary
 *
 * Return the fault summary, just the count of major, minor and critical
 * faults
 *
 * Parameters:
 *      ctx_t    *ctx   context argument (only used by RPC layer)
 *	fault_summary_t *summary (count of major, minor, critical faults)
 */
int
get_fault_summary(
ctx_t *ctx,		/* client connection	*/
fault_summary_t *fault_summary /* return fault summary, count of faults */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:Get fault summary";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));

	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_fault_summary, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*fault_summary = *((fault_summary_t *)
	    result.samrpc_result_u.result.result_data);

	free(result.samrpc_result_u.result.result_data);
	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * get_all_faults
 * Function to get all the faults
 *
 * The faults_list is populated with  the faults
 *
 * If there is an error, the return value is set to the return value from the
 * server API. The samerrno and samerrmsg are passed on to the client
 * and faults_list is NULL.
 */
int
get_all_faults(
ctx_t *ctx,		/* client connection		*/
sqm_lst_t **faults_list	/* return - list of fault	*/
)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all faults";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(faults_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_all_faults, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*faults_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail of the list
	 */
	SET_LIST_TAIL((*faults_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
