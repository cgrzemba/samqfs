
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

#pragma ident	"$Id"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"
/*
 * report_clnt.c
 *
 * RPC client wrapper for report api
 */

/*
 * generate report
 * The amount of information content to be included in the report
 * is input using the report_requirement parameter
 * The results of the report are not instanteous, but the user can
 * access the report using the get_all_reports() api
 */
int
gen_report(
	ctx_t 	*ctx,			/* client connection */
	report_requirement_t 	*req	/* input - content to be included */
)
{
	int ret_val;
	report_requirement_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:generate report";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(req)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.req = req;

	SAMRPC_CLNT_CALL(samrpc_gen_report, report_requirement_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


int
get_file_metrics_report(
	ctx_t 	*ctx,			/* client connection */
	char	*fsname,
	enum_t	which,
	time_t	start,
	time_t	end,
	char	**buf
)
{
	int ret_val;
	file_metric_rpt_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get file metric report";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(fsname, buf)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.str = fsname;
	arg.which = which;
	arg.start = start;
	arg.end = end;

	SAMRPC_CLNT_CALL(samrpc_get_file_metrics_report, file_metric_rpt_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*buf = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s returning with status [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
