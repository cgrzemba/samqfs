
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident	"$Revision: 1.12 $"

#include <stdlib.h>
#include "pub/mgmt/file_metrics_report.h"
#include "mgmt/sammgmt.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * report_svr.c
 *
 * RPC server wrapper for report api
 */

samrpc_result_t *
samrpc_gen_report_6_svr(
report_requirement_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Generate report");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to generate report");
	ret = gen_report(arg->ctx, arg->req);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Generate report return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_file_metrics_report_6_svr(
file_metric_rpt_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{
	int	ret = -1;
	char	*buf = NULL;

	Trace(TR_DEBUG, "Get file metrics report");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get file metrics report");
	ret = get_file_metrics_report(arg->ctx, arg->str, arg->which,
	    arg->start, arg->end, &buf);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, buf);

	Trace(TR_DEBUG, "Get file metrics report return[%d]:[%s]",
	    ret, (ret == 0) ? buf : "NULL");
	return (&rpc_result);
}
