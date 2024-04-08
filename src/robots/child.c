/*
 * child.c - children support for the robot shepherd.
 */

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

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <wait.h>
#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "sam/nl_samfs.h"
#include "aml/fsd.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "robots.h"

#pragma ident "$Revision: 1.22 $"

static char    *_SrcFile = __FILE__;

/* Function prototypes */
static int sam_remote_allowed(robots_t *);

/* globals */
extern int number_robots;
extern shm_alloc_t master_shm, preview_shm;

void
start_children(rob_child_pids_t *pids)
{
	rob_child_pids_t *pid;

	for (pid = pids; pid->who != NULL; pid++) {
		if (sam_remote_allowed(pids->who)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "Start %s(%d).",
				    pid->who->cmd, pid->eq);
			start_a_process(pid);
		}
	}
}

void
kill_children(rob_child_pids_t *pids)
{
	int i;
	rob_child_pids_t *pid;

	/* set so children do not become zombies */
	sigignore(SIGCHLD);

	for (i = 0, pid = pids; i < number_robots; pid++, i++) {
		if ((pid->pid > 0) && (pid->who != NULL))
			/* pseudo device has no equipment */
			if (pid->eq == 0)
				/* the stk ssi needs SIGTERM */
				kill(-(pid->pid), SIGTERM);
			else
				/* no SAM child can ignore SIGINIT */
				kill(pid->pid, SIGINT);
	}
}

/*
 * start a child.  Always pass main shared memory id, preview table shared
 * memory id and equipment number.
 *
 * For the pseudo device(s) the equipment number is the process id of this
 * process.  This is needed by the stk SSI process others are not know at
 * this time..
 */

void
start_a_process(rob_child_pids_t *pid)
{
	char *dc_mess, *execute_path;
	char mshmid[12], pshmid[12], equip[12];
	int fd;

	sprintf(mshmid, "%d", master_shm.shmid);
	sprintf(pshmid, "%d", preview_shm.shmid);
	if (pid->device != NULL) {
		dc_mess = pid->device->dis_mes[DIS_MES_CRIT];
		if (pid->oldstate != pid->device->state) {
			pid->oldstate = pid->device->state;
			if (pid->device->state > DEV_IDLE) {
				sam_syslog(LOG_INFO,
				    catgets(catfd, SET, 9001,
				    "%s(%d): not started: device off or down."),
				    pid->who->cmd, pid->device->eq);
				return;
			}
		} else if (pid->device->state > DEV_IDLE) {
			char *MES_9002 = catgets(catfd, SET, 9002,
			    "%s: not running: device off or down");
			char *mess = (char *)malloc_wait((strlen(MES_9002) +
			    strlen(pid->who->cmd) + 10), 2, 0);

			sprintf(mess, MES_9002, pid->who->cmd);
			memccpy(pid->device->dis_mes[DIS_MES_NORM],
			    mess, '\0', DIS_MES_LEN);
			free(mess);
			if (pid->who &&
			    (pid->who->type == DT_PSEUDO_SC ||
			    pid->who->type == DT_PSEUDO_SS)) {
				pid->restart_time = (time_t)- 1;
				pid->who->auto_restart = NO_RESTART;
			}
			return;
		}
		sprintf(equip, "%d", pid->device->eq);
	} else {
		dc_mess = ((shm_ptr_tbl_t *)
		    master_shm.shared_memory)->dis_mes[DIS_MES_CRIT];
		sprintf(equip, "%d", getpid());
	}

	pid->start_time = time(NULL);
	/*
	 * Set non-standard files to close on exec.
	 */
	for (fd = STDERR_FILENO + 1; fd < OPEN_MAX; fd++) {
		(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
	}
	if ((pid->pid = fork1()) == 0) {
		/* we are the child */
		if (pid->device == NULL)	/* set up a process group */
			setpgrp();

		execute_path = (char *)malloc(MAXPATHLEN);
		sprintf(execute_path, "%s/%s", SAM_EXECUTE_PATH, pid->who->cmd);
		execl(execute_path, pid->who->cmd, &mshmid, &pshmid,
		    &equip, NULL);
		/* Have to just quietly go away, we have closed the syslog fd */
		sprintf(dc_mess, "execl: %s: %s", pid->who->cmd,
		    error_handler(errno));
		_exit(-errno);
	}
	/* Fork failed */
	if (pid->pid < 0)
		sam_syslog(LOG_ALERT, "start_a_process: fork: %m.");
}

/*
 * reap_child - get exit status for a dead child.  Set up restart time.
 */

void
reap_child(rob_child_pids_t *pids)
{
	int what_pid, wait_status, i, got_one = FALSE;
	char *ent_pnt = "reap_child";
	rob_child_pids_t *pid;

	while ((what_pid = waitpid(-1, &wait_status, WNOHANG)) > 0) {
		for (i = 0, pid = pids; i < number_robots; pid++, i++) {
			if ((what_pid == pid->pid) && (pid->who != NULL)) {
				int estat = WEXITSTATUS(wait_status);

				if (WIFEXITED(wait_status)) {
					/*
					 * if the exit status is negative,
					 * sign extend it.
					 */
					if ((estat << (sizeof (int) - 1) * 8) <
					    0) {
						estat |= 0xffffff00;
					}
				}
				pid->status = wait_status;
				if (WIFEXITED(wait_status) && estat < 0) {
					/*
					 * child failed to exec.  Don't
					 * restart it
					 */
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 11048,
					    "%s: %s(%d) exited with status"
					    " %d (%s)"), ent_pnt,
					    pid->who->cmd, pid->device ?
					    pid->device->eq : 0,
					    -estat, strerror(-estat));
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 11049,
					    "%s: %s will not be re-started"),
					    ent_pnt, pid->who->cmd);
					pid->restart_time = (time_t)- 1;
				} else {
					pid->restart_time = time(NULL) + 10;
					sam_syslog(LOG_INFO,
					    "%s: %s(%d): exit status = %#x.",
					    ent_pnt, pid->who->cmd,
					    pid->device ? pid->device->eq : 0,
					    wait_status);
				}
				if (WIFSIGNALED(wait_status) &&
				    WCOREDUMP(wait_status)) {
					char msg[80];

					/*
					 * Child core dumped, call
					 * save_core.sh to save the core file
					 */
					sprintf(msg,
					    "%s %d crashed (core dumped)",
					    pid->who->cmd, pid->device ?
					    pid->device->eq : 0);
					errno = 0;
					if (FsdNotify(SAVE_CORE_SH, LOG_ERR,
					    0, msg) <= 0)
						sam_syslog(LOG_DEBUG,
						    "%s: Notify(): %m",
						    ent_pnt);
				}
				pid->pid = 0;
				got_one = TRUE;
				break;
			}
		}
		if (!got_one && DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "%s: Unknown pid %d, exit status = %#x.",
			    ent_pnt, what_pid, wait_status);
	}

	if (what_pid < 0)
		sam_syslog(LOG_INFO, "reap_child: %m.");
	else if (what_pid == 0 && !got_one && DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s: No children.", ent_pnt);
}

