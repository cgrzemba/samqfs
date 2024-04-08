
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

#pragma ident	"$Revision: 1.14 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * arch_set_svr.c
 *
 * RPC server wrapper for archive api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * *************
 *  config APIs
 * *************
 */

samrpc_result_t *
samrpc_get_all_arch_sets_3_svr(
ctx_arg_t *arg,		/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *ar_set_lst;

	Trace(TR_DEBUG, "Get all archive sets");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib get all arch sets");
	ret = get_all_arch_sets(arg->ctx, &ar_set_lst);

	SAMRPC_SET_RESULT(ret, SAM_AR_ARCH_SET_LIST, ar_set_lst);

	Trace(TR_DEBUG, "Get all archive sets return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_arch_set_3_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	arch_set_t *ar_set;

	Trace(TR_DEBUG, "Get archive set");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib get arch set");
	ret = get_arch_set(arg->ctx, arg->str, &ar_set);

	SAMRPC_SET_RESULT(ret, SAM_AR_ARCH_SET, ar_set);

	Trace(TR_DEBUG, "Get archive set return[%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_create_arch_set_3_svr(
arch_set_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	Trace(TR_DEBUG, "Create archive set");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib create arch set");
	ret = create_arch_set(arg->ctx, arg->set);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Create archive set return[%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_modify_arch_set_3_svr(
arch_set_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Modify archive set");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib modify arch set");
	ret = modify_arch_set(arg->ctx, arg->set);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Modify archive set return[%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_delete_arch_set_3_svr(
string_arg_t *arg,	/* arguments to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Delete archive set");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Call native lib delete arch set");
	ret = delete_arch_set(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Delete archive set return[%d]", ret);

	return (&rpc_result);
}
