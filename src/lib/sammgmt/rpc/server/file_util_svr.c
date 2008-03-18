
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

#pragma ident	"$Id"

#include "mgmt/sammgmt.h"
#include <stdlib.h>
#include "pub/mgmt/file_util.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * file_util_svr.c
 *
 * RPC server wrapper for file utilities api
 */

extern int server_timestamp;
extern bool_t timestamp_updated;
extern samrpc_result_t rpc_result;

samrpc_result_t *
samrpc_list_dir_4_svr(
	file_restrictions_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "List dir max[%d], filepath[%s], restrictions[%s]",
	    arg->maxentries, arg->filepath, arg->restrictions);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to list dir");
	ret = list_dir(
	    arg->ctx,
	    arg->maxentries,
	    arg->filepath,
	    arg->restrictions,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "List dir files return [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_list_directory_6_svr(
	file_restrictions_more_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG,
	    "List directory max[%d], filepath[%s], restrictions[%s]",
	    arg->maxentries, arg->dir, arg->restrictions);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to list dir");
	ret = list_directory(
	    arg->ctx,
	    arg->maxentries,
	    arg->dir,
	    arg->file,
	    arg->restrictions,
	    arg->morefiles,
	    &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "List directory files return [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_file_status_4_svr(
	strlst_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get file status");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get file status");

	ret = get_file_status(arg->ctx, arg->lst, &lst);

	SAMRPC_SET_RESULT(ret, SAM_INT_LIST, lst);

	Trace(TR_DEBUG, "Get file status return [%d] with [%d] entries",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_file_details_4_svr(
	string_strlst_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get file details");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get file details");

	ret = get_file_details(arg->ctx, arg->str, arg->strlst, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get file details return [%d] with [%d] status",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_create_file_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Creating file[%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to create file");
	ret = create_file(
	    arg->ctx,
	    arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Create directory return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_file_exists_3_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "check existing [%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to check existing");
	ret = file_exists(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "check existing return[%d]", ret);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_tail_4_svr(
	string_uint32_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = 0;
	int_ptrarr_result_t *res = NULL;

	res = (int_ptrarr_result_t *)mallocer(sizeof (int_ptrarr_result_t));
	(void) memset(res, 0, sizeof (int_ptrarr_result_t));

	Trace(TR_DEBUG, "get tail");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to get tail");
	res->count = arg->u_flag;

	ret = tail(arg->ctx, arg->str,
	    &(res->count), &(res->pstr), &(res->str));

	SAMRPC_SET_RESULT(ret, SAM_INT_ARRPTRS, res);

	Trace(TR_DEBUG, "tail return [%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_create_dir_1_svr(
	string_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;

	Trace(TR_DEBUG, "Creating directory [%s]", arg->str);

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to create directory");
	ret = create_dir(arg->ctx, arg->str);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Create directory return[%d]", ret);
	return (&rpc_result);
}


samrpc_result_t *
samrpc_get_txt_file_5_svr(
	string_uint32_uint32_arg_t *arg, /* ARGSUSED */
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	int_ptrarr_result_t *res = NULL;

	res = (int_ptrarr_result_t *)mallocer(sizeof (int_ptrarr_result_t));
	(void) memset(res, 0, sizeof (int_ptrarr_result_t));

	Trace(TR_DEBUG, "get txt file");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	Trace(TR_DEBUG, "Calling native library to get txt file");
	res->count = arg->u_2;

	ret = get_txt_file(arg->ctx, arg->str, arg->u_1, &(res->count),
	    &(res->pstr), &(res->str));

	SAMRPC_SET_RESULT(ret, SAM_INT_ARRPTRS, res);

	Trace(TR_DEBUG, "get txt file return [%d]", ret);

	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_extended_file_details_5_svr(
	strlst_uint32_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get extended file details");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get file details");

	ret = get_extended_file_details(arg->ctx, arg->strlst, arg->u32, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get extended file details ret [%d] with [%d] status",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_get_copy_details_5_svr(
	string_uint32_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Get copy details");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get file details");

	ret = get_copy_details(arg->ctx, arg->str, arg->u_flag, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG, "Get copy details ret [%d] with [%d] status",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_list_and_collect_file_details_6_svr(
	file_details_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	file_details_result_t *res = NULL;

	Trace(TR_DEBUG, "List and collect file details");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get file details");

	res = mallocer(sizeof (file_details_result_t));
	if (res != NULL) {
		ret = list_and_collect_file_details(arg->ctx, arg->fsname,
		    arg->snap, arg->dir, arg->file, arg->howmany, arg->u32,
		    arg->restrictions, &(res->more), &(res->list));
	}
	SAMRPC_SET_RESULT(ret, SAM_FILE_DETAILS, res);

	Trace(TR_DEBUG,
	    "List and collect file details ret [%d] with [%d] status",
	    ret, (res->list != NULL) ? (res->list)->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_collect_file_details_6_svr(
	file_details_arg_t *arg,
	struct svc_req *req	/* ARGSUSED */
)
{
	int ret = -1;
	sqm_lst_t *lst = NULL;

	Trace(TR_DEBUG, "Collect file details");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to get file details");

	ret = collect_file_details(arg->ctx, arg->fsname,
	    arg->snap, arg->dir, arg->files, arg->u32, &lst);

	SAMRPC_SET_RESULT(ret, SAM_STRING_LIST, lst);

	Trace(TR_DEBUG,
	    "Collect file details ret [%d] with [%d] status",
	    ret, (lst != NULL) ? lst->length : -1);
	return (&rpc_result);
}

samrpc_result_t *
samrpc_delete_files_6_svr(
strlst_arg_t *arg,	/* argument to api */
struct svc_req *req	/* ARGSUSED */
)
{

	int ret = -1;
	Trace(TR_DEBUG, "Delete files");

	/* free previous result */
	xdr_free(xdr_samrpc_result_t, (char *)&rpc_result);

	SAMRPC_CHECK_TIMESTAMP(arg->ctx->handle->timestamp);

	Trace(TR_DEBUG, "Calling native library to delete files");
	ret = delete_files(arg->ctx, arg->lst);

	SAMRPC_SET_RESULT(ret, SAM_VOID, 0);

	Trace(TR_DEBUG, "Delete files return[%d]", ret);
	return (&rpc_result);
}
