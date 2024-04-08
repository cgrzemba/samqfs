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
#ifndef _CFG_ARCHIVER_FIELDS_H
#define	_CFG_ARCHIVER_FIELDS_H

#pragma ident	"$Revision: 1.21 $"

#include <limits.h>
#include <sys/types.h>

#include "sam/types.h"
#include "sam/setfield.h"

static int params_tbl0;
static struct fieldInt params_tbl1 = { 0, 0, LLONG_MAX, 0 };
static struct fieldInt params_tbl2 = { 16, 2, 8192, 0 };
static struct fieldInt params_tbl_drivemax = { 0, 0, LLONG_MAX, 0 };
static struct fieldInt params_tbl3 = { 0, 0, LLONG_MAX, 0 };
static struct fieldInt params_tbl4 = { 1, 1, SHRT_MAX, 0 };
static struct fieldInt params_tbl5 = { 0, 0, LLONG_MAX, 0 };
static struct fieldEnum params_tbl6 = { "none", Joins };
static struct fieldFlag params_tbl7 = { B_TRUE, "off", "on", "off" };
static struct fieldEnum params_tbl8 = { "none", OfflineCopies };
static struct fieldInt params_tbl9 = { 0, 0, LLONG_MAX, 0 };
static struct fieldInt params_tbl10 = { 0, 0, INT_MAX, 0 };
static struct fieldFunc params_tbl11 = { NULL, NULL, paramsReserveSet,
		paramsReserveTostr };
static struct fieldEnum params_tblRsort = { "none", Sorts };
static struct fieldEnum params_tbl12 = { "path", Sorts };
static struct fieldInt params_tbl13 = { 0, 0, INT_MAX, 0 };
static struct fieldInt params_tbl14 = { 0, 0, INT_MAX, 0 };
static struct fieldInt params_tbl15 = { 0, 0, LLONG_MAX, 0 };
static struct fieldFlag params_tbl16 = { B_TRUE, "off", "on", "off" };
static struct fieldFlag params_tbl17 = { B_TRUE, "off", "on", "off" };
static struct fieldFlag params_tbl_unarchage = { B_TRUE, "access",
		"modify", "access" };
static struct fieldFunc params_tbl18 = { NULL, NULL, paramsPrioritySet,
		paramsPriorityTostr };
static struct fieldFlag params_tbl_directio = { B_TRUE, "on", "on", "off" };

static int params_tbl19;
static struct fieldInt params_tbl20 = { 0, 0, LLONG_MAX, 0 };
static struct fieldInt params_tbl21 = { 0, 0, 100, 0 };
static struct fieldFlag params_tbl22 = { B_TRUE, "off", "on", "off" };
static struct fieldString params_tbl23 = { "", 32 };
static struct fieldInt params_tbl24 = { 0, 0, 100, 0 };
static struct fieldInt params_tbl25 = { 0, 0, INT_MAX, 0 };
static struct fieldInt params_tbl26 = { 0, 0, 100, 0 };
static struct fieldInt params_tbl_rearch_cpy = { 0, 1, 4, 0 };
static struct fieldInt params_tbl_queue_time_limit
			= { 24*60*60, 0, INT_MAX, 0 };

static struct fieldVals params_tbl[] = {
	{ "", DEFBITS, offsetof(struct ar_set_copy_params,
		change_flag), &params_tbl0,  },
	{ "archmax", FSIZE, offsetof(struct ar_set_copy_params,
		archmax), &params_tbl1, AR_PARAM_archmax },
	{ "bufsize", INT, offsetof(struct ar_set_copy_params,
		bufsize), &params_tbl2, AR_PARAM_bufsize },
	{ "drivemax", FSIZE, offsetof(struct ar_set_copy_params,
		drivemax), &params_tbl_drivemax, AR_PARAM_drivemax },
	{ "drivemin", FSIZE, offsetof(struct ar_set_copy_params,
		drivemin), &params_tbl3, AR_PARAM_drivemin },
	{ "drives", INT, offsetof(struct ar_set_copy_params, drives),
		&params_tbl4, AR_PARAM_drives },
	{ "fillvsns", FSIZE, offsetof(struct ar_set_copy_params,
		fillvsns_min), &params_tbl5, AR_PARAM_fillvsns },
	{ "join", ENUM, offsetof(struct ar_set_copy_params, join),
		&params_tbl6, AR_PARAM_join},
	{ "lock", SETFLAG, offsetof(struct ar_set_copy_params, buflock),
		&params_tbl7, AR_PARAM_buflock},
	{ "offline_copy", ENUM, offsetof(struct ar_set_copy_params,
		offline_copy), &params_tbl8, AR_PARAM_offline_copy },
	{ "ovflmin", FSIZE, offsetof(struct ar_set_copy_params, ovflmin),
		&params_tbl9, AR_PARAM_ovflmin },
	{ "simdelay", INTERVAL, offsetof(struct ar_set_copy_params, simdelay),
		&params_tbl10, AR_PARAM_simdelay },
	{ "reserve", FUNC | NO_DUP_CHK, offsetof(struct ar_set_copy_params,
		reserve), &params_tbl11, AR_PARAM_reserve },
	{ "rsort", ENUM, offsetof(struct ar_set_copy_params, rsort),
		&params_tblRsort, AR_PARAM_rsort },
	{ "sort", ENUM, offsetof(struct ar_set_copy_params, sort),
		&params_tbl12, AR_PARAM_sort },
	{ "startage", INTERVAL, offsetof(struct ar_set_copy_params, startage),
		&params_tbl13, AR_PARAM_startage },
	{ "startcount", INT, offsetof(struct ar_set_copy_params, startcount),
		&params_tbl14, AR_PARAM_startcount },
	{ "startsize", FSIZE, offsetof(struct ar_set_copy_params, startsize),
		&params_tbl15, AR_PARAM_startsize },
	{ "tapenonstop", SETFLAG, offsetof(struct ar_set_copy_params,
		tapenonstop), &params_tbl16, AR_PARAM_tapenonstop },
	{ "tstovfl", SETFLAG, offsetof(struct ar_set_copy_params,
		tstovfl), &params_tbl17, AR_PARAM_tstovfl },
	{ "unarchage", FLAG, offsetof(struct ar_set_copy_params, unarchage),
		&params_tbl_unarchage, AR_PARAM_unarchage},
	{ "priority", FUNC | NO_DUP_CHK, offsetof(struct ar_set_copy_params,
		priority_lst), &params_tbl18, },
	{ "directio", FLAG, offsetof(struct ar_set_copy_params, directio),
		&params_tbl_directio, AR_PARAM_directio},
	{ "rearch_stage_copy", INT16, offsetof(struct ar_set_copy_params,
		rearch_stage_copy), &params_tbl_rearch_cpy,
		AR_PARAM_rearch_stage_copy},
	{ "queue_time_limit", INTERVAL, offsetof(struct ar_set_copy_params,
		queue_time_limit), &params_tbl_queue_time_limit,
		AR_PARAM_queue_time_limit},


