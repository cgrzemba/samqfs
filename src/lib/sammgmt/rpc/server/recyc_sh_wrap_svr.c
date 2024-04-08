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
#include "pub/mgmt/recyc_sh_wrap.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * recyc_sh_wrap_svr.c
 *
 * RPC server wrapper for recycler.sh wrapper api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * samrpc_get_recycl_sh_action_status_1_svr
 * server stub for get_recycl_sh_action_status
 */
samrpc_result_t *
samrpc_get_recycl_sh_action_status_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Get recycler.sh action status");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to get recycler.sh action status");
	ret = get_recycl_sh_action_status(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);
	Trace(TR_DEBUG, "Get recycler.sh action status return[%d]", ret);
	return (&rpc_result);
}

/*
 * samrpc_add_recycle_sh_action_label_1_svr
 * server stub for add_recycle_sh_action_label
 */
samrpc_result_t *
samrpc_add_recycle_sh_action_label_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Add label to recycler.sh");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to add label to recycler.sh");
	ret = add_recycle_sh_action_label(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);
	Trace(TR_DEBUG, "Add label to recycler.sh return[%d]", ret);
	return (&rpc_result);
}

/*
 * samrpc_add_recycle_sh_action_export_1_svr
 * server stub for add_recycle_sh_action_export
 */
samrpc_result_t *
samrpc_add_recycle_sh_action_export_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Add export to recycler.sh");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to add export to recycler.sh");

	ret = add_recycle_sh_action_export(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);
	Trace(TR_DEBUG, "Add export to recycler.sh return[%d]", ret);
	return (&rpc_result);
}

/*
 * samrpc_del_recycle_sh_action_1_svr
 * server stub for del_recycle_sh_action
 */
samrpc_result_t *
samrpc_del_recycle_sh_action_1_svr(
	ctx_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Delete export/label from recycler.sh");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib to del export/label from recycler.sh");
	ret = del_recycle_sh_action(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);
	Trace(TR_DEBUG, "Delete export/label from recycler.sh return[%d]", ret);
	return (&rpc_result);
}
