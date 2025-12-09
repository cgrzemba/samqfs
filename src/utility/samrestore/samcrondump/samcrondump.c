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

#include <stdio.h>
#include <sys/syslog.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <fnmatch.h>
#include <sys/vfstab.h>
#include <dirent.h>

#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "mgmt/restore_int.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/fsmdb_api.h"
#include "pub/mgmt/task_schedule.h"
#include "pub/mgmt/restore.h"
#include "mgmt/util.h"
#include "aml/shm.h"

extern int samcftime(char *buf, const char *format, const time_t *t);

/* globals needed by libsamfs.so */
shm_alloc_t              master_shm, preview_shm;

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* until the init function gets put into a header */
extern void init_utility_funcs(void);

#define	BUFFSIZ		MAXPATHLEN
#define	NUM_BENCHMARK	3	/* number of benchmark dumps */
#define	WEEK		(DAY * 7)

/* structure for holding snapshot statistics */
typedef struct {
	int64_t snapdate;
	int64_t snaptime;
	uint64_t snapsize;
	uint64_t numfiles;
	char	sep;
} snapstat_t;

/* Various paths of files */
#define	RotateLog	"/opt/SUNWsamfs/examples/log_rotate.sh"
#define	SamFSDump	"/opt/SUNWsamfs/sbin/samfsdump"
#define	Compress	"/usr/bin/compress"
#define	Gzip		"/usr/bin/gzip"
#if 0
#define SamFSDumpOpt	"%s -xTf %s "
#else
#define SamFSDumpOpt	"%s -Tf %s "
#endif

/* Forward references */
static int check_fs(snapsched_t *sched);
static int dump_fs(char *taskid, snapsched_t *sched);
static int do_fork(snapsched_t *sched, char *cmd);

static int
is_dump_inconsistent(
	snapstat_t oldstats[],
	snapstat_t newstat,
	time_t *timediff,
	uint64_t *sizediff,
        unsigned int limitPercent);

static void finish_dump(char *snapname, FILE *dumplog, int statfd);
static int get_lock(char *lkfile, int *lockfd);
static void purge_snapshots(snapsched_t *sched, time_t intime, FILE *dumplog);
static void log_event(FILE *dumplog, char *msg);

/* globals */
static time_t		now;
static char		*timefmt = "%e %b %Y %T %Z";
static uint32_t		action_flag = NOTIFY_AS_FAULT | NOTIFY_AS_EMAIL;

extern char *optarg;

/*
 * samcrondump - program to be invoked by cron, takes an "id" comprised
 * of the filesystem name and optional a directory at which to start.
 * The start directory is relative to the point path and will be resolved
 * in the course of things.
 */
int
main(int argc, char *argv[])
{
	char		msgbuf[MAX_MSGBUF_SIZE];
	int		rval;
	char		c;
	char		*taskid = NULL;
	char		*schedstr = NULL;
	sqm_lst_t	*lstp = NULL;
	snapsched_t	sched;
	boolean_t	live = B_FALSE;

	now = time(NULL);	/* Get moment command was invoked */

	/* Init library functions for Tracing, Error handling and Messages */
	init_utility_funcs();

	while ((rval = getopt(argc, argv, "Lf:")) != -1) {
		c = (char) rval;
		switch (c) {
			case 'f':
				taskid = optarg;
				break;
			case 'L':
				/* walk a live filesystem */
				live = B_TRUE;
			default:
				break;
		}
	}

	if (taskid == NULL) {
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_CANT_TAKE_DUMP), "<empty>");
		(void) strlcat(msgbuf, ":  Usage: samcrondump -f fsname[/path]",
		    sizeof (msgbuf));
		(void) fprintf(stderr, msgbuf);
		(void) fflush(stderr);
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_CANT_TAKE_DUMP, LOG_ERR, msgbuf, action_flag);
		return (1);
	}

	if (!live) {
		rval = get_specific_tasks(NULL, "SN", taskid, &lstp);

		if ((rval != 0) || (lstp->length == 0)) {
			/* Nothing to do */
			if (lstp != NULL) {
				lst_free_deep(lstp);
			}
			return (0);
		}
	}

	if (live) {
		char	mountpt[MAXPATHLEN + 1];

		rval = getfsmountpt(taskid, mountpt, sizeof (mountpt));
		if (rval == 0) {
			rval = import_from_mountedfs(taskid, mountpt, NULL);
		}

		lst_free_deep(lstp);

		return (rval);
	}

	/* samfsdump snapshot processing */

	schedstr = (char *)lstp->head->data;
	lst_free(lstp);

	rval = parse_snapsched((char *)lstp->head->data, &sched,
	    sizeof (snapsched_t));

	/* done with this, successful or not */
	free(schedstr);

	if (rval != 0) {
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_CANT_TAKE_DUMP), taskid);
		(void) strlcat(msgbuf, ": Schedule is corrupt",
		    sizeof (msgbuf));
		(void) fprintf(stderr, msgbuf);
		(void) fflush(stderr);
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_CANT_TAKE_DUMP, LOG_ERR, msgbuf, action_flag);
		return (1);
	}

	rval = check_fs(&sched); /* Dump this fs if necessary */
	if (rval == 0) {
		rval = dump_fs(taskid, &sched);
	}

	return (rval);
}

