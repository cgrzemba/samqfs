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
#pragma ident	"$Revision: 1.8 $"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "pub/mgmt/task_schedule.h"
#include "mgmt/config/common.h"

/*
 *  fsmupd
 *
 *  Utility program to facilitate updating File System Manager
 *  components from one version to the next, and perform whatever
 *  post-installation/pre-removal activities are required.
 *
 *  Post-install/pre-remove must take care of any crontab entries
 *  we've added.
 *
 */

/*  Function declarations */
static int post_install_tasks(void);
static int pre_remove_tasks(void);
static int upgrade(void);
static int add_cron_entries(void);
static int remove_cron_entries(void);
static int all_cron_entries(sqm_lst_t **results);

/*
 * horribly, avoiding creating a header file for just 1 function.  If more
 * get added, do the header thing.
 */
extern int csd_to_task(void);

int
main(int argc, char *argv[])
{
	int		st;
	char		c;

	while ((c = getopt(argc, argv, "iru")) != -1) {
		switch (c) {
			case 'i':
				/* post-install */
				st = post_install_tasks();
				break;
			case 'r':
				/* pre-remove */
				st = pre_remove_tasks();
				break;
			case 'u':
				/* upgrade components */
				st = upgrade();
				break;
			default:
				/* ignore invalid args */
				st = -1;
				break;
		}
	}

	return (st);
}

/*
 *  Add calls to post-installation tasks to this function.
 */
int
post_install_tasks(void)
{
	int	st;

	st = add_cron_entries();

	return (st);
}

/*
 *  Add calls to pre-removal tasks to this function
 */
static int
pre_remove_tasks(void)
{
	int	st;

	st = remove_cron_entries();

	return (st);
}

/*
 *  Add component upgrade calls to this function
 */
static int
upgrade(void)
{
	int	st;

	st = csd_to_task();

	return (st);
}

static int
add_cron_entries(void)
{
	int		st;
	sqm_lst_t		*lstp = NULL;

	st = all_cron_entries(&lstp);
	if (st != 0) {
		return (st);
	}

	st = set_crontab_entry(NULL, lstp);
	lst_free_deep(lstp);

	return (st);
}

static int
remove_cron_entries(void)
{
	int		st;
#if 0
	sqm_lst_t		*lstp = NULL;

	/*
	 * we only do SN tasks for the moment, so for remove, we can
	 * take a shortcut and not get all the known tasks. Re-enable
	 * this in the future if required.
	 */
	st = all_cron_entries(&lstp);
	if (st != 0) {
		return (st);
	}

	st = delete_crontab_entry(NULL, lstp);
	lst_free_deep(lstp);
#endif
	st = delete_crontab_entry(SBIN_DIR"/samcrondump", NULL);

	return (st);
}

/*
 * helper for the cron functions.  Generates all the crontab entries to be
 * added or removed.
 */
static int
all_cron_entries(sqm_lst_t **results)
{
	int		st;
	sqm_lst_t		*tlist;
	sqm_lst_t		*lstp;
	node_t		*node;
	task_sched_t	*task;
	char		*task_types[] = {"SN", "RP", NULL};
	int		i;

	lstp = lst_create();
	if (lstp == NULL) {
		return (-1);
	}

	/*
	 * not sure there's a way to determine if a task type
	 * uses cron or not, so for now just get the ones for
	 * snapshots
	 */
	for (i = 0; task_types[i] != NULL; i++) {
		st = get_task_structs(task_types[i], NULL, &tlist);
		if (st != 0) {
			continue;
		}

		for (node = tlist->head; node != NULL; node = node->next) {
			task = node->data;

			if (task == NULL) {
				continue;
			}

			st = validate_cron_sched(task);
			if (st != 0) {
				/* ignore bad entries */
				continue;
			}

			if (task->private != NULL) {
				lst_append(lstp, task->private);
				task->private = NULL;
			}
		}
	}

	*results = lstp;

	return (0);
}
