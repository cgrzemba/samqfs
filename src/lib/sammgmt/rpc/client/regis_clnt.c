
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

#pragma ident	"$Revision: 1.7 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * regis_clnt.c
 *
 * RPC client wrapper for registration api
 */


int
cns_get_registration(
ctx_t *ctx,		/* client connection		*/
char *asset_prefix,	/* asset for which to get registration */
char **reg_kv		/* return key value string for the registration */
)
{

	int ret_val;
	string_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get registration";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(asset_prefix, reg_kv)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = asset_prefix;

	SAMRPC_CLNT_CALL(samrpc_cns_get_registration, string_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*reg_kv = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}



int
cns_register(ctx_t *ctx,
char *kv_string,
crypt_str_t *cl_pwd,
crypt_str_t *proxy_pwd,
char *clnt_pub_key_hex)
{

	int ret_val;
	cns_reg_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:cns_register";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(kv_string, cl_pwd, clnt_pub_key_hex)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.kv = kv_string;
	arg.hex = clnt_pub_key_hex;
	arg.cl_pwd = cl_pwd;
	arg.proxy_pwd = proxy_pwd;

	SAMRPC_CLNT_CALL(samrpc_cns_register, cns_reg_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
cns_get_public_key(
ctx_t *ctx,
char **server_pub_key_hex,
crypt_str_t **signature)
{
	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	public_key_result_t *pub_key;

	char *func_name = "rpc:get public key";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(ctx, server_pub_key_hex, signature)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_cns_get_public_key, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	pub_key = (public_key_result_t *)
	    result.samrpc_result_u.result.result_data;

	if (pub_key) {
		*server_pub_key_hex = pub_key->pub_key_hex;
		*signature = pub_key->signature;
	} else {
		*server_pub_key_hex = NULL;
		*signature = NULL;
	}

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