/*
 * dump_fs - perform a samfsdump on a file system.
 */

static int
dump_fs(char *taskid, snapsched_t *sched)
{
	int		rval;
	char		msgbuf[MAX_MSGBUF_SIZE];
	struct stat64	statbuf;
	char		stampnm[BUFFSIZ] = {0};
	char		snapname[BUFFSIZ];
	int		statfd = -1;
	FILE		*dumplog = stdout;
	char		filnam[BUFFSIZ];
	int		i;
	snapstat_t	snapstats[NUM_BENCHMARK] = {0};
	time_t		startsnap;
	time_t		snaptime;
	char		forkcmd[(MAXPATHLEN*11) + 41];
	float		snaphours;
	uint32_t	interval = 0;
	char		unit;

	/*
	 * verify snapshot storage directory exists.  Create it if
	 * not.
	 */
	rval = stat64(sched->location, &statbuf);
	if ((rval == 0) && (!S_ISDIR(statbuf.st_mode))) {
		/* exists, but isn't a directory */
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_DUMP_FAILED_NODIR),
		    taskid, sched->location);
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_DUMP_FAILED_NODIR, LOG_ERR,
		    msgbuf, action_flag);
		return (-1);
	}

	if (rval != 0) {
		/* try to create the target directory */
		rval = mkdirp(sched->location, 0750);

		if (rval != 0) {
			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_DUMP_FAILED_NODIR),
			    taskid, sched->location);
			PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
			    SE_DUMP_FAILED_NODIR, LOG_ERR,
			    msgbuf, action_flag);
			return (0);
		}
	}

	/*
	 * Build pathname for the validation stats & lock file
	 * Allow for multiple dump schedules pointing to the same
	 * directory.
	 *
	 * This has a problem if multiple schedules for the same filesystem
	 * use the same snapshot directory.  It's perfectly legal to schedule
	 * it like that, so need to come up with something more than just fsname
	 * as a semaphore.  Maybe change '/' to '_' from startDir.
	 */
	(void) snprintf(stampnm, BUFFSIZ, "%s/.%s", sched->location,
	    sched->id.fsname);

	/* Create filename for dump */
	(void) samcftime(snapname, sched->namefmt, &now);

	/*
	 * This file is a semaphore to prevent multiple
	 * concurrent dumps, and also stores statistical information
	 * for the last three good dumps for validation purposes.
	 */
	rval = get_lock(stampnm, &statfd);
	if (rval == -2) {
		Trace(TR_MISC, "%s metadata snapshot already in progress.\n",
		    taskid);
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_DUMP_SNAPBUSY), taskid);
		PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
		    SE_DUMP_FAILED_NODIR, LOG_WARNING, msgbuf, action_flag);
		finish_dump(snapname, dumplog, statfd);
		return (rval);
	} else if (rval == -1) {
		Trace(TR_MISC, "Could not open %s snapshot lock.\n", stampnm);
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_CANT_READ_FILE), stampnm);
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_DUMP_FAILED_NODIR, LOG_ERR, msgbuf, action_flag);
		finish_dump(snapname, dumplog, statfd);
		return (rval);
	}

	/* Before performing the dump, open the log file if specified */
	if (sched->logfile[0] != '\0') {
		rval = stat64(sched->logfile, &statbuf);

		/* restart log file if it's too big */
		if ((rval == 0) && (statbuf.st_size > (1024 * 1024 * 20))) {
			(void) snprintf(forkcmd, sizeof (forkcmd), "%s %s",
			    RotateLog, sched->logfile);
			/* don't fail if the log rotate script failed */
			(void) do_fork(sched, forkcmd);
		}

		dumplog = fopen64(sched->logfile, "a");
		if (dumplog == NULL) {
			dumplog = stdout;
			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_CANT_OPEN_FLOG), sched->logfile);
			PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
			    SE_CANT_OPEN_FLOG, LOG_WARNING,
			    msgbuf, action_flag);
			log_event(dumplog, msgbuf);

			/* clear logfile so do_fork() won't try to use it */
			sched->logfile[0] = '\0';
		}
	}

	/* print a start datestamp into the log file */
	(void) snprintf(msgbuf, sizeof (msgbuf),
	    GetCustMsg(SE_SNAPSHOT_STARTED), snapname);
	log_event(dumplog, msgbuf);

	/* Check to make sure no snap by this name exists yet */

	rval = get_snap_name(sched->location, snapname, filnam, NULL, NULL,
	    NULL);

	/* successful return means a snapshot already exists */
	if (rval == 0) {
		Trace(TR_MISC, "Dump file already exists %s", filnam);
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_CANT_TAKE_DUMP), filnam);
		(void) strlcat(msgbuf, GetCustMsg(SE_DUMP_DUMPEXISTS),
		    sizeof (msgbuf));
		log_event(dumplog, msgbuf);
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_CANT_TAKE_DUMP, LOG_ERR,
		    msgbuf, action_flag);
		finish_dump(snapname, dumplog, statfd);
		return (-1);
	}

	/* Create filename of dump we will take */
	(void) snprintf(filnam, BUFFSIZ, "%s/%s.dmp", sched->location,
	    snapname);

	/*
	 * Execute prescript for anything that has to be done first.
	 * Pass the new dump's filename as an argument in case it cares.
	 */
	if (sched->prescript[0] != '\0') {
		/* check if prescript exists and is executable */
		if ((stat64(sched->prescript, &statbuf)) != 0 ||
		    (statbuf.st_size == 0)) {

			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_DUMP_PRESCRIPT_NOTFOUND),
			    snapname, sched->prescript);
			log_event(dumplog, msgbuf);

			if (!sched->prescriptFatal) {
				PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
				    SE_DUMP_PRESCRIPT_NOTFOUND, LOG_WARNING,
				    msgbuf, action_flag);
			} else {
				PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
				    SE_DUMP_PRESCRIPT_NOTFOUND, LOG_ERR,
				    msgbuf, action_flag);
				(void) snprintf(msgbuf, sizeof (msgbuf),
				    GetCustMsg(SE_CANT_TAKE_DUMP), filnam);
				log_event(dumplog, msgbuf);
				finish_dump(snapname, dumplog, statfd);
				return (-1);
			}
		}

		(void) snprintf(msgbuf, sizeof (msgbuf), "Prescript %s started",
		    sched->prescript);
		log_event(dumplog, msgbuf);

		(void) snprintf(forkcmd, sizeof (forkcmd), "%s %s",
		    sched->prescript, filnam);
		rval = do_fork(sched, forkcmd);

		if (rval) {
			Trace(TR_MISC, "Prescript failed error %d.\n", rval);
			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_DUMP_PRESCRIPT_FAILED),
			    snapname, sched->prescript);
			log_event(dumplog, msgbuf);
			if (!sched->prescriptFatal) {
				PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
				    SE_DUMP_PRESCRIPT_FAILED, LOG_WARNING,
				    msgbuf, action_flag);
			} else {
				PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
				    SE_DUMP_PRESCRIPT_FAILED, LOG_ERR,
				    msgbuf, action_flag);
				finish_dump(snapname, dumplog, statfd);
				return (-1);
			}
		}

		(void) snprintf(msgbuf, sizeof (msgbuf),
		    "Prescript %s complete", sched->prescript);
		log_event(dumplog, msgbuf);

	}

	/* Maximum of 10 excluded directories allowed for samfsdump */
	(void) snprintf(forkcmd, sizeof (forkcmd), SamFSDumpOpt, SamFSDump,
	    filnam);
	for (i = 0; i < 10; i++) {
		if (sched->excludeDirs[i][0] == '\0') {
			break;
		}
		(void) strlcat(forkcmd, "-X ", sizeof (forkcmd));
		(void) strlcat(forkcmd, sched->excludeDirs[i],
		    sizeof (forkcmd));
		(void) strlcat(forkcmd, " ", sizeof (forkcmd));
	}
	(void) strlcat(forkcmd, ".", sizeof (forkcmd));

	log_event(dumplog, "samfsdump started");

	startsnap = time(NULL);

	rval = do_fork(sched, forkcmd);

	snaptime = (time(NULL) - startsnap);
	snaphours = (float)snaptime/HOUR;

	if (rval) {
		char tmpbuf[MAX_MSGBUF_SIZE];

		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_CANT_TAKE_DUMP), snapname);

		log_event(dumplog, msgbuf);

		/*
		 * point the user to where to find more info on the
		 * failure in the fault log.
		 */
		if (dumplog != stdout) {
			(void) snprintf(tmpbuf, sizeof (tmpbuf),
			    GetCustMsg(SE_DUMP_MOREINFO),
			    sched->logfile);
		} else {
			(void) snprintf(tmpbuf, sizeof (tmpbuf),
			    GetCustMsg(SE_DUMP_MOREINFO),
			    GetCustMsg(SE_DUMP_ROOTMAIL));
		}
		(void) strlcat(msgbuf, tmpbuf, sizeof (msgbuf));
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_CANT_TAKE_DUMP, LOG_ERR, msgbuf, action_flag);
		finish_dump(snapname, dumplog, statfd);
		return (-1);
	}

	log_event(dumplog, "samfsdump complete");

	/* If file size is 0, flag as err */
	if ((stat64(filnam, &statbuf) != 0) || (statbuf.st_size == 0)) {
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_DUMP_SIZE_0), snapname);
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_DUMP_SIZE_0, LOG_ERR, msgbuf, action_flag);
		log_event(dumplog, msgbuf);
		finish_dump(snapname, dumplog, statfd);
		return (0);
	}

	/*
	 *  Now that we've got the dump file, we can update the stats
	 *  and release the lock for this snapshot schedule.  Prevents
	 *  long-running compression and indexing from blocking the next
	 *  scheduled dump.
	 */

	if (statfd > 0) {
		int		j;
		size_t		sz = sizeof (snapstat_t);
		ssize_t		rsz;
		snapstat_t	newstat;
		time_t		tdiff;
		uint64_t	sdiff;
		float		diffhours;
		char		szbuf[128];
		char		diffszbuf[128];

		newstat.snapdate = now;
		newstat.snaptime = snaptime;
		newstat.snapsize = (uint64_t) statbuf.st_size;
		newstat.sep = '\n';
		/* TODO:  get number of files in snapshot */
		newstat.numfiles = 0;

		(void) lseek64(statfd, 1, SEEK_SET);

		/* look for previous entries */
		for (j = 0; j < NUM_BENCHMARK; j++) {
			rsz = read(statfd, &snapstats[j], sz);
			if (rsz < sz) {
				/* invalid entry */
				(void) memset(&snapstats[j], 0, sz);
				break;
			}
		}

		rval = is_dump_inconsistent(snapstats, newstat, &tdiff, &sdiff, sched->diffLimit);
		switch (rval) {
			case 1:
				diffhours = (float)tdiff/HOUR;

				(void) snprintf(msgbuf, sizeof (msgbuf),
				    GetCustMsg(SE_DUMP_TIME_INCONSISTENT),
				    snapname, snaphours, 3,
				    taskid, diffhours);
				PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
				    SE_DUMP_TIME_INCONSISTENT,
				    LOG_WARNING, msgbuf, action_flag);
				log_event(dumplog, msgbuf);
				break;
			case 2:
				match_fsize(newstat.snapsize, 2, FALSE,
				    szbuf, sizeof (szbuf));
				match_fsize(sdiff, 2, FALSE, diffszbuf,
				    sizeof (diffszbuf));

				(void) snprintf(msgbuf, sizeof (msgbuf),
				    GetCustMsg(SE_DUMP_SIZE_INCONSISTENT),
				    snapname, szbuf, 3,
				    taskid, diffszbuf);
				PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
				    SE_DUMP_SIZE_INCONSISTENT,
				    LOG_WARNING, msgbuf, action_flag);
				log_event(dumplog, msgbuf);
				break;
			default:
				break;
		}

		/* write out the updated stats */
		(void) lseek64(statfd, 1, SEEK_SET);
		(void) ftruncate64(statfd, 1);

		/* if we've already saved 3, remove the oldest (first). */
		if (j == NUM_BENCHMARK) {
			j = 1;
		} else {
			j = 0;
		}

		for (; j < NUM_BENCHMARK; j++) {
			if (snapstats[j].snapdate > 0) {
				(void) write(statfd, &snapstats[j], sz);
			}
		}
		(void) write(statfd, &newstat, sz);

		/* Finally, unlock */
		(void) close(statfd);
		statfd = -1;
	}

	/*
	 * If the dump took longer than the scheduled interval, issue
	 * a warning.
	 */
	translate_period(sched->periodicity, EXTENDED_PERIOD_UNITS,
	    &interval, &unit);

	if (interval > 0) {
		time_t		secs = 0;

		/* don't bother checking if interval more than days */
		if (unit == 's') {
			secs = interval;
		} else if (unit == 'm') {
			secs = interval * 60;
		} else if (unit == 'h') {
			secs = interval * HOUR;
		} else if (unit == 'd') {
			secs = interval * DAY;
		}

		if ((secs > 0) && (snaptime > secs)) {
			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_DUMP_INTERVAL_TOO_SHORT),
			    snapname, snaphours, secs/HOUR);
			PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
			    SE_DUMP_INTERVAL_TOO_SHORT,
			    LOG_WARNING, msgbuf, action_flag);
			log_event(dumplog, msgbuf);
		}
	}

	/*
	 * If dump should be indexed, do it before compressing because
	 * it'll be faster, and if the customer has selected .Z compression
	 * we can't read the file directly to index it.
	 * Do not index unless we've been told to -- consumes too many
	 * system resources to just make an assumption here.
	 */

	if (sched->autoindex == 1) {
		uint64_t	t;

		log_event(dumplog, "Indexing started");

		rval = import_from_samfsdump(sched->id.fsname, filnam, &t);

		if (rval) {
			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_DUMP_INDEX_FAILED),
			    snapname);
			PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
			    SE_DUMP_INDEX_FAILED, LOG_WARNING,
			    msgbuf, action_flag);
			log_event(dumplog, msgbuf);
			finish_dump(snapname, dumplog, statfd);
			return (rval);
		}
		log_event(dumplog, "Indexing complete");
	}

	/* Compress dump if requested */
	forkcmd[0] = '\0';

	if (sched->compress == 2) {
		(void) strcpy(forkcmd, Compress);
	} else if (sched->compress == 1) {
		(void) strcpy(forkcmd, Gzip);
	}

	if (forkcmd[0] != '\0') {
		log_event(dumplog, "Compression started");

		(void) strlcat(forkcmd, " ", sizeof (forkcmd));
		(void) strlcat(forkcmd, filnam, sizeof (forkcmd));
		rval = do_fork(sched, forkcmd);

		if (rval) {
			Trace(TR_MISC, "Failure to compress %s, code %d\n",
			    filnam, rval);
			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_DUMP_COMPRESS_FAILED),
			    snapname);
			PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
			    SE_DUMP_COMPRESS_FAILED,
			    LOG_WARNING, msgbuf, action_flag);
			log_event(dumplog, msgbuf);
		} else {
			log_event(dumplog, "Compression complete");
		}
	}

	/*
	 * Execute postscript, for anything that has to be done afterwards.
	 * Ignore errors in postscript, dump has already been taken.
	 * Pass the new dump's filename as an argument in case it cares.
	 */
	if (sched->postscript[0] != '\0') {
		/* check if postscript exists */
		if ((stat64(sched->postscript, &statbuf)) != 0 ||
		    (statbuf.st_size == 0)) {

			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_DUMP_POSTSCRIPT_NOTFOUND),
			    snapname, sched->postscript);
			PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
			    SE_DUMP_POSTSCRIPT_NOTFOUND, LOG_WARNING,
			    msgbuf, action_flag);
		} else {
			(void) snprintf(msgbuf, sizeof (msgbuf),
			    "Postscript %s started", sched->postscript);
			log_event(dumplog, msgbuf);

			(void) snprintf(forkcmd, sizeof (forkcmd), "%s %s",
			    sched->postscript, filnam);
			rval = do_fork(sched, forkcmd);

			if (rval) {
				(void) snprintf(msgbuf, sizeof (msgbuf),
				    GetCustMsg(SE_DUMP_POSTSCRIPT_FAILED),
				    snapname, sched->postscript);
				log_event(dumplog, msgbuf);
				PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
				    SE_DUMP_POSTSCRIPT_FAILED, LOG_WARNING,
				    msgbuf, action_flag);
			} else {
				(void) snprintf(msgbuf, sizeof (msgbuf),
				    "Postscript %s complete",
				    sched->postscript);
				log_event(dumplog, msgbuf);
			}
		}
	}

	/*
	 * Lastly, see if a retention schedule is set and purge out
	 * dumps that have aged out of the system.
	 */
	purge_snapshots(sched, now, dumplog);

	finish_dump(snapname, dumplog, statfd);
	return (0);
}


