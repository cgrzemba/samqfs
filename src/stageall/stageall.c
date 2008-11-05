/*
 * stageall.c - associative staging demon.
 *
 * Perform associative staging.
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

#pragma ident "$Revision: 1.19 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

/* Solaris headers. */
#include <sys/param.h>
#include <sys/wait.h>
#include <libgen.h>
#include <syslog.h>
#include <sys/shm.h>

/* SAM-FS headers. */
#include "pub/lib.h"
#include "pub/stat.h"
#include "sam/defaults.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/names.h"
#include "sam/syscall.h"

/* Macros. */
#define	dsyslog(X)	if (defaults->debug & SAM_DBG_STGALL) sam_syslog X

/* Structures. */
struct stageall_entry {	/* Each entry is a stageall request */
	struct stageall_entry *next;
	int pid;	/* Child pid working on this entry */
	char cwd[1];	/* Current working directory. */
};


/* Private data. */
static sam_defaults_t *defaults;
struct stageall_entry *stageall_ptr = NULL;
static int child_in = 0;
static int child_out = 0;

#define	MAXPIDFINI	100
static struct {
	int pid;	/* Finished child pid */
	int stat;	/* Finished child exit_status */
} child[MAXPIDFINI];

/* Private functions. */
static void process_directory(struct stageall_entry *sp);

/* Signal catching functions. */
static void sig_child(int sig);


int
main(
/* LINTED argument unused in function */
int argc,
char *argv[])
{
	sigset_t set, oset;
	struct sigaction sig_action;
	char cwd[MAXPATHLEN + 1];	/* Current working directory. */

	program_name = argv[0];
	if (strcmp(GetParentName(), SAM_FSD) != 0) {
		fprintf(stderr,
		    "program_name may only be executed from sam-fsd\n");
		exit(EXIT_USAGE);
	}
	if (FindProc(SAM_STAGEALL, "") > 0) {
		error(EXIT_FAILURE, 0, "another stagealld is already running");
	}

	/*
	 * Set process signal handling.
	 * SIGCHLD for death of child processes.
	 * SIGHUP reconfigure.
	 * SIGINT exit.
	 * SIGTERM exit.
	 */
	sigfillset(&set);		/* Block all signals except */
	sigdelset(&set, SIGCHLD);
	sigdelset(&set, SIGHUP);
	sigdelset(&set, SIGINT);
	sigdelset(&set, SIGTERM);
	sigprocmask(SIG_BLOCK, &set, &oset);

	sig_action.sa_handler = SIG_DFL; /* want to restart system calls */
	sigemptyset(&sig_action.sa_mask); /* on excepted signals. */
	sig_action.sa_flags = SA_RESTART;

	sigaction(SIGINT, &sig_action, (struct sigaction *)NULL);
	sigaction(SIGTERM, &sig_action, (struct sigaction *)NULL);

	sig_action.sa_handler = SIG_IGN; /* ignore SIGHUP */
	sigaction(SIGHUP, &sig_action, (struct sigaction *)NULL);

	sig_action.sa_handler = sig_child; /* handler for child termination */
	sigaction(SIGCHLD, &sig_action, (struct sigaction *)NULL);

	defaults = GetDefaults();
	if (setgid(0) == -1) {
		error(2, errno, "setgid(0)");
	}

	for (;;) {
		struct stageall_entry *sp, **prev_sp;
		int pid;

		/*
		 * Wait at root.
		 */
		if (chdir("/") == -1) {
			error(2, errno, "chdir(/)");
			/*NOTREACHED*/
			break;
		}

		/*
		 * Make associative stage system call request.
		 */
		while (sam_syscall(SC_stageall, NULL, 0) < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno != ENOPKG) {
				error(2, errno, "syscall(stageall)");
			}
			sleep(15);
		}

		/*
		 * Current working directory changed to associative
		 * staging directory.
		 */
		getcwd(cwd, sizeof (cwd)-1);
		dsyslog((LOG_WARNING, "Change to directory %s \n", cwd));

		/*
		 * Remove finished entry(s) from the forward linked list.
		 */
		while (child_out != child_in) {
			int found = 0;
			int wait_status;

			pid = child[child_out].pid;
			wait_status = child[child_out].stat;
			child[child_out].pid = 0;
			child[child_out].stat = 0;
			child_out++;
			if (child_out == MAXPIDFINI) {
				child_out = 0;
			}
			prev_sp = &stageall_ptr;
			sp = stageall_ptr;
			while (sp != NULL) {
				if (sp->pid == pid) {
					if (wait_status) {
						error(2, 0,
						    "child %d died with "
						    "exit status = %04x.",
						    pid, wait_status);
					}
					dsyslog((LOG_WARNING,
					    "Remove %s pid %d,"
					    "in=%d, out=%d, %x\n",
					    sp->cwd, pid, child_in,
					    child_out, sp));
					found = 1;
					/* Remove entry */
					*prev_sp = sp->next;
					free(sp);
					break;
				}
				prev_sp = &sp->next;
				sp = sp->next;
			}
			if (found == 0) {
				error(2, 0, "unknown child pid %d, "
				    "exit status = %04x.", pid, wait_status);
				/*NOTREACHED*/
			}
		}

		/*
		 * If duplicate stageall directory request, ignore.
		 * Otherwise put request in forward linked list.
		 */
		prev_sp = &stageall_ptr;
		sp = stageall_ptr;
		while (sp != NULL) {
			if ((strcmp(cwd, sp->cwd)) == 0) {
				/* Duplicate */
				break;
			}
			prev_sp = &sp->next;
			sp = sp->next;
		}
		if (sp) {
			/* Stageall already in progress for this directory */
			dsyslog((LOG_WARNING,
			    "Duplicate %s pid %d, in=%d, out=%d\n",
			    cwd, sp->pid, child_in, child_out));
			continue;
		}
		sp = (struct stageall_entry *)malloc
		    (sizeof (struct stageall_entry) + strlen(cwd));
		*prev_sp = sp;
		if (sp == NULL) {
			error(2, errno, "Unable to malloc()");
			/*NOTREACHED*/
		}
		sp->next = NULL;
		strcpy(sp->cwd, cwd);
		sp->pid = 0;

		/*
		 * Fork child to process associative staging directory.
		 */
		if ((pid = fork1()) > 0) {
			sp->pid = pid;
			dsyslog((LOG_WARNING,
			    "Main %s pid %d, in=%d, out=%d, %x\n",
			    sp->cwd, pid, child_in, child_out, sp));
		} else if (pid == 0) {
			dsyslog((LOG_WARNING,
			    "Child %s pid %d, in=%d, out=%d, %x\n",
			    sp->cwd, pid, child_in, child_out, sp));
			process_directory(sp);	/* The child */
			_exit(0);
		} else {
			error(2, errno, "Unable to fork1()");
			/*NOTREACHED*/
		}
	}
	return (1);
}

