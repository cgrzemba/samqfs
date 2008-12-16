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
#ifndef _TASK_SCHEDULE_H
#define	_TASK_SCHEDULE_H

#pragma ident	"$Revision: 1.10 $"


#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

/*
 * Returns an array of key value pair strings describing the tasks
 * scheduled in the system.
 *
 * Each key value string may have the following keys:
 * task		= <2 character string taskid>
 * starttime	= <date>,
 * periodicity	= <period>
 * duration	= <duration>
 * id		= <1072 character secondary id used to identify instances of
 *			certain types of task see below>
 *
 * <date> format is: YYYYMMDDhhmm
 *
 * <duration> and <period> are an integer value, followed by a unit among:
 * "s" (seconds)
 * "m" (minutes, 60s)
 * "h" (hours, 3 600s)
 * "d" (days, 86 400s)
 * "w" (weeks, 604 800s)
 * "y" (years, 31 536 000s)
 *
 * In addition the following periodicity units are supported for
 * the mgmt tasks:
 * M (months)
 * D (<n>th day of the month)
 * W (<n>th day of the week - 0 = Sunday, 6 = Saturday)
 *
 *
 * IAS Tasklets Task IDS (Task description):
 * AW (Auto Worm)
 * HC (Hash Computation)
 * HI (Hash Indexing)
 * PA (Periodic Audit)
 * AD (Automatic Deletion)
 * DD (De-Duplication)
 *
 * For all IAS tasklet schedules:
 * The keys task, starttime, periodicity and duration are mandatory.
 * The id key should not be present.
 *
 * SAM/QFS Task IDS (Task Description)
 * RC (Recycler Schedule)
 * SN (Snapshot Schedule will contain additional keys)
 * RP (Report Schedule will contain additional keys)
 *
 * The keys task, starttime and periodicity are mandatory for all
 * SAM/QFS schedules. The key duration is not supported. Additional
 * keys may be required for certain task types including the id field
 * which allows the scheduling of multiple instances of a type of task to
 * operate on different targets.
 */
int get_task_schedules(ctx_t *c, sqm_lst_t **l);

/*
 * Retrieves schedule information for specific task classes and/or
 * ids from the ias repository and our own repository.
 *
 *  Both task and id are optional fields.
 *
 *  If task is NULL and id is NULL, all entries will be returned.
 *  If task is non-NULL and id is NULL, all entries for that task class
 *  are returned.
 *  If task is NULL and id is non-NULL, all entries for any class that
 *  match the id are returned.
 *
 * Returns a list of kv_strings.
 */
int
get_specific_tasks(ctx_t *c, char *task, char *id, sqm_lst_t **l);

/*
 * Set or add the task described in the kv_string
 */
int set_task_schedule(ctx_t *c, char *kv_string);

/*
 * Remove the schedule for the task described in the kv_string.
 * The schedules for the IAS Tasklets cannot be removed.
 *
 * If an id key is present in either the configuration file or
 * the input string its value will be used
 * for the match.
 */
int remove_task_schedule(ctx_t *c, char *kv_pair_str);



/* Structures and Definitions for server side use only */
typedef struct task_sched {
	char task[3];
	char id[1072];		/* long enough for a path and fs name */
	char starttime[16];	/* date */
	char duration[16];	/* duration */
	char periodicity[16];	/* duration */
	char *kv_string;	/* The original kv string */
	void *private;		/* caller-specific extra data area. */
				/* If not NULL, 'private' must be malloced */
				/* because it will be freed when the  */
				/* structure is destroyed */
} task_sched_t;


int validate_task_schedule(task_sched_t *in, boolean_t extended_units);

/*
 * Compares two tasks to see if the task and id fields are equal.
 * Returns 1 if they match 0 if they don't.
 */
int task_matches(task_sched_t *t1, task_sched_t *t2);

/*
 *  Returns a list of task_sched_t structures to the caller.
 *
 *  Both task and id are optional fields.
 *
 */
int get_task_structs(char *task, char *id, sqm_lst_t **tasks);

/*
 * set_crontab_entry()
 *
 * Given a crontab entry or list of entries, adds to the root crontab file.
 * If entry with same command line exists, updates it.  Once file is complete,
 * notifies cron of the update.
 *
 */
int set_crontab_entry(char *croncmd, sqm_lst_t *cronlist);

/*
 * delete_crontab_entry()
 *
 * Given a crontab entry or list of entries, deletes from the root crontab
 * file.
 */
int delete_crontab_entry(char *croncmd, sqm_lst_t *cronlist);

/*
 *  Used when new schedules are added by other functions in this module,
 *  and also by the utility function to generate crontab entries
 *  for handling at kit install/delete time.
 */
int
validate_cron_sched(task_sched_t *in);

#endif	/* _TASK_SCHEDULE_H */
