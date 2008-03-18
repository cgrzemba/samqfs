/*
 *	recycle.h - SAM-FS APIs for recycling operations.
 */

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
#ifndef _RECYCLE_H
#define	_RECYCLE_H

#pragma ident   "$Revision: 1.11 $"		/* tab */


#include "sam/types.h"

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/types.h"

/*
 *	structs
 */


/*
 *	This structure define each media type's no recycle vsn list.
 */
typedef struct no_rc_vsns {
	mtype_t media_type;
	sqm_lst_t  *vsn_exp;
} no_rc_vsns_t;


/*
 *	recycler parameters
 */
typedef struct rc_param {
	int		hwm;	/* % media utilization */
	fsize_t		data_quantity; /* in bytes */
	boolean_t	ignore;
	uname_t	email_addr;  /* address to inform when hwm is exceeded */
	int		mingain;	/* % minimum gain to do recycle */
	int		vsncount;
	uint32_t	change_flag;
	boolean_t	mail;	/* possible pre 4.0 support */
				/* not supported currently */
	int		minobs;	/* only valid in archiver.cmd for disk vols */
} rc_param_t;

#define	RC_hwm		0x00000001
#define	RC_data_quantity 0x00000002
#define	RC_ignore	0x00000004
#define	RC_email_addr	0x00000008
#define	RC_mingain	0x00000010
#define	RC_vsncount	0x00000020
#define	RC_mail		0x00000040
#define	RC_minobs	0x00000080


/*
 *	This structure define each robot's recycler parameters.
 *	rc_param_t is defined in common.h
 */
typedef struct rc_robot_cfg {
	upath_t		robot_name;
	rc_param_t	rc_params;
} rc_robot_cfg_t;


/*
 *	rc_log_opts_t is used to tune the
 *	output to the logfile specified.
 */
typedef struct rc_log_opts_s {

	boolean_t extrap_capacity;	/* include extrapolated capacity */
	boolean_t cat_suppress;		/* suppress catalog section */
	boolean_t cat_vol_suppress;	/* suppress cat section vol listing */
	boolean_t vol_suppress;		/* suppress volume section */
	boolean_t vol_selection_on;	/* msgs why/why not volume selected */
	boolean_t suppress_empty;	/* dont log empty volumes */
	boolean_t show_files_on_vol;    /* display files on selected volume */
	boolean_t show_expired;		/* msg for copies older */
					/*  than tape's label */
	boolean_t ignore_expired;	/* continue when expired */
					/* copies detected */

} rc_log_opts_t;


/*
 *	Functions to get log file location for recycler.
 *
 *	upath_t rc_log -	recycler log path.
 */
int get_rc_log(ctx_t *ctx, upath_t rc_log);


/*
 *	Functions to set log file location for recycler.
 *
 *	upath_t rc_log -	recycler log path.
 */
int set_rc_log(ctx_t *ctx, const upath_t rc_log);


/*
 *	Functions to retrieve the default log file location.
 *
 *	upath_t rc_log -	recycler log path.
 */
int get_default_rc_log(ctx_t *ctx, upath_t rc_log);


/*
 *	Functions to get and set notification scripts for recycler as well as
 *	retrieve the default location of the notification script.
 *
 *	upath_t rc_script -	recycler notify script path.
 */
int get_rc_notify_script(ctx_t *ctx, upath_t rc_script);
int set_rc_notify_script(ctx_t *ctx, const upath_t rc_script);
int get_default_rc_notify_script(ctx_t *ctx, upath_t rc_script);


/*
 * Function to get the list of all VSNs (as specified originally, i.e., regular
 * expression, explicit vsn name etc.) for which no recycle has been set.
 *
 * sqm_lst_t **no_rc_vsns -	reyurned list of no_rc_vsn_t data, it
 *				must be freed by caller.
 */
int get_all_no_rc_vsns(ctx_t *ctx, sqm_lst_t **no_rc_vsns);


/*
 * Function to get the list of all VSNs (as specified originally, i.e., regular
 * expression, explicit vsn name etc.) for which no recycle has been set for
 * a particular type of media.
 * const mtype_t media_type -	media type.
 * no_rc_vsns_t **no_recycle_vsns -	returned no_rc_vsns_t data, it
 *					must be freed by caller.
 */
int get_no_rc_vsns(ctx_t *ctx, const mtype_t media_type,
	no_rc_vsns_t **no_recycle_vsns);


/*
 *	Functions to add, modify or remove VSNs for which no recycle needs
 *	to be enforced for a particular type of media.
 *
 *	no_rc_vsns_t *no_recycle_vsns -	no_rc_vsns_t data needed to be added,
 *				modified, or removed.
 */
int add_no_rc_vsns(ctx_t *ctx, no_rc_vsns_t *no_recycle_vsns);
int modify_no_rc_vsns(ctx_t *ctx, no_rc_vsns_t *no_recycle_vsns);
int remove_no_rc_vsns(ctx_t *ctx, no_rc_vsns_t *no_recycle_vsns);


/*
 *	Function to get the recycle parameters for all the libraries.
 *
 *	sqm_lst_t **rc_robot_cfg - returned list of rc_robot_cfg data,
 *					it must be freed by caller.
 */
int get_all_rc_robot_cfg(ctx_t *ctx, sqm_lst_t **rc_robot_cfg);


/*
 *	Function to get the recycle parameters for a particular library.
 *
 *	const upath_t robot_name -	library name.
 *	rc_robot_cfg_t **rc_robot_cfg -	returned rc_robot_cfg data,
 *					it must be freed by caller.
 */
int get_rc_robot_cfg(ctx_t *ctx, const upath_t robot_name,
	rc_robot_cfg_t **rc_robot_cfg);


/*
 *	Functions to set recycle parameters
 *	for a particular library.
 *
 *	rc_robot_cfg_t *rc_robot_cfg -	rc_robot_cfg_t data need to be set.
 */
int set_rc_robot_cfg(ctx_t *ctx, rc_robot_cfg_t *rc_robot_cfg);


/*
 *	Functions to reset recycle parameters to default value
 *	for a particular library. It means that it will be removed
 *	from the configuration file.
 *
 *	rc_robot_cfg_t *rc_robot_cfg -	rc_robot_cfg_t data need to be reset.
 */
int reset_rc_robot_cfg(ctx_t *ctx, rc_robot_cfg_t *rc_robot_cfg);


/*
 *	Function to get the default recycle parameters.
 *
 *	rc_param_t **rc_params -	returned default rc_params data,
 *					it must be freed by caller.
 */
int get_default_rc_params(ctx_t *ctx, rc_param_t **rc_params);


/*
 * Free methods.
 */

void free_no_rc_vsns(no_rc_vsns_t *vsns);
void free_list_of_no_rc_vsns(sqm_lst_t *vsns_list);
void free_list_of_rc_robot_cfg(sqm_lst_t *robots_list);


#endif	/* _RECYCLE_H */
