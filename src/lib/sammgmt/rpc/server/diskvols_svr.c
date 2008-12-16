
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

#pragma ident	"$Revision: 1.15 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

/*
 * diskvols_svr.c
 *
 * RPC server wrapper for diskvols api
 */

/*
 * samrpc_get_disk_vol_1_svr
 * server stub for get_disk_vol
 */
samrpc_result_t *
samrpc_get_disk_vol_1_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	disk_vol_t *disk_vol;

	Trace(TR_DEBUG, "Get diskvol");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get diskvol");
	ret = get_disk_vol(arg->ctx, arg->str, &disk_vol);

	SAMRPC_SET_RESULT(ret, SAM_DISKVOL, disk_vol);

	Trace(TR_DEBUG, "Get diskvol return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_get_all_disk_vols
 * server stub for get_all_disk_vols
 */
samrpc_result_t *
samrpc_get_all_disk_vols_1_svr(
ctx_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all diskvols");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get all diskvols");
	ret = get_all_disk_vols(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_DISKVOL_LIST, lst);

	Trace(TR_DEBUG, "Get all diskvols return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_get_all_clients
 * server stub for get_all_clients
 */
samrpc_result_t *
samrpc_get_all_clients_1_svr(
ctx_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	sqm_lst_t *lst;

	Trace(TR_DEBUG, "Get all clients");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get all clients");
	ret = get_all_clients(arg->ctx, &lst);

	SAMRPC_SET_RESULT(ret, SAM_CLIENT_LIST, lst);

	Trace(TR_DEBUG, "Get all clients return[%d]", ret);
	return (&rpc_result);
}

/*
 * samrpc_add_disk_vol
 * server stub for add_disk_vol
 */
samrpc_result_t *
samrpc_add_disk_vol_1_svr(
disk_vol_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Add diskvol");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to add diskvol");
	ret = add_disk_vol(arg->ctx, arg->disk_vol);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Add diskvol return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_remove_disk_vol
 * server stub for remove_disk_vol
 */
samrpc_result_t *
samrpc_remove_disk_vol_1_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Remove diskvol[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to remove diskvol");
	ret = remove_disk_vol(arg->ctx, arg->str);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Remove diskvol return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_add_client
 * server stub for add_client
 */
samrpc_result_t *
samrpc_add_client_1_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Add client[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to add client");
	ret = add_client(arg->ctx, arg->str);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Add client return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_remove_client
 * server stub for remove_client
 */
samrpc_result_t *
samrpc_remove_client_1_svr(
string_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Remove client[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to remove client");
	ret = remove_client(arg->ctx, arg->str);

	/*
	 * Multiple client support
	 *
	 * The configuration has changed. So an other client should get an
	 * error for any operation unless the cleint resyncs with the rpc
	 * server by invoking init_sam_mgmt
	 *
	 * Update the rpc server timestamp
	 */
	SAMRPC_UPDATE_TIMESTAMP(ret);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Remove client return[%d]", ret);
	return (&rpc_result);
}


/*
 * samrpc_set_disk_vol_flags
 * server stub for set diskvol flags
 */
samrpc_result_t *
samrpc_set_disk_vol_flags_4_svr(
string_uint32_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;

	Trace(TR_DEBUG, "Set flag for volume[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to set flag for volume");
	ret = set_disk_vol_flags(arg->ctx, arg->str, arg->u_flag);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Remove client return[%d]", ret);
	return (&rpc_result);

}
