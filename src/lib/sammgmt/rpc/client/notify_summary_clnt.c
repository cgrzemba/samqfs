
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

#pragma ident	"$Revision: 1.19 $"

#include "mgmt/sammgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"
/*
 * notify_summary_clnt.c
 *
 * RPC client wrapper for notify summary api
 */

/*
 * get_notify_summary
 * retrieve all notification summary info
 */
int
get_notify_summary(
ctx_t *ctx,	/* client connection */
sqm_lst_t **notify	/* return - list of notification summary */
)
{

	int ret_val;
	ctx_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get notification";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(notify)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;

	SAMRPC_CLNT_CALL(samrpc_get_notify_summary, ctx_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*notify = (sqm_lst_t *)result.samrpc_result_u.result.result_data;
	/*
	 * xdr does not preserve the tail of the list
	 * set the tail
	 */
	SET_LIST_TAIL((*notify));

	PTRACE(2, "%s return [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * del_notify_summary
 * delete an entry from a notification file
 */
int
del_notify_summary(
ctx_t *ctx,		/* client communication */
notf_summary_t *notf_summ	/* notification to delete */
)
{

	int ret_val;
	notify_summary_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:delete notification";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(notf_summ)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.notf_summ = notf_summ;

	SAMRPC_CLNT_CALL(samrpc_del_notify_summary, notify_summary_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s return with [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * mod_notify_summary
 * modify an entry from a notification file
 */
int
mod_notify_summary(
ctx_t *ctx,		/* client connection */
uname_t oldemail,	/* old email address */
notf_summary_t *notf_summ	/* notification to modify */
)
{

	int ret_val;
	mod_notify_summary_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:Modify notification";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(oldemail, notf_summ)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	strcpy(arg.oldemail, oldemail);
	arg.notf_summ = notf_summ;

	SAMRPC_CLNT_CALL(samrpc_mod_notify_summary, mod_notify_summary_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s return [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}


/*
 * add_notify_summary
 * add an entry to a notification file
 */
int
add_notify_summary(
ctx_t *ctx,		/* client connection */
notf_summary_t *notf_summ	/* Notification summary to be added */
)
{

	int ret_val;
	notify_summary_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:Add notification";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(notf_summ)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.notf_summ = notf_summ;

	SAMRPC_CLNT_CALL(samrpc_add_notify_summary, notify_summary_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;

	PTRACE(2, "%s return [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}

/*
 * get_email_addrs_by_subj()
 *
 * Returns a comma separated list of email addresses subscribed to
 * a specific notification subject.
 */
int
get_email_addrs_by_subj(
ctx_t *ctx,		/* client connection */
notf_subj_t subj_wanted,
char **addrs		/* return - list of email addresses */
)
{

	int ret_val;
	get_email_addrs_arg_t arg;
	samrpc_result_t result;
	char *func_name = "rpc:get email addrs by subj";
	char *err_msg;
	enum clnt_stat stat;

	PTRACE(2, "%s entry", func_name);

	CHECK_CLIENT_HANDLE(ctx, func_name);
	if (ISNULL(addrs)) {
		PTRACE(2, "%s exit %s", func_name, samerrmsg);
		return (-1);
	}

	PTRACE(3, "%s calling RPC...", func_name);

	memset((char *)&result, 0, sizeof (result));
	arg.ctx = ctx;
	arg.subj_wanted = subj_wanted;

	SAMRPC_CLNT_CALL(samrpc_get_email_addrs_by_subj, get_email_addrs_arg_t);

	CHECK_FUNCTION_FAILURE(result, func_name);

	ret_val = result.status;
	*addrs = (char *)result.samrpc_result_u.result.result_data;

	PTRACE(2, "%s return [%d]...", func_name, ret_val);
	PTRACE(2, "%s exit", func_name);
	return (ret_val);
}
