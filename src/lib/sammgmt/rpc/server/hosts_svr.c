
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

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * hosts_svr.c
 *
 * RPC server wrapper for hosts api
 */

samrpc_result_t *
samrpc_get_host_config_3_svr(
string_arg_t *arg,		/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get all hosts configuration");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get hosts config");
	ret = get_host_config(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_HOST_INFO_LIST, lst);

	Trace(TR_DEBUG, "Get host config return[%d] with[%d] hosts",
	    ret, (ret == -1) ? ret : lst->length);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_remove_host_3_svr(
string_string_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Remove host[%s] from shared fs[%s]",
	    arg->str1, arg->str2);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to remove host");
	ret = remove_host(arg->ctx, arg->str1, arg->str2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Remove host return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_change_metadata_server_3_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Change metadata server[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to remove host");
	/* not yet impl */
	/* ret = change_metadata_server(arg->ctx, arg->str); */

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Change metadata server return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_add_host_3_svr(
string_host_info_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Add host[%s] to shared fs[%s]",
	    arg->h->host_name, arg->fs_name);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to add host");
	ret = add_host(arg->ctx, arg->fs_name, arg->h);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Add host return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_discover_ip_addresses_3_svr(
ctx_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Discover ip adresses");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	/* No timestamp check required */

	Trace(TR_DEBUG, "Calling native library to discover ip addresses");
	ret = discover_ip_addresses(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Dicover ip addresses return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_advanced_network_cfg_5_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get advanced network configuration");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get advanced net config");
	ret = get_advanced_network_cfg(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get advanced net cfg return[%d] with[%d] entries",
	    ret, (ret == -1) ? ret : lst->length);

	return (&rpc_result);
}



samrpc_result_t *
samrpc_set_advanced_network_cfg_5_svr(
string_strlst_arg_t *arg,	/* argument to api */
struct svc_req *req		/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set advanced network configuration");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to set advanced net config");
	ret = set_advanced_network_cfg(arg->ctx, arg->str, arg->strlst);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set advanced net cfg return[%d]", ret);

	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_mds_host_6_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	char *str = NULL;

	Trace(TR_DEBUG, "Getting mds host");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get mds host");
	ret = get_mds_host(arg->ctx, arg->str, &str);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, str);

	Trace(TR_DEBUG, "Get mds host return[%d] with[%s] entries",
	    ret, (str != NULL) ? str : "NULL");

	return (&rpc_result);
}