/*
 * check_fs -	Called to validate that we should do something now and
 * 		set any params not already in the snapshot schedule
 */

static int
check_fs(snapsched_t	*sched)
{
	char		mountpt[BUFFSIZ];
	int		rval;
	char		msgbuf[MAX_MSGBUF_SIZE];
	struct vfstab	vfsent;

	/*
	 * Ensure file system should be dumped periodically
	 * and that the schedule has not been disabled.
	 */
	if (sched->disabled) {
		return (-1);
	}

	/* Get current mount point of fs */
	rval = getfsmountpt(sched->id.fsname, mountpt, sizeof (mountpt));
	if (rval) {
		/*
		 * check to see if it's even in vfstab.  if not,
		 * don't issue an error, must be a dangling crontab entry
		 */
		FILE	*fp;

		fp = fopen("/etc/vfstab", "r");
		if (fp == NULL) {
			return (-1);
		}

		rval = getvfsspec(fp, &vfsent, sched->id.fsname);
		if (rval == 0) {	/* defined, but not mounted */
			Trace(TR_MISC,
			    "File system %s not mounted, not dumped\n",
			    sched->id.fsname);

			(void) snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_DUMP_FS_NOT_MOUNTED),
			    sched->id.fsname);
			PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
			    SE_DUMP_FS_NOT_MOUNTED,
			    LOG_ERR, msgbuf,
			    NOTIFY_AS_FAULT | NOTIFY_AS_EMAIL);

		}
		/* either way, we can't proceed */
		(void) fclose(fp);
		return (-1);
	}

	/* append the requested starting point, if specified */
	if (*sched->id.startDir != '\0') {
		(void) strlcat(mountpt, "/", sizeof (mountpt));
		(void) strlcat(mountpt, sched->id.startDir, sizeof (mountpt));
	}

	/* set startDir to the fully-qualified path */
	(void) strlcpy(sched->id.startDir, mountpt,
	    sizeof (sched->id.startDir));

	return (0);
}

