/*
 *
 * Solaris 2.x Sun Storage & Archiving Management File System
 *
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

#pragma ident "$Revision: 1.29 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* POSIX headers. */
#include <unistd.h>
#include <synch.h>
#include <fcntl.h>
#include <libgen.h>
#include <syslog.h>
#include <wait.h>
#include <sys/param.h>
#include <sys/types.h>

/* SAM-FS headers. */
#include "sam/exit.h"
#include "sam/types.h"
#include "sam/param.h"
#include "sam/lib.h"
#include "sam/names.h"
#include "aml/shm.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/* Local headers. */
#include "amld.h"


void
start_children(
	child_pids_t pids[])
{
	int		i;

	for (i = 0; i < NUMBER_OF_CHILDREN; i++) {
		if (pids[i].who < 0) {
			/* if who is negative, skipit for now */
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG,
				    "Process %s disabled at compile time.",
				    pids[i].cmd);
			}
			continue;
		}
		if (!(pids[i].restart & NO_AUTOSTART)) {
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG, "Starting %s.",
				    pids[i].cmd);
			}
			/* all autostarts use standard args. */
			start_a_process(&pids[i], NULL);
		}
	}
}


void
kill_children(
	child_pids_t pids[])
{
	int i;

	(void) sigignore(SIGCHLD);	/* set so children do not */
	/* become zombies. */
	for (i = 0; i < NUMBER_OF_CHILDREN; i++) {
		if (pids[i].pid > 0) {
			/* no child of SAM can ignore */
			kill(pids[i].pid, SIGINT);
		}
	}
}

/* start a child. */

