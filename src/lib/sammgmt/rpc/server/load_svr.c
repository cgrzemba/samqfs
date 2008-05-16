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

#pragma ident	"$Revision: 1.13 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>
#include "pub/mgmt/load.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * load_svr.c
 *
 * RPC server wrapper for load api
 */

samrpc_result_t *
samrpc_get_pending_load_info_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all pending load information");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get pending load");
	ret = get_pending_load_info(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_LD_PENDING_LOAD_INFO_LIST, lst);

	Trace(TR_DEBUG, "Get pending load return[%d] with[%d] loads",
	    ret, (ret == -1) ? ret : lst->length);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_clear_load_request_1_svr(
clear_load_request_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Clear load request vsn[%s] index[%d]",
	    arg->vsn, arg->index);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to clear load request");
	ret = clear_load_request(arg->ctx, arg->vsn, arg->index);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Clear load request return[%d]", ret);
	return (&rpc_result);
}
