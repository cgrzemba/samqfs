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

#pragma ident	"$Revision: 1.7 $"
static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

#include "mgmt/sammgmt.h"
#include <stdlib.h>

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

samrpc_result_t *
samrpc_cns_get_registration_6_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	char *reg_kv;

	Trace(TR_DEBUG, "Get registration");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get registration");
	ret = cns_get_registration(
	    arg->ctx,
	    arg->str,
	    &reg_kv);


	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, reg_kv);

	Trace(TR_DEBUG, "Get registration returned");
	return (&rpc_result);
}

samrpc_result_t *
samrpc_cns_register_6_svr(
cns_reg_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_MISC, "register");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to register");
	ret = cns_register(
	    arg->ctx,
	    arg->kv,
	    arg->cl_pwd,
	    arg->proxy_pwd,
	    arg->hex);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_MISC, "register returned");
	return (&rpc_result);
}


samrpc_result_t *
samrpc_cns_get_public_key_6_svr(
ctx_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	public_key_result_t *res = NULL;

	Trace(TR_MISC, "get public key");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_MISC, "Calling native library to get public key");
	res = (public_key_result_t *)mallocer(sizeof (public_key_result_t));
	if (res != NULL) {
		ret = cns_get_public_key(
		    arg->ctx,
		    &(res->pub_key_hex),
		    &(res->signature));
	}

	SAMRPC_SET_RESULT(ret, SAM_PUBLIC_KEY, res);

	Trace(TR_MISC, "get public key returned %s",
	    Str(res->pub_key_hex));
	return (&rpc_result);
}
