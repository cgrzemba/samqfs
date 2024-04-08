
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

#pragma ident	"$Revision: 1.13 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * job_history_svr.c
 *
 * RPC server wrapper for job_history api
 */

samrpc_result_t *
samrpc_get_jobhist_by_fs_1_svr(
job_hist_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
#ifdef SAMMGMT_CONTROL_API
	job_hist_t *job_hist;
#endif

	Trace(TR_DEBUG, "Get job history by fs[%s]", arg->fsname);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get job history");
#ifdef SAMMGMT_CONTROL_API
	ret = get_jobhist_by_fs(
	    arg->ctx,
	    arg->fsname,
	    arg->job_type,
	    &job_hist);

	SAMRPC_SET_RESULT(ret, SAM_JOB_HISTORY, &job_hist);

	Trace(TR_DEBUG, "Get job history return[%d]", ret);
	return (&rpc_result);
#else
	SAMRPC_NOT_YET();
#endif
}


samrpc_result_t *
samrpc_get_all_jobhist_1_svr(
job_type_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
#ifdef SAMMGMT_CONTROL_API
	sqm_lst_t *job_list;
#endif

	Trace(TR_DEBUG, "Get all job history");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get all job history");
#ifdef SAMMGMT_CONTROL_API
	ret = get_all_jobhist(
	    arg->ctx,
	    arg->job_type,
	    &job_list);

	SAMRPC_SET_RESULT(ret, SAM_JOB_HISTORY_LIST, &job_list);

	Trace(TR_DEBUG, "Get all job history return[%d]", ret);
	return (&rpc_result);
#else
	SAMRPC_NOT_YET();
#endif
}
