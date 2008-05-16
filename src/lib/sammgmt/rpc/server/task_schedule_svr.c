
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

#pragma ident	"$Revision: 1.8 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * task_schedule_svr.c
 *
 * RPC server wrapper task scheduler api
 */


samrpc_result_t *
samrpc_get_task_schedules_6_svr(
ctx_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get task schedules");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get task schedules");
	ret = get_task_schedules(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get task schedules return[%d] with [%d] results",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_specific_tasks_6_svr(
string_string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get specific task schedules");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get specific tasks");
	ret = get_specific_tasks(
	    arg->ctx,
	    arg->str1,
	    arg->str2,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get specific tasks return[%d] with [%d] results",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_set_task_schedule_6_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "set task schedule[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to set task schedule");
	ret = set_task_schedule(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "set task schedule return [%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_remove_task_schedule_6_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "remove task schedule[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to remove task schedule");
	ret = remove_task_schedule(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "remove task schedule return [%d]", ret);

	return (&rpc_result);
}
