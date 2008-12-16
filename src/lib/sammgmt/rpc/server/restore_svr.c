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

#pragma ident	"$Revision: 1.20 $"

#include "mgmt/sammgmt.h"
#include <stdlib.h>
#include "pub/mgmt/restore.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * restore_svr.c
 *
 * RPC server wrapper for restore api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

samrpc_result_t *
samrpc_set_csd_params_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Set csd params[%s] for fsname[%s]",
	    arg->str2, arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to set csd params");
	ret = set_csd_params(arg->ctx, arg->str1, arg->str2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Set csd params return [%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_csd_params_4_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *str = NULL;

	Trace(TR_DEBUG, "Get csd params for fsname[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get csd params");
	ret = get_csd_params(arg->ctx, arg->str, &str);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, str);

	Trace(TR_DEBUG, "Get csd params return [%d][%s]",
	    ret, (str != NULL) ? str : "NULL");
	return (&rpc_result);
}

samrpc_result_t *
samrpc_list_dumps_4_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "List dumps for fsname[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to list_dumps");
	ret = list_dumps_by_dir(arg->ctx, arg->str, NULL, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "List dumps return [%d] with [%d] dumps",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_list_dumps_by_dir_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "List dumps by dir for fsname[%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to list_dumps_by_dir");
	ret = list_dumps_by_dir(arg->ctx, arg->str1, arg->str2, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "List dumps by dir return [%d] with [%d] dumps",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_dump_status_4_svr(
	string_strlst_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get dump status for fsname[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get dumps status");

	/* set the tail on the list received over RPC */
	SET_LIST_TAIL(arg->strlst);

	ret = get_dump_status(arg->ctx, arg->str, arg->strlst, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get dump status return [%d] with [%d] status",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_dump_status_by_dir_4_svr(
	string_string_strlst_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get dump status by dir for fsname[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get dumps status by dir");

	/* set the tail on the list received over RPC */
	SET_LIST_TAIL(arg->strlst);

	ret = get_dump_status_by_dir(
	    arg->ctx,
	    arg->str,
	    arg->str2,
	    arg->strlst,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get dump status by dir return [%d] with [%d] status",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_decompress_dump_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *str = NULL;

	Trace(TR_DEBUG, "Decompress dump[%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to decompress dump");
	ret = decompress_dump(arg->ctx, arg->str1, arg->str2, &str);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, str);

	Trace(TR_DEBUG, "decompress dump return [%d][%s]",
	    ret, (str != NULL) ? str : "NULL");
	return (&rpc_result);
}

samrpc_result_t *
samrpc_cleanup_dump_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Cleanup dump[%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to cleanup dump");
	ret = cleanup_dump(arg->ctx, arg->str1, arg->str2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "cleanup dump return [%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_delete_dump_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Delete dump[%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to delete dump");
	ret = delete_dump(arg->ctx, arg->str1, arg->str2);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "delete dump return [%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_take_dump_4_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *str = NULL;

	Trace(TR_DEBUG, "Take dump[%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to take dump");
	ret = take_dump(arg->ctx, arg->str1, arg->str2, &str);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, str);

	Trace(TR_DEBUG, "take dump return [%d][%s]",
	    ret, (str != NULL) ? str : "NULL");
	return (&rpc_result);
}


samrpc_result_t *
samrpc_list_versions_4_svr(
	file_restrictions_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "List versions [%s], [%s], [%d], [%s], [%s]",
	    arg->fsname, arg->dumppath, arg->maxentries,
	    arg->filepath, arg->restrictions);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to list versions");
	ret = list_versions(
	    arg->ctx,
	    arg->fsname,
	    arg->dumppath,
	    arg->maxentries,
	    arg->filepath,
	    arg->restrictions,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "List versions return [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_version_details_4_svr(
	version_details_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get version details for fsname[%s]", arg->fsname);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get version details");
	ret = get_version_details(
	    arg->ctx, arg->fsname, arg->dumpname, arg->filepath, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get version details return [%d] with [%d] elements",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_search_versions_4_svr(
	file_restrictions_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *str = NULL;

	Trace(TR_DEBUG, "Search versions for filepath[%s]", arg->filepath);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to search versions");
	ret = search_versions(
	    arg->ctx,
	    arg->fsname,
	    arg->dumppath,
	    arg->maxentries,
	    arg->filepath,
	    arg->restrictions,
	    &str);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, str);

	Trace(TR_DEBUG, "Search versions return [%d] with [%s]",
	    ret, (str != NULL) ? str : "NULL");
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_search_results_4_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "get search results");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get search results");
	ret = get_search_results(
	    arg->ctx,
	    arg->str,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "get search results return [%d] with [%d] elements",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_restore_inodes_4_svr(
	restore_inodes_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	char *str = NULL;

	Trace(TR_DEBUG, "Restore inodes");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to restore inodes");
	ret = restore_inodes(
	    arg->ctx,
	    arg->fsname,
	    arg->dumppath,
	    arg->filepaths,
	    arg->destinations,
	    arg->copies,
	    arg->replace,
	    &str);

	SAMRPC_SET_RESULT(ret, SAM_MGMT_STRING, str);

	Trace(TR_DEBUG, "Restore inodes return [%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_set_snapshot_locked_5_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Lock dump[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to lock dump");
	ret = set_snapshot_locked(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "set_snapshot_locked return [%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_clear_snapshot_locked_5_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Unlock dump[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to unlock dump");
	ret = clear_snapshot_locked(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "clear_snapshot_locked return [%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_indexed_snapshot_directories_6_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Getting indexed snapshot directories for fsname[%s]",
	    arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get index directories");
	ret = get_indexed_snapshot_directories(arg->ctx, arg->str, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get index directories returned [%d] with [%d] dirs",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_indexed_snapshots_6_svr(
	string_string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Getting indexed snapshots [%s]", arg->str1);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get indexed snapshots");
	ret = get_indexed_snapshots(arg->ctx, arg->str1, arg->str2, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get index snapshots returned [%d] with [%d] indices",
	    ret, (lst != NULL) ? lst->length : -1);

	return (&rpc_result);
}
