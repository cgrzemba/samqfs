
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

#pragma ident	"$Revision: 1.21 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * samrpc_get_all_faults_6_svr
 * server stub for get_all_faults
 */
samrpc_result_t *
samrpc_get_all_faults_6_svr(
ctx_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get faults");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to get faults");
	ret = get_all_faults(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_FAULTS_LIST, lst);

	Trace(TR_DEBUG, "Get faults return[%d] with [%d] faults",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


/*
 * samrpc_get_faults_by_lib_1_svr
 * server stub for get_faults_by_lib
 */
samrpc_result_t *
samrpc_get_faults_by_lib_1_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get faults by library[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get faults");
	ret = get_faults_by_lib(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_FAULTS_LIST, lst);

	Trace(TR_DEBUG, "Get faults return[%d] with [%d] faults",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


/*
 * samrpc_get_faults_by_eq_1_svr
 * server stub for get_faults_by_eq
 */
samrpc_result_t *
samrpc_get_faults_by_eq_1_svr(
equ_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get faults by eq[%d]", arg->eq);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get faults");
	ret = get_faults_by_eq(arg->ctx, arg->eq, &lst);

	SAMRPC_SET_RESULT(ret, SAM_FAULTS_LIST, lst);

	Trace(TR_DEBUG, "Get faults return[%d] with [%d] faults",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


/*
 * samrpc_is_faults_gen_status_on_1_svr
 * server stub for is_faults_gen_status_on
 */
samrpc_result_t *
samrpc_is_faults_gen_status_on_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	boolean_t *status = NULL;

	Trace(TR_DEBUG, "Get fault generation status");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	status = (boolean_t *)mallocer(sizeof (boolean_t));
	if (status != NULL) {
		Trace(TR_DEBUG, "Calling native library for fault gen status");
		ret = is_faults_gen_status_on(arg->ctx, status);
	}
	SAMRPC_SET_RESULT(ret, SAM_PTR_BOOL, status);

	Trace(TR_DEBUG, "Get fault generation status return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_delete_faults_1_svr
 * server stub for delete_faults
 */
samrpc_result_t *
samrpc_delete_faults_1_svr(
fault_errorid_arr_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Delete faults");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to delete faults");
	ret = delete_faults(arg->ctx, arg->num, arg->errorID);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Delete faults return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_ack_faults_1_svr
 * server stub for ack_faults
 */
samrpc_result_t *
samrpc_ack_faults_1_svr(
fault_errorid_arr_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Acknowledge faults");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to ACK faults");
	ret = ack_faults(arg->ctx, arg->num, arg->errorID);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Acknowledge faults return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_get_fault_summary_1_svr
 * server stub for get fault summary
 */
samrpc_result_t *
samrpc_get_fault_summary_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	fault_summary_t *fault_summary;

	Trace(TR_DEBUG, "Get fault summary");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);
	fault_summary = (fault_summary_t *)mallocer(sizeof (fault_summary_t));
	if (fault_summary != NULL) {

		Trace(TR_DEBUG, "Calling native library to get fault summary");
		ret = get_fault_summary(arg->ctx, fault_summary);
	}

	SAMRPC_SET_RESULT(ret, SAM_FAULTS_SUMMARY, fault_summary);

	Trace(TR_DEBUG, "Get fault summary return[%d]", ret);
	return (&rpc_result);
}