/*
 * check_children - check for dead children and restart the process if its
 * time and we are allowed.
 */

void
check_children(rob_child_pids_t *pids)
{
	int i;
	time_t now;
	rob_child_pids_t *pid;

	time(&now);		/* get the time */
	for (i = 0, pid = pids; i < number_robots; pid++, i++) {
		if (pid->restart_time < 0) {
			continue;
		}
		if ((pid->pid > 0) || (pid->who == NULL))
			continue;

		if ((pid->device != NULL) &&
		    (pid->device->state < pid->oldstate) &&
		    (pid->device->state < DEV_OFF)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "Start %s(%d).",
				    pid->who->cmd, pid->eq);
			start_a_process(pid);
			continue;
		}
		if (((now - pid->restart_time) < 0) || !pid->who->auto_restart)
			continue;	/* not time/allowed to restart */

		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "Restart %s(%d).",
			    pid->who->cmd, pid->eq);
		start_a_process(pid);
	}
}

static int
sam_remote_allowed(robots_t *robot)
{
	dev_ent_t	*device;

	if (robot->type != DT_PSEUDO_SC && robot->type != DT_PSEUDO_SS) {
		return (1);
	}
	device = (dev_ent_t *)SHM_REF_ADDR(((shm_ptr_tbl_t *)
	    master_shm.shared_memory)->first_dev);

	/* if the robot (either sam remote server or sam remote client) is in */
	/* the device table, that means that the license server has allowed   */
	/* the site to run sam remote.  That means that robots should start   */
	/* this command. */
	for (; device != NULL;
	    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
		if (device->equ_type == robot->type) {
			return (1);
		}
	}
	/* don't start it, don't restart it. */
	robot->auto_restart = NO_RESTART;
	return (0);
}
