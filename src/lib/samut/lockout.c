/*
 * lockout.c - kill off competing processes
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

#pragma ident "$Revision: 1.13 $"

/* ANSI C headers. */
#include <errno.h>
#include <signal.h>

/* POSIX headers. */

#ifdef linux
#include "/usr/include/dirent.h"
#else
#include "dirent.h"
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Solaris headers. */
#include <libgen.h>

#include <strings.h>
#ifdef linux
#include <stdio.h>
#include <string.h>
#endif
#include <stdlib.h>

/* SAM-FS headers. */
#include <sam/lib.h>

/* Local headers. */

/* Private data. */

/* Private functions. */


static int sam_register(char *, char *, pid_t);
static int sam_getlock(char *, char *, char *, pid_t);
static int sam_rellock(char *, char *);
static int sam_killprocs(char *, char *, char *, pid_t, int);

/*
 * sam_lockout(char *name, char *dir, char *prefix, int *siglist)
 *
 * Gain exclusive access to some resource.  If siglist == NULL, then
 * we do not have 'precedence'.  We wait around trying to create a
 * lock file, then see if any other registered, live proc exists in
 * the directory, then register ourself (if no live proc) and then
 * delete the lock file and return.  We return 0 if we gained exclusive
 * access, and > 0 if some other process already exists.
 *
 * If siglist != NULL, we have precedence.  We register first, then
 * try to grab the lockfile.  Once we get it, we look around for
 * other procs, sequentially send them signals from siglist, and
 * eventually remove the lockfile and return.  We return > 0 if
 * there was still a live process out there when we sent our last
 * round of signals.
 *
 * Returns:
 *	  0 - success
 *	< 0 - error
 *	> 0 - other procs exist(ed)
 */

int
sam_lockout(
	char *procname,	/* name of program */
	char *dir,	/* dir of pid registry */
	char *prefix,	/* lock/pid file prefix */
	int *siglist)	/* 0 terminated list of signals (implies precedence) */
{
	int r;
	pid_t pid = getpid();

	if (siglist == NULL) {
		/*
		 * We have no precedence.  We wait, and if some other live proc
		 * exists, we return > 0 (presumably this proc will then exit).
		 * Otherwise, we register and return success (0).
		 */
		if ((r = sam_getlock(procname, dir, prefix, pid)) != 0) {
			return (r);
		}
		if ((r = sam_register(dir, prefix, pid)) == 0) {
			/*
			 * signal = 0 ==> count the number of live procs found.
			 */
			r = sam_killprocs(procname, dir, prefix, pid, 0);
		}
		if (sam_rellock(dir, prefix) < 0) {
			return (-1);
		}
		return (r);
	} else {
		int *sig;

		/*
		 * We have precedence.  We register first, then we wait a bit
		 * trying to get the lock (if necessary), then we take it anyway
		 * (if necessary), kill off anyone else who's running, and then
		 * return success (0, or if we killed someone else, > 0).
		 */
		if (sam_register(dir, prefix, pid) < 0) {
			return (-1);
		}
		if (sam_getlock(procname, dir, prefix, pid) < 0) {
			return (-1);
		}
		for (sig = siglist; *sig != 0; sig++) {
			r = sam_killprocs(procname, dir, prefix, pid, *sig);
			if (r == 0) {		/* nobody left to kill */
				break;
			}
			sleep(2);
		}
		(void) sam_rellock(dir, prefix);
		return (r);
	}
}


/*
 * ----- sam_register(char *dir, char *prefix, pid_t pid)
 *
 * Create the file <dir>/<prefix><pid>.
 *
 * The intent is to 'register' the calling program so that
 * other program instances can find this one readily, either
 * to know that it's running (and relinquish their duties
 * to it) or to kill it.
 */