/*
 * Execute a command in a fork and wait for it to finish.
 */

int
do_fork(snapsched_t *sched, char *cmd)
{
	pid_t	pid;
	int	rval;
	int	status;
	char	msgbuf[MAX_MSGBUF_SIZE];
	int	saverr;

	if ((pid = fork1()) == 0) {

		/*
		 * Child process - do samfsdump here.
		 */

		if (sched->logfile[0] != '\0') {
			(void) freopen(sched->logfile, "a", stdout);
			(void) freopen(sched->logfile, "a", stderr);
		}

		/* close open fds > stderr */
		closefrom(3);

		/*
		 * some of the commands we exec - samfsdump in
		 * particular - require that the current working
		 * directory be at the root of the directory to
		 * be dumped.
		 */
		(void) chdir(sched->id.startDir);

		rval = execl("/bin/sh", "sh", "-c", cmd, NULL);
		saverr = errno;

		/* If we get here, exec failed. */
		Trace(TR_MISC, "Exec of %s failed, reason %s\n", cmd,
		    strerror(saverr));
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_DUMP_BADEXEC), cmd, strerror(saverr));
		(void) fprintf(stderr, "%s\n", msgbuf);
		(void) fflush(stderr);
		_exit(rval);	/* Finish fork */

	} else if (pid == -1) {

		/*
		 * Error, no process created
		 */
		Trace(TR_MISC, "Cannot create fork for %s\n", cmd);
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_FORK_EXEC_FAILED), cmd);
		(void) fprintf(stderr, "%s\n", msgbuf);
		(void) fflush(stderr);
		return (-1);

	} else {
		/*
		 * Parent process. Wait for command to finish.
		 */

		while (waitpid(pid, &status, 0) >= 0)
			(void) sleep(1);
		return (status);
	} /* NOTREACHED */
}