void
start_a_process(
	child_pids_t *pid,
	char **special)
{
	shm_ptr_tbl_t *shm_ptr_tbl;
	char	*ent_pnt = "start_a_process";
	char	mshmid[12], pshmid[12];
	char	*s_mess;
	char	xpath[MAXPATHLEN];
	char	*execute_path = &xpath[0];
	int		fd;

	s_mess =
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dis_mes[DIS_MES_NORM];
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	sprintf(mshmid, "%d", master_shm.shmid);

	sprintf(pshmid, "%d", preview_shm.shmid);

	pid->start_time = time(NULL);
	sprintf(execute_path, "%s/%s", SAM_EXECUTE_PATH, pid->cmd);

	Trace(TR_MISC, "Starting %s", pid->cmd);

	/* Set non-standard files to close on exec. */
	for (fd = STDERR_FILENO + 1; fd < OPEN_MAX; fd++) {
		(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
	}

	if ((pid->pid = fork1()) == 0) {
		int err;

		/* Child. */
		switch (pid->arguments) {
		case STD_ARGS:
			execl(execute_path, pid->cmd, &mshmid, &pshmid,
			    (char *)NULL);
			err = errno;
			break;

		case SPECIAL_ARGS:
			pid->args = special;
			*special = pid->cmd;	/* set first argument */
			if (special != NULL) {
				(void) execvp(execute_path, pid->args);
				err = errno;
			} else {
				err = EINVAL;
			}
			break;

		default:
			err = EINVAL;
			break;
		}

		/*
		 * All fds have been closed so syslog is not available.
		 * Put a message in the global message area.
		 */
		if (DBG_LVL(SAM_DBG_DEBUG)) {
			char dmy_str[DIS_MES_LEN * 2];

			memset(dmy_str, 0, DIS_MES_LEN * 2);
			sprintf(dmy_str, "%s: exec: %s: %s", ent_pnt,
			    execute_path, StrFromErrno(errno, NULL, 0));
			memccpy(s_mess, dmy_str, '\0', DIS_MES_LEN);
		}
		_exit(-err);
	}
	if (pid->pid < 0) {
		sam_syslog(LOG_ALERT, "%s: fork: %s/%s: %s",
		    ent_pnt, SAM_EXECUTE_PATH,
		    pid->cmd, StrFromErrno(errno, NULL, 0));
		return;
	}
	switch (pid->who) {

	case CHILD_CATSERVER:
		/* Wait for the catalog server to respond. */
		CatalogTerm();	/* In case the library is still in use */
		while (CatalogInit("sam-amld") == -1) {
			int retval;

			sleep(2);
			errno = 0;
			retval = sigsend(P_PID, pid->pid, 0);
			if (retval == -1 || errno == ENOENT) {
				sam_syslog(LOG_ALERT,
				    "Catalog server initialization failed.");
				kill_children(pids);
				exit(2);
			}
		}
		break;

	case CHILD_SCANNER:
		shm_ptr_tbl->scanner = pid->pid;
		break;

	default:
		break;
	}
}


/*
 * reap_child - get exit status for a dead child.
 * Set up restart time
 */
void
reap_child(
	child_pids_t pids[])
{
	shm_ptr_tbl_t *shm_ptr_tbl;
	pid_t	what_pid;
	char	*ent_pnt = "reap_child";
	int		i;
	int		wait_status;

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;

	while ((what_pid = waitpid(-1, &wait_status, WNOHANG)) > 0) {
		for (i = 0; i < NUMBER_OF_CHILDREN; i++) {
			if (what_pid == pids[i].pid) {
				int estat = WEXITSTATUS(wait_status);

				if (WIFEXITED(wait_status)) {
					/* if the exit status is negative, */
					/* sign extend it. */
					if ((estat << (sizeof (int) - 1) * 8) <
					    0) {
						estat |= 0xffffff00;
					}
				}
				pids[i].status = wait_status;
				if (WIFEXITED(wait_status) && estat < 0) {
					/* child failed to exec. Don't */
					/* restart it. */
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 11047,
					    "%s: %s exited with status %d"
					    " (%s)"), ent_pnt, pids[i].cmd,
					    -estat, strerror(-estat));
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 11049,
					    "%s: %s will not be restarted"),
					    ent_pnt, pids[i].cmd);
					pids[i].restart |= NO_RESTART;
				}
				switch (estat) {
				case EXIT_USAGE:
				case EXIT_NORESTART:
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 11049,
					    "%s: %s will not be restarted"),
					    ent_pnt, pids[i].cmd);
					pids[i].restart |= NO_RESTART;
					break;

				case EXIT_NOMEM:
					pids[i].restart_time = time(NULL) + 60;
					break;

				default:
				case 1:
				case EXIT_FATAL:
					pids[i].restart_time = time(NULL) + 10;
					break;
				}

				pids[i].pid = 0;
				switch (pids[i].who) {
				case CHILD_SCANNER:
					shm_ptr_tbl->scanner = 0;
					break;
				}

				sam_syslog(LOG_INFO, "%s: %s, status = %#x",
				    ent_pnt, pids[i].cmd, wait_status);
				break;
			}
		}

		if (i == NUMBER_OF_CHILDREN && DBG_LVL(SAM_DBG_DEBUG)) {
			sam_syslog(LOG_DEBUG,
			    "%s: Unknown pid %d, exit status = %#x",
			    ent_pnt, what_pid, wait_status);
		}
	}

	if (what_pid == -1 && DBG_LVL(SAM_DBG_DEBUG)) {
		sam_syslog(LOG_DEBUG, "%s:whatpid:%s", error_handler(errno));
	}
}


/*
 * check_children - check for dead children and restart the process if
 * it's time and we are allowed.
 */
void
check_children(
	child_pids_t pids[])
{
	time_t	now;
	int		i;

	now = time(NULL);		/* get the time */
	for (i = 0; i < NUMBER_OF_CHILDREN; i++) {
		if ((pids[i].who < 0) || ((pids[i].who > 0) &&
		    (pids[i].pid > 0))) {
			continue;
		}
		if (((now - pids[i].restart_time) < 0) ||
		    (pids[i].restart & NO_RESTART)) {
			continue;	/* not time/allowed to restart */
		}
		if (DBG_LVL(SAM_DBG_DEBUG)) {
			sam_syslog(LOG_DEBUG, "Re-starting process %s.",
			    pids[i].cmd);
		}
		if (!(pids[i].restart & NO_RESTART)) {
			start_a_process(&pids[i], NULL);
		}
	}
}