/*
 * Process individual stageall directory.
 */
static void
process_directory(
struct stageall_entry *sp)
{
	DIR *dp;
	struct dirent *dirp;
	struct sam_stat sb;

	/*
	 * Open the new directory.
	 * Extend the full path.
	 */
	if ((dp = opendir(".")) == NULL) {
		error(0, errno, "opendir(%s)", sp->cwd);
		return;
	}

	while ((dirp = readdir(dp)) != NULL) {
		char *name = dirp->d_name;

		/* Ignore dot and dot-dot. */
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}
		if (sam_lstat(name, &sb, sizeof (sb)) < 0) {
			error(0, errno, "sam_lstat(%s)", name);
			continue;
		}
		if (SS_ISSTAGE_N(sb.attr) || !SS_ISSTAGE_A(sb.attr)) {
			continue;
		}

		/*
		 * Get the target of the simlimk.
		 */
		if (S_ISLNK(sb.st_mode)) {
			/* Read link buffer */
			char path[MAXPATHLEN + 1];
			int	linklen;

			linklen = readlink(name, path, sizeof (path)-1);
			if (linklen < 0) {
				error(0, errno, "readlink(%s)", name);
				continue;
			}
			path[linklen] = '\0';
			name = path;
			if (sam_lstat(name, &sb, sizeof (sb)) < 0) {
				error(0, errno, "sam_lstat(%s)", name);
				continue;
			}
		}

		/*
		 * Stage only regular files that are offline
		 * and not already staging.
		 */
		if (!S_ISREG(sb.st_mode) || !SS_ISOFFLINE(sb.attr)) {
			continue;
		}
		if (SS_ISSTAGING(sb.flags)) {
			continue;
		}
		dsyslog((LOG_WARNING,
		    "Stage %s/%s pid %d\n", sp->cwd, name, sp->pid));

		/* Stage with no_assoc set */
		if (sam_stage(name, "is") < 0) {
			error(0, errno, "sam_stage(%s)", name);
			continue;
		}
	}
	if (closedir(dp) < 0) {
		error(0, errno, "closedir(%s)", sp->cwd);
	}
}


/*
 * Catch child signal.
 */
static void
/*ARGSUSED0*/
sig_child(
int sig)
{
	int pid;
	int wait_status;

	while ((pid = waitpid(-1, &wait_status, WNOHANG)) > 0) {
		child[child_in].pid = pid;
		child[child_in].stat = wait_status;
		if (++child_in == MAXPIDFINI) {
			child_in = 0;
		}
		/* If child completion table overflows, terminate stageall. */
		if (child_in == child_out) {
			error(2, 0, "Child completion table overflowed");
		}
	}
}
