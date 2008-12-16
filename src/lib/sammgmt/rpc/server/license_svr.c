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

#pragma ident	"$Revision: 1.17 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>
#include "pub/mgmt/license.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * license_svr.c
 *
 * RPC server wrapper for license api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;


samrpc_result_t *
samrpc_get_license_type_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Get the license type");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get the license type");
	ret = get_license_type(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Get license type return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_expiration_date_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Get the license expiration date");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get expiration date");
	ret = get_expiration_date(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Get license expiration date return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_samfs_type_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Get the SAMFS type");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get SAMFS type");
	ret = get_samfs_type(arg->ctx);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Get SAMFS type return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_licensed_media_slots_1_svr(
string_string_arg_t *arg,	/* argument to api */
struct svc_req *req			/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Get total number of licensed media slots");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get licensed media slot");
	ret = get_licensed_media_slots(
	    arg->ctx,
	    arg->str1,
	    arg->str2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Total number of licensed media slot[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_licensed_media_types_1_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get list of licensed media types");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get licensed media type");
	ret = get_licensed_media_types(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_LIC_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get licensed media types return[%d] with [%d] types",
	    ret, (ret == -1) ? ret : lst->length);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_license_info_3_svr(
ctx_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	license_info_t *info = NULL;

	Trace(TR_DEBUG, "Get license info");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* no timestamp check required */

	Trace(TR_DEBUG, "Calling native library to get license info");
	ret = get_license_info(arg->ctx, &info);

	SAMRPC_SET_RESULT(ret, SAM_LIC_INFO, info);

	Trace(TR_DEBUG, "Get license info return[%d]", ret);

	return (&rpc_result);
}
