
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
#include "pub/mgmt/sammgmt_rpc.h"

/*
 * job_history_clnt.c
 *
 * RPC client wrapper for load api
 */

/*
 * get_jobhist_by_fs
 *
 * Function to get information about all pending load information.
 */
int
get_jobhist_by_fs(
ctx_t *ctx,			/* client connection		*/
uname_t fsname,			/* filesystem name		*/
job_type_t job_type,		/* type of job - Archiver, releaser etc. */
job_hist_t **job_hist		/* return - job history list	*/
)
{

	int ret_val;
	job_hist_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get jobhist by fs";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, job_hist)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strcpy(arg.fsname, fsname);
	arg.job_type = job_type;

	SAMRPC_CLNT_CALL(samrpc_get_jobhist_by_fs, job_hist_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*job_hist = (job_hist_t *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_all_jobhist
 * get list of job history for all fs
 */
int
get_all_jobhist(
ctx_t *ctx,	/* client connection		*/
job_type_t job_type,		/* type of job - Archiver, Releaser etc. */
sqm_lst_t **job_list		/* list of jobtype jobs for all fs */
)
{

	int ret_val;
	job_type_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get all jobhist";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(job_list)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.job_type = job_type;

	SAMRPC_CLNT_CALL(samrpc_get_all_jobhist, job_type_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*job_list = (sqm_lst_t *)result.samrpc_result_u.result.result_data;

	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*job_list));

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