	{ "", DEFBITS, offsetof(struct ar_set_copy_params,
		recycle) +  offsetof(struct rc_param, change_flag),
		&params_tbl19,  },
	{ "recycle_dataquantity", FSIZE, offsetof(struct ar_set_copy_params,
		recycle) +  offsetof(struct rc_param, data_quantity),
		&params_tbl20, RC_data_quantity},
	{ "recycle_hwm", INT, offsetof(struct ar_set_copy_params, recycle) +
		offsetof(struct rc_param, hwm), &params_tbl21,
		RC_hwm },
	{ "recycle_ignore", SETFLAG, offsetof(struct ar_set_copy_params,
		recycle) + offsetof(struct rc_param, ignore),
		&params_tbl22, RC_ignore},
	{ "recycle_mailaddr", STRING, offsetof(struct ar_set_copy_params,
		recycle) + offsetof(struct rc_param, email_addr),
		&params_tbl23, RC_email_addr },
	{ "recycle_mingain", INT, offsetof(struct ar_set_copy_params, recycle)
		+ offsetof(struct rc_param, mingain), &params_tbl24,
		RC_mingain },
	{ "recycle_vsncount", INT, offsetof(struct ar_set_copy_params,
		recycle) + offsetof(struct rc_param, vsncount),
		&params_tbl25, RC_vsncount },
	{ "recycle_minobs", INT, offsetof(struct ar_set_copy_params, recycle) +
		offsetof(struct rc_param, minobs), &params_tbl26,
		RC_minobs },
	{ NULL }};


static int criteria_tbl0;
static struct fieldString cr_policy = { "", 32 };
static struct fieldString cr_path = { "", 128 };
static struct fieldFunc criteria_tbl1 = { NULL, NULL, propGroupSet,
	propGroupTostr };
static struct fieldFunc criteria_tbl2 = { NULL, NULL, propNameSet,
	propNameTostr };
static struct fieldInt criteria_tbl3 = { 0, 0, LLONG_MAX, 0 };
static struct fieldInt criteria_tbl4 = { 0, 0, LLONG_MAX, 0 };
static struct fieldFunc criteria_tbl5 = { NULL, NULL, propUserSet,
	propUserTostr };
static struct fieldInt criteria_tbl6 = { 0, 0, INT_MAX, 0 };
static struct fieldFlag criteria_tbl7 = { (uint32_t)FP_nftv, "off",
	"on", "off" };
static struct fieldString criteria_tbl8 = { "", 32 };


static struct fieldVals criteria_tbl[] = {
{ "", DEFBITS, offsetof(struct ar_set_criteria, change_flag),
	&criteria_tbl0,  },
{ "policy", STRING, offsetof(struct ar_set_criteria,
	set_name), &cr_policy, },
{ "dir", STRING, offsetof(struct ar_set_criteria,
	path), &cr_path, AR_ST_path},
{ "group", FUNC, offsetof(struct ar_set_criteria, group),
	&criteria_tbl1, AR_ST_group },
{ "name", FUNC, offsetof(struct ar_set_criteria, name),
	&criteria_tbl2, AR_ST_name },
{ "maxsize", FSIZE, offsetof(struct ar_set_criteria, maxsize),
	&criteria_tbl3, AR_ST_maxsize },
{ "minsize", FSIZE, offsetof(struct ar_set_criteria, minsize),
	&criteria_tbl4, AR_ST_minsize },
{ "user", FUNC, offsetof(struct ar_set_criteria, user),
	&criteria_tbl5, AR_ST_user },
{ "access", INTERVAL, offsetof(struct ar_set_criteria, access),
	&criteria_tbl6, AR_ST_access },
{ "nftv", SETFLAG, offsetof(struct ar_set_criteria,
	nftv), &criteria_tbl7, AR_ST_nftv },
{ "after", STRING, offsetof(struct ar_set_criteria,
	after), &criteria_tbl8, AR_ST_after },
{ NULL }};


#endif /* _CFG_ARCHIVER_FIELDS_H */