/*
 * is_dump_inconsistent -
 *
 * Check the stats from the previous 3 dumps against the
 * current dump.
 *
 * Returns:
 *	0	Dump makes sense
 *	1	Dump time differs by greater than 10 percent.
 *	2	Dump size differs by greater than 10 percent.
 *
 *  TODO:  Add number of files to dumpstats and use for comparison
 */
static int
is_dump_inconsistent(
	snapstat_t oldstats[],
	snapstat_t newstat,
	time_t *timediff,
	uint64_t *sizediff,
	unsigned int diffPercent)
{
	int	 i;
	time_t timetotal = 0;
	uint64_t sizetotal = 0;
	int64_t avg;
	uint64_t maxdiff;
        float diffLimit = (float)diffPercent/100;

	/* if we don't have enough to compare with, assume ok */
	if (oldstats[NUM_BENCHMARK-1].snapdate == 0) {
		return (0);
	}

	for (i = 0; i < NUM_BENCHMARK; i++) {
		timetotal += oldstats[i].snaptime;
		sizetotal += oldstats[i].snapsize;
	}

	Trace(TR_MISC, "Use diff limit %f\n", diffLimit);
	if (timetotal > 0) {
		avg = timetotal/NUM_BENCHMARK;
/* LINTED */
		maxdiff = (uint64_t)((float)avg * diffLimit);

		*timediff = labs(avg - newstat.snaptime);

		/*
		 * don't return an alert if the difference is less
		 * than 5 minutes.  Precludes notification messages
		 * from being sent far too frequently on very fast
		 * snapshots.
		 */
		if ((*timediff > 300) && (*timediff > maxdiff)) {
			return (1);
		}
	}

	if (sizetotal > 0) {
		avg = (int64_t)sizetotal/NUM_BENCHMARK;
/* LINTED */
		maxdiff = (uint64_t)((float)avg * diffLimit);

		*sizediff = (uint64_t)labs(avg - (int64_t)newstat.snapsize);

		if (*sizediff > maxdiff) {
			return (2);
		}
	}

	return (0);
}