static int
sam_register(
	char *dir,
	char *prefix,
	pid_t pid)
{
	int fd;
	char path[128];

	(void) snprintf(path, sizeof (path), "%s/%s%d", dir, prefix, pid);
	if ((fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0) {
		return (-1);
	}
	(void) close(fd);
	return (0);
}


#define	LOCK_RETRIES	5
#define	LOCK_PAUSE	2

/*
 * ----- sam_getlock(char *procname, char *dir, char *prefix, pid_t pid)
 *
 * Create the file <dir>/<prefix>"lock" using the O_EXCL flag to
 * obtain exclusive access.  If this fails, then stick around until
 * it's available.  If things take too long, make sure there's
 * actually a program out there that seems to own it.  Otherwise,
 * remove it and continue trying to create it.  Return 0 when it's
 * successfully created.
 */
static int
sam_getlock(
	char *procname,
	char *dir,
	char *prefix,
	pid_t pid)
{
	int fd, tries, procs;
	char path[128];

	tries = 0;
	snprintf(path, sizeof (path), "%s/%s%s", dir, prefix, "lock");

retry:
	if ((fd = open(path, O_EXCL|O_CREAT|O_TRUNC, 0600)) >= 0) {
		close(fd);
		return (0);
	}
	if (errno != EEXIST) {
		return (-1);
	}
	++tries;

	/*
	 * The lock file exists.  Is there a proc to go with it?
	 * The hard part is figuring out if someone now dead left
	 * it around.  No simple, reliable way to do this...
	 */
	procs = sam_killprocs(procname, dir, prefix, pid, 0);
	if (procs != 0) {
		return (procs);
	}
	if (tries < LOCK_RETRIES) {
		sleep(LOCK_PAUSE);
	} else {
		/*
		 * sam_syslog(LOG_INFO,
		 *    "removed dead lock file (%s/%s%s)", dir, prefix, "lock");
		 */
		(void) unlink(path);
		tries = 0;
	}
	goto retry;
}


/*
 * sam_rellock(char *dir, char *prefix)
 *
 * Remove <dir>/<prefix>lock
 */
static int
sam_rellock(
	char *dir,
	char *prefix)
{
	char path[128];

	snprintf(path, sizeof (path), "%s/%s%s", dir, prefix, "lock");
	if (unlink(path) < 0) {
		return (-1);
	}
	return (0);
}


/*
 * sam_killprocs(char *name, char *dir, char *prefix, pid_t pid, int sig)
 *
 * Look through dir for files named <prefix>.*.  Match .* to a pid,
 * and see if the pid exists and its name matches <name>, and it's
 * not this proc.  If so, and sig is non-zero, send it signal sig.
 * Count the total number of such things found (even if sig == 0).
 * If the pid does not exist or if it does exist but the name does
 * not match, remove the pid file.
 *
 * Be careful not to kill any pid < 2, and skip any "lock" file.
 *
 * Return the number of procs found (other than pid <pid>).
 */
static int
sam_killprocs(
	char *name,
	char *dir,
	char *prefix,
	pid_t pid,
	int sig)
{
	pid_t xpid;
	int found, prefixlen = strlen(prefix);
	DIR *dp;
	struct dirent *dirp;
	char path[128], procname[128];

	name = basename(name);

	found = 0;
	if ((dp = opendir(dir)) == NULL) {
		return (-1);
	}
	while ((dirp = readdir(dp)) != NULL) {
		if (strncmp(dirp->d_name, prefix, prefixlen) == 0 &&
		    strcmp(&dirp->d_name[prefixlen], "lock") != 0) {
			int remove = 0;

			xpid = atoi(&dirp->d_name[prefixlen]);
			if (xpid <= 1) {
				remove = 1;
			} else if (xpid != pid) {
				bzero(procname, sizeof (procname));
				(void) GetProcName(xpid, procname,
				    sizeof (procname));
				if (strncmp(procname, name,
				    sizeof (procname)) == 0) {
					++found;
					if (sig && kill(xpid, sig) < 0) {
						if (errno == ESRCH) {
							/* proc gone */
							remove = 1;
						}
					}
				} else {
					remove = 1;
				}
			}
			if (remove) {
				(void) sprintf(path, "%s/%s", dir,
				    (char *)&dirp->d_name);
				(void) unlink(path);
			}
		}
	}
	if (closedir(dp) < 0) {
		return (-1);
	}
	return (found);
}
