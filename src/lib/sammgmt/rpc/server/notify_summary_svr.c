
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
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * notify_summary_svr.c
 *
 * RPC server wrapper for notify_summary api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * samrpc_get_notify_summary_1_svr
 * server stub for get_notify_summary
 */
samrpc_result_t *
samrpc_get_notify_summary_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get notification summary");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get notify summary");
	ret = get_notify_summary(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_NOTIFY_SUMMARY_LIST, lst);

	Trace(TR_DEBUG, "Get notify summary return[%d] with [%d] notifications",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


/*
 * samrpc_del_notify_summary_1_svr
 * server stub for del_notify_summary
 */
samrpc_result_t *
samrpc_del_notify_summary_1_svr(
notify_summary_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Delete notification summary");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to delete notify summary");
	ret = del_notify_summary(arg->ctx, arg->notf_summ);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Delete notify summary return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_mod_notify_summary_1_svr
 * server stub for mod_notify_summary
 */
samrpc_result_t *
samrpc_mod_notify_summary_1_svr(
mod_notify_summary_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Modify notify summary");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to modify notify summary");
	ret = mod_notify_summary(
	    arg->ctx,
	    arg->oldemail,
	    arg->notf_summ);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Modify notify summary return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_add_notify_summary_1_svr
 * server stub for add_notify_summary
 */
samrpc_result_t *
samrpc_add_notify_summary_1_svr(
notify_summary_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Add notify summary");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to add notify summary");

	ret = add_notify_summary(arg->ctx, arg->notf_summ);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Add notify summary return[%d]", ret);
	return (&rpc_result);
}

/*
 * samrpc_get_email_addrs_by_subj_5_svr
 * server stub for get_email_addrs_by_subj
 */
samrpc_result_t *
samrpc_get_email_addrs_by_subj_5_svr(
get_email_addrs_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	char	*str;

	Trace(TR_DEBUG, "Get email addrs by subj");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get notify summary");
	ret = get_email_addrs_by_subj(arg->ctx, arg->subj_wanted, &str);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, str);

	Trace(TR_DEBUG, "Get email addrs by subj [%d]", ret);

	return (&rpc_result);
}