/*
 * Function to close out the dump in progress on error or success.
 * Called only from dump_fs().
 */
static void
finish_dump(char *snapname, FILE *dumplog, int statfd)
{

	char		msgbuf[MAX_MSGBUF_SIZE];

	/* print a done datestamp into the log file */
	if (dumplog != NULL) {
		(void) snprintf(msgbuf, sizeof (msgbuf),
		    GetCustMsg(SE_SNAPSHOT_COMPLETE), snapname);
		log_event(dumplog, msgbuf);

		if (dumplog != stdout) {
			(void) fclose(dumplog);
		}
	}

	if (statfd > 0) {
		(void) close(statfd);
	}
}

static int
get_lock(char *lkfile, int *lockfd)
{
	struct flock64	lock = {0};
	struct flock64	pending_lock = {0};
	int		st;
	int		trylockfd;

	/*
	 * Try to get permission to start the dump.
	 * If byte 1 is locked, another process is already
	 * dumping this file system/directory.  Try locking
	 * byte 0, and wait to acquire byte 1 (allowing 1
	 * dump process to wait).  If byte 0 is also locked,
	 * post an error and abort this dump request.
	 */

	if ((lkfile == NULL) || (lockfd == NULL)) {
		return (-1);
	}

	trylockfd = open64(lkfile, O_RDWR|O_CREAT, 0700);

	if (trylockfd == -1) {
		return (-1);
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 1;
	lock.l_len = 1;

	st = fcntl(trylockfd, F_SETLK64, &lock);

	if (st != 0) {
		/* try the pending lock */
		pending_lock.l_type = F_WRLCK;
		pending_lock.l_whence = SEEK_SET;
		pending_lock.l_start = 0;
		pending_lock.l_len = 1;

		st = fcntl(trylockfd, F_SETLK64, &pending_lock);
		if (st != 0) {
			/* someone's already waiting, fail */
			(void) close(trylockfd);
			return (-2);
		}

		/* wait for goahead lock to be released */
		st = fcntl(trylockfd, F_SETLKW64, &lock);
		if (st != 0) {
			/* failed?  Should never happen */
			(void) close(trylockfd);
			return (-2);
		}

		/* release the pending lock */
		pending_lock.l_type = F_UNLCK;
		(void) fcntl(trylockfd, F_SETLK64, &pending_lock);
	}

	*lockfd = trylockfd;
	return (0);
}

static void
purge_snapshots(
	snapsched_t	*sched,
	time_t		intime,
	FILE		*dumplog)
{
	struct tm		snap_tm;
	struct tm		in_tm;
	time_t			comptime;
	time_t			snaptime;
	char			msgbuf[MAX_MSGBUF_SIZE];
	int			st;
	int			keepVal = 0;
	char			unit;
	char			*ptr;
	sqm_lst_t			*snaplist;
	node_t			*node;
	char			*snapname;
	char			*snappath;

	st = translate_period(sched->duration, EXTENDED_PERIOD_UNITS,
	    (uint32_t*)&keepVal, &unit);

	if ((sched->namefmt[0] == '\0') || (st != 0) || (keepVal == 0)) {
		/* invalid format, duration or unset, do no harm */
		return;
	}

	/* determine the time to compare against */
	(void) localtime_r(&intime, &in_tm);
	comptime = intime;

	switch (unit) {
		case 's':
			/* Seconds */
			comptime -= keepVal;
			break;
		case 'm':
			/* Minutes */
			comptime -= (keepVal * 60);
			break;
		case 'h':
			/* Hours */
			comptime -= (keepVal * HOUR);
			break;
		case 'd':
			/* Days */
			comptime -= (keepVal * DAY);
			break;
		case 'w':
			/* Weeks */
			comptime -= (keepVal * WEEK);
			break;
		case 'M':
			/* Months */
			in_tm.tm_mon -= keepVal;
			if (in_tm.tm_mon < 0) {
				in_tm.tm_year--;
				in_tm.tm_mon += 12;
			}
			comptime = mktime(&in_tm);
			break;
		case 'y':
			/* Years */
			in_tm.tm_year -= keepVal;
			comptime = mktime(&in_tm);
			break;
		default:
			/* can't happen */
			return;
			break;		/* NOTREACHED */
	}

	/*
	 *  Get the list of snapshots -- including those with only indexes -
	 *  and request they be deleted.  Note that delete_dump() will exclude
	 *  "locked" snapshots - those with <foo>.lk.dmp extensions.
	 */
	st = list_dumps_by_dir(NULL, sched->id.fsname, sched->location,
	    &snaplist);

	if (st != 0) {
		/* samerrmsg already set */
		(void) fprintf(dumplog, "%s\n", samerrmsg);
		(void) fflush(dumplog);
		return;
	}

	for (node = snaplist->head; node != NULL; node = node->next) {
		snappath = node->data;
		if (snappath == NULL) {
			continue;
		}

		/* need only the base name for strptime() */
		snapname = strrchr(snappath, '/');
		if (snapname == NULL) {
			snapname = snappath;
		} else {
			snapname++;
		}

		(void) memset(&snap_tm, 0, sizeof (struct tm));

		ptr = strptime(snapname, sched->namefmt, &snap_tm);
		if (ptr == NULL) {
			/* failed to get the date out of the filename */
			continue;
		}

		snaptime = mktime(&snap_tm);
		if (comptime > snaptime) {
			st = delete_dump(NULL, sched->id.fsname, snappath);

			if (st == 0) {
				(void) snprintf(msgbuf, sizeof (msgbuf),
				    GetCustMsg(SE_SNAP_PURGED),
				    snapname);
				(void) fprintf(dumplog, "%s\n", msgbuf);
				(void) fflush(dumplog);
			}
		}
	}

	lst_free_deep(snaplist);
}

/* Function to log a datestamped entry into the log */
static void
log_event(FILE *dumplog, char *msg)
{
	char		timbuf[BUFFSIZ];
	time_t		logtime;

	logtime = time(NULL);
	(void) samcftime(timbuf, timefmt, &logtime);
	(void) fprintf(dumplog, "%s  %s\n", timbuf, msg);
	(void) fflush(dumplog);
}
