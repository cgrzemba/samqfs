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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#ifndef	_NOTIFY_SUMMARY_H
#define	_NOTIFY_SUMMARY_H

#pragma ident	"$Revision: 1.24 $"

/*
 * notify_summary.h
 * Header file for notification summary
 * module.
 */



#include <sys/types.h>

#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

#define	SUBJ_MAX	13	/* Total notification subjects */

#define	SCRIPTS_MAX	4	/* Notification scripts: exists in SAM */
/* length of notf. like 'root' or 'admin' */
#define	NOTF_LEN	128

/*
 * This module collects notification
 * summary from only the files mentioned
 * below. The recycler notification
 * particulars on a per set or per
 * vsn basis in either the archiver.cmd
 * or the recycler.cmd specified by
 * -recyc_mailaddr or -mailaddr resp.
 * is NOT handled here.
 * Specific API()s exist for them:
 * get_all_ar_set_copy_params()
 * and get_all_rc_robot_cfg().
 * Those should be called with
 * appropriate params.
 */

#define	DEV_DOWN	SCRIPT_DIR"/dev_down.sh"
#define	ARCH_INTR	SCRIPT_DIR"/archiver.sh"
#define	MED_REQ		SCRIPT_DIR"/load_notify.sh"
#define	RECY_STAT_RC	SCRIPT_DIR"/recycler.sh"
#define	DUMP_INTR	"DumpInterrupted"
#define	FS_FULL		"ENospace"

#define	SAM_EXAMPLES_PATH	OPT_DIR"/examples"

/* the types of notifications */
typedef enum notf_subj {
	DEVICEDOWN,	/* device down: Y/N */
	ARCHINTR,	/* archiver interrupted ? */
	MEDREQD,	/* Media Required */
	RECYSTATRC,	/* Recycler Status: from recycler.sh */
	DUMPINTR,	/* Dump interrupted */
	FSFULL,		/* File system full */
	HWM_EXCEED,	/* HWM exceeded */
	ACSLS_ERR,	/* acsls errors */
	ACSLS_WARN,	/* acsls information */
	DUMP_WARN,	/* dump warnings */
	LONGWAIT_TAPE,	/* outstanding tape request */
	/* Intellistore only */
	FS_INCONSISTENT, /* fs mount required or logical device unavailable */
	SYSTEM_HEALTH,  /* processes using 100% CPU */
} notf_subj_t;


/* Principal structure for notfn. summary */
typedef struct notf_summary {
	char		admin_name[NOTF_LEN]; /* the name */
	char		emailaddr[NOTF_LEN]; /* the email address */
	boolean_t	notf_subj_arr[SUBJ_MAX]; /* an array of events */
} notf_summary_t;


/*
 * retrieve all notification summary info
 *
 * Parameters:
 *	ctx_t *ctx		context object.
 *	list **notify	list of notfn. objects.
 */
int get_notify_summary(ctx_t *ctx, sqm_lst_t **notify);

/*
 * delete an entry given an array of notification events
 *
 * Parameters:
 *	ctx_t *ctx					context object.
 *	notf_summary_t *notf_summ	notf. summary object.
 */
int del_notify_summary(ctx_t *ctx, notf_summary_t *notf_summ);

/*
 * modify an entry given an array of notification events
 *
 * Parameters:
 *	ctx_t *ctx					context object
 *	upath_t oldemail			the old email
 *	notf_summary_t *notf_summ	notf. summary object
 */
int mod_notify_summary(ctx_t *ctx, upath_t oldemail,
    notf_summary_t *notf_summ);

/*
 * add an entry given an array of notification events.
 * add to ONLY those files in which the 'mail' command
 * is uncommented.
 *
 * Parameters:
 *	ctx_t ctx					context object
 *	notf_summary_t *notf_summ	notf. summary object
 */
int add_notify_summary(ctx_t *ctx, notf_summary_t *notf_summ);

int get_addr(char *type, sqm_lst_t **addrs);

/*
 * get_email_addrs_by_subj()
 *
 * Returns a comma separated list of email addresses subscribed to
 * a specific notification subject.
 *
 * Notification subjects are as defined in the enum notf_subj_t
 */
int get_email_addrs_by_subj(ctx_t *clnt, notf_subj_t subj_wanted,
    char **addrs);

#endif	/* _NOTIFY_SUMMARY_H */
