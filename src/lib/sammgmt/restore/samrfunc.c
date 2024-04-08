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
#pragma ident	"$Revision: 1.51 $"

#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <pthread.h>
#include <libgen.h>
#include <errno.h>
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/process_job.h"
#include "pub/mgmt/restore.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "pub/mgmt/fsmdb_api.h"
#include "pub/mgmt/task_schedule.h"

#define	BUFFSIZ		MAXPATHLEN + 1
#define	CSDBUFFSIZ	sizeof (csdbuf_t) + BUFFSIZ
#define	CRONDUMPLOC	SBIN_DIR"/samcrondump"
#define	SAMFSDUMPLOC	SBIN_DIR"/samfsdump"
#define	FIXDUMPLOC	SBIN_DIR"/samcronfix"
#define	CSDHOUR		3600

static char *comp_exts[3] = {
	".gz",
	".Z",
	NULL
};

/* function prototypes */
static int
do_cleanup_dump(char *fsname, char *dumppath, boolean_t delete_all);
static int
snapsched_to_CSDstring(snapsched_t *sched, char *buf, size_t buflen);
static int qsort_descending(const void *in_a, const void *in_b);
extern samcftime(char *, const char *, const time_t *);

/* Static datastructures */

sqm_lst_t *search_results = NULL; /* Results of a search before retrieval */
int search_active = 0;		/* Flag indicating search-in-progress */

static int
set_snap_lock_state(char *snapname, boolean_t lock);

/*
 * Below restore_xxx fields can only be modified by the restore thread,
 * and cleared when search_active is zero.
 */
uint64_t restore_max;		/* Total number of files to restore */
uint64_t restore_cur;		/* Current number of files restored */

pthread_attr_t           samr_pth_attr;
pthread_mutex_t search_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* Macro to propagate and return error value */
#define	RVAL(X) {rval = X; if (rval) return (rval); }

/*
 *  CSD setup - main routine
 *  This function is TEMPORARY until the GUI changes over to use the
 *  new task schedule for snapshot schedules.  Until that time, we need
 *  to maintain the key/value pairs to/from the GUI from the old CSD struct.
 *
 *  And yes, this looks remarkably like the version in update_restore.c,
 *  but that version will persist, and this one will be deleted.
 */
int
set_csd_params(
	ctx_t *c,		/* ARGSUSED */
	char *fsname,
	char *parameters)
{
	int		st;
	csdbuf_t	csd;
	snapsched_t	sched;
	char		buf[CSDBUFFSIZ];

	if (ISNULL(fsname, parameters)) {
		return (-1);
	}

	memset(&csd, 0, sizeof (csdbuf_t));
	st = parse_csdstr(parameters, &csd, sizeof (csdbuf_t));
	if (st != 0) {
		return (-1);
	}

	/* translate to new structure */
	memset(&sched, 0, sizeof (snapsched_t));
	strlcpy(sched.id.fsname, fsname, sizeof (sched.id.fsname));
	strlcpy(sched.location, csd.location, sizeof (sched.location));
	strlcpy(sched.namefmt, csd.names, sizeof (sched.namefmt));
	strlcpy(sched.prescript, csd.prescript, sizeof (sched.prescript));
	strlcpy(sched.postscript, csd.postscript, sizeof (sched.postscript));
	strlcpy(sched.logfile, csd.logfile, sizeof (sched.logfile));
	sched.autoindex = csd.autoindex;
	sched.disabled = csd.disabled;
	sched.prescriptFatal = csd.prescrfatal;

	memcpy(&(sched.excludeDirs), &(csd.excludedirs),
	    sizeof (sched.excludeDirs));

	if (strncmp(csd.compress, "gzip", 4) == 0) {
		sched.compress = 1;
	} else if (strcmp(csd.compress, "compress") == 0) {
		sched.compress = 2;
	}

	samcftime(sched.starttime, "%Y%m%d%H%M", &(csd.frequency.start));

	snprintf(sched.periodicity, sizeof (sched.periodicity),
	    "%ds", csd.frequency.interval);

	if (csd.retainfor.val != 0) {
		if (csd.retainfor.unit == 'D') {
			csd.retainfor.unit = 'd';
		} else if (csd.retainfor.unit == 'W') {
			csd.retainfor.unit = 'w';
		} else if (csd.retainfor.unit == 'Y') {
			csd.retainfor.unit = 'y';
		}
		snprintf(sched.duration, sizeof (sched.duration),
		    "%d%c", csd.retainfor.val, csd.retainfor.unit);
	}

	st = snapsched_to_string(&sched, buf, CSDBUFFSIZ);
	if (st != 0) {
		return (-1);
	}

	st = set_task_schedule(NULL, buf);

	return (st);
}

/*
 * As soon as the GUI switches to use task schedules directly, this
 * function can be removed.
 */
int
get_csd_params(ctx_t *c, char *fsname, char **params)	/* ARGSUSED */
{
	int		st;
	sqm_lst_t		*lstp;
	char		buffer[CSDBUFFSIZ];
	snapsched_t	sched;
	char		*schedstr = NULL;

	if (ISNULL(fsname, params)) {
		return (-1);
	}

	st = get_specific_tasks(NULL, "SN", fsname, &lstp);
	if (st != 0) {
		return (-1);
	}

	if (lstp->length != 0) {
		schedstr = (char *)lstp->head->data;
	}

	lst_free(lstp);

	if (schedstr == NULL) {
		*params = copystr("");
		if (*params != NULL) {
			return (0);
		} else {
			return (-1);
		}
	}

	/* parse function */
	st = parse_snapsched(schedstr, &sched, sizeof (snapsched_t));

	if (st == 0) {
		st = snapsched_to_CSDstring(&sched, buffer, CSDBUFFSIZ);
	}

	if (st == 0) {
		*params = copystr(buffer);
		if (*params == NULL) {
			st = -1;
		}
	}

	free(schedstr);
	return (st);
}

/* Dump information retrieval */

/*  OBSOLETE:  list_dumps has been deprecated in favor of list_dumps_by_dir() */
int
list_dumps(ctx_t *c, char *fsname, sqm_lst_t **dumps) /* ARGSUSED */
{
	int	rval;

	rval = list_dumps_by_dir(c, fsname, NULL, dumps);

	return (rval);
}

/* Dump information retrieval from a specified directory */
int
list_dumps_by_dir(
	ctx_t *c, 	/* ARGSUSED */
	char *fsname,
	char *usepath,
	sqm_lst_t **dumps)
{
	int		rval;
	char		dumpdirname[BUFFSIZ];
	char		*pathp;
	node_t		*node;
	node_t		*next;
	snapspec_t	*snapArray = NULL;
	uint32_t	count = 0;
	int		i;
	char		*ptr;
	size_t		len;
	size_t		slen;

	if (ISNULL(fsname, dumps)) {
		return (-1);		/* samerr already set */
	}

	if (usepath == NULL) {
		RVAL(getdumpdir(fsname, dumpdirname));	/* Get CSD directory */
		pathp = dumpdirname;
	} else {
		pathp = usepath;
	}

	len = strlen(pathp);

	*dumps = lst_create();	/* Return results in this list */
	if (*dumps == NULL) {
		return (-1);	/* If allocation failed, samerr is set */
	}

	/* see if the database knows about any snaps */
	get_snapshot_status(fsname, &snapArray, &count);

	for (i = 0; i < count; i++) {
		/* only want to include indexed snaps */
		if (snapArray[i].snapState != INDEXED) {
			continue;
		}

		/* skip empty names and "live" metrics-only snaps */
		if ((snapArray[i].snapname[0] == '\0') ||
		    (snapArray[i].snapname[0] != '/')) {
			continue;
		}

		/* check that this is in the specified directory */
		ptr = strrchr(snapArray[i].snapname, '/');
		if ((ptr == NULL) || (ptr == snapArray[i].snapname)) {
			/* can't happen? */
			continue;
		}

		slen = ptr - snapArray[i].snapname;

		if ((slen != len) ||
		    (strncmp(snapArray[i].snapname, pathp, len) != 0)) {
			continue;
		}

		ptr = strdup(snapArray[i].snapname);
		if (ptr == NULL) {
			setsamerr(SE_NO_MEM);
			free(snapArray);
			lst_free_deep(*dumps);
			*dumps = NULL;

			return (-1);
		}

		if (lst_append(*dumps, ptr) != 0) {
			free(ptr);
			lst_free_deep(*dumps);
			*dumps = NULL;
			return (-1);
		}
	}

	/* Recursively walk the dump directory */
	rval = walk_dumps(pathp, "", dumps);

	/*
	 * error is only fatal here if there are no indexed snaps.  Allow
	 * user to delete indexes if the original snap dir was deleted.
	 */
	if ((rval) && (count == 0)) {
		lst_free_deep(*dumps);
		*dumps = NULL;
		return (rval);
	}

	lst_qsort(*dumps, node_cmp);

	/* make sure there are no duplicate entries */
	node = (*dumps)->head;

	while (node != NULL) {
		next = node->next;

		if (next == NULL) {
			/* done */
			break;
		}

		if ((node_cmp(&node, &next)) == 0) {
			/*
			 * there may be more than 1 duplicate of an entry,
			 * so leave node set to the original, and do the
			 * comparison with the new "next" entry.
			 */
			node->next = node->next->next;
			if (node->next == NULL) {
				(*dumps)->tail = node;
			}
			free(next->data);
			free(next);
			(*dumps)->length--;
		} else {
			node = node->next;
		}
	}

	return (rval);
}

/* Get details about dumps. OBSOLETE -- Use get_dump_status_by_dir() instead */
int
get_dump_status(
	ctx_t *c,		/* ARGSUSED */
	char *fsname,
	sqm_lst_t *dumps,
	sqm_lst_t **status)
{
	int rval;

	rval = get_dump_status_by_dir(c, fsname, NULL, dumps, status);

	return (rval);
}

/* Get details about dumps in a specified directory */
int
get_dump_status_by_dir(
	ctx_t *c,		/* ARGSUSED */
	char *fsname,
	char *usepath,
	sqm_lst_t *dumps,
	sqm_lst_t **status)
{
	int		rval = 0;
	char		snapdir[BUFFSIZ];
	char		*pathp;
	node_t		*dump;
	char		details[BUFFSIZ];
	struct stat64	sout;
	snapspec_t	*snapArray = NULL;
	uint32_t	count = 0;
	char		*in_name;
	char		namebuf[BUFFSIZ];
	char		*filstat;
	int		locked = 0;
	int		compressed = 0;
	int		indexed = 0;
	int		broken;
	int		snapindex;
	size_t		len;
	int		i;
	char		*ptr;
	char		*details_fmt = "status=%s,size=%lld,created=%lu,"
	"modified=%lu,locked=%d,indexed=%d,compressed=%d";

	if (ISNULL(fsname, dumps, status)) {
		return (-1);
	}

	if ((usepath == NULL) || (usepath[0] == '\0')) {
		/* get the directory specified in the schedule */
		RVAL(getdumpdir(fsname, snapdir));
		pathp = snapdir;
	} else {
		pathp = usepath;
	}

	*status = lst_create();	/* Return results in this list */
	if (*status == NULL) {
		return (-1);	/* If allocation failed, samerr is set */
	}

	/*
	 * Contact the database to see if any snaps are indexed
	 * Failure here is ok, means no indices are available at the moment
	 */
	get_snapshot_status(fsname, &snapArray, &count);

	/* Walk through list of dumps */
	for (dump = dumps->head; dump != NULL; dump = dump->next) {
		if (dump->data == NULL) {
			continue;
		}

		in_name = (char *)dump->data;
		broken = 0;
		indexed = 0;
		snapindex = -1;
		memset(&sout, 0, sizeof (struct stat64));

		/* Get details about each of four potential dump files */
		rval = get_snap_name(pathp, in_name, namebuf, &locked,
		    &compressed, &sout);

		/*
		 * keep going even if the file is not found.  It may still
		 * be indexed, and we should report those.
		 */
		if (rval != 0) {
			broken = 1;
		}

		/* Have details, start generating information */

		for (i = 0; i < count; i++) {
			if (strcmp(in_name, snapArray[i].snapname) == 0) {
				if (snapArray[i].snapState == INDEXED) {
					snapindex = i;
					indexed = 1;
				}
				break;
			}
		}

		if (compressed > 0) {
			filstat = "compressed";
		}

		/*
		 * gzipped and uncompressed snapshots can be restored.  Those
		 * compressed with "compress" (.Z) need to be decompressed
		 * before use.
		 */
		if (broken) {
			filstat = "broken";
		} else if ((compressed != 2) && (indexed == 1)) {
			filstat = "available";
		} else {
			/* Find out if an index/decompress is in progress */
			if (decomfind(namebuf)) {
				filstat = "indexing";
			} else {
				filstat = "unindexed";
			}
		}

		/* Generate key/value string about the snapshot */
		snprintf(details, BUFFSIZ, details_fmt, filstat, sout.st_size,
		    (snapindex == -1) ?
		    sout.st_ctim.tv_sec : snapArray[snapindex].snaptime,
		    (snapindex == -1) ?
		    sout.st_mtim.tv_sec : snapArray[snapindex].snaptime,
		    locked, indexed, compressed);

		if (snapindex != -1) {
			len = strlen(details);

			snprintf(details + len, BUFFSIZ - len,
			    ",numnodes=%lld", snapArray[snapindex].numEntries);
		}

		ptr = copystr(details);
		if (ptr == NULL) {
			rval = -1;
			break;
		}
		rval = lst_append(*status, ptr);
		if (rval != 0) {
			free(ptr);
			break;
		}
	}

	if (snapArray != NULL) {
		free(snapArray);
	}

	if (rval != 0) {
		lst_free_deep(*status);
		*status = NULL;
	}

	return (rval);
}


/* Take a samfsdump now. GUI interface into samfsdump, no more. */

int
take_dump(ctx_t *c, char *fsname, char *dumppath, char **jobid)	/* ARGSUSED */
{
	char mountpt[BUFFSIZ];
	char path[BUFFSIZ];
	char cmd[BUFFSIZ];
	int rval;
	argbuf_t *arg;
	pid_t pid;
	int status;
	struct stat64 sout;

	snprintf(path, BUFFSIZ, "%s.dmp", dumppath);

	/* Make sure destination does not already exist */
	memset(&sout, 0, sizeof (sout));
	rval = stat64(path, &sout);
	if (rval == 0)
		return (samrerr(SE_FILE_ALREADY_EXISTS, path));

	RVAL(getfsmountpt(fsname, mountpt, sizeof (mountpt)));

	arg = mallocer(sizeof (dumpbuf_t));
	if (arg == NULL)
		return (-1);	/* samerr already set */
	strlcpy(arg->u.fsname, fsname, MAXNAMELEN);
	strlcpy(arg->u.dumpname, path, MAXPATHLEN);

	/* set up the command for exec */
	snprintf(cmd, BUFFSIZ,
	    "cd %s ; %s -f %s . >>%s 2>&1",
	    mountpt, SAMFSDUMPLOC, path, RESTORELOG);

	/* Create an entry in the jobs database */
	rval = start_activity(dumplist, kill_fork, SAMRTHR_U, arg, jobid);
	if (rval != 0) {
		/* free stuff */
		free(arg);
		return (-1);
	}

	pid = exec_get_output(cmd, NULL, NULL);
	if (pid < 0) {
		end_this_activity(*jobid);
		return (-1);
	}

	set_pid_or_tid(*jobid, pid, 0);

	/*
	 * set up a wait of upto 5 seconds just to see if the job gets
	 * going without error.
	 */
	rval = bounded_activity_wait(&status, 5, *jobid, pid, copystr(*jobid),
	    dumpwait);
	if (rval == -1) {
		samerrno = SE_DUMP_FAILED_SEE_LOG;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_DUMP_FAILED_SEE_LOG), path);
		/* Since there was an error clear the job id */
		free(*jobid);
		*jobid = NULL;
		return (-1);
	}

	/*
	 * Might want to clear the jobid in the long run if the job finished
	 * successfully in the 5 seconds.
	 */

	return (0);
}


/* Make a dump available; decompress it if required, index it */

int
decompress_dump(
	ctx_t *c,		/* ARGSUSED */
	char *fsname,
	char *dumppath,
	char **jobid)
{
	int rval;
	argbuf_t *arg;	/* Buffer to pass to thread */
	pthread_t tid;
	char	*tjobid;

	/* Before we do anything else, make sure this isn't a second click */
	rval = decomfind(dumppath);
	if (rval) {
		return (samrerr(SE_DECOMP_GOING, dumppath));
	}

	/* Set up argument block for thread */
	arg = mallocer(sizeof (decombuf_t));
	if (arg == NULL)
		return (-1);

	/* Copy arguments to block we will pass to thread */
	strlcpy(arg->d.fsname, fsname, MAXNAMELEN);
	strlcpy(arg->d.dumppath, dumppath, MAXPATHLEN);

	/* Create datastructures around thread */
	rval = start_activity(decomlist, decomkill, SAMRTHR_D, arg, jobid);
	if (rval) {
		Trace(TR_DEBUG, "decompress_dump start_activity failed %s",
		    samerrmsg);
		free(arg);
		return (rval);
	}

	strlcpy(arg->d.xjobid, *jobid, MAXNAMELEN);

	/* Start a thread to run decompress, and return before it starts */

	tjobid = copystr(*jobid);

	pthread_create(&tid, &samr_pth_attr, samr_decomp, tjobid);

	set_pid_or_tid(*jobid, 0, tid);

	return (0);
}

/* Undo the Make Available - get rid of index and uncompressed dump */

int
cleanup_dump(
	ctx_t *c,		/* ARGSUSED */
	char *fsname,
	char *dumppath)
{
	int rval;

	rval = do_cleanup_dump(fsname, dumppath, FALSE);

	return (rval);
}

/* Delete a dump file */
int
delete_dump(
	ctx_t  *c,			/* ARGSUSED */
	char   *fsname,
	char   *dumppath)
{
	int rval;

	rval = do_cleanup_dump(fsname, dumppath, TRUE);

	return (rval);
}

static int
do_cleanup_dump(
	char *fsname,
	char *dumppath,
	boolean_t delete_all)
{
	int	rval;
	char	snapname[MAXPATHLEN + 1];
	char	*bufp;
	char	c;

	if (ISNULL(dumppath)) {
		return (-1);
	}

	/*
	 * normalize the snapshot name for subsequent checks
	 *
	 * decomfind() and samrcleanup() require the short form
	 * of the snapshot name as input:  the [.lk].dmp[.gz|Z]
	 * extensions should NOT be included in the passed-down
	 * name.
	 */
	strlcpy(snapname, dumppath, sizeof (snapname));
	bufp = strstr(snapname, ".dmp");
	if (bufp != NULL) {
		c = *(bufp + 4);
		if ((c == '.') || (c == '\0')) {
			*bufp = '\0';
			bufp = strstr(snapname, ".lk");
			if (bufp != NULL) {
				c = *(bufp + 3);
				if ((c == '.') || (c == '\0')) {
					*bufp = '\0';
				}
			}
		}
	}

	/* Before we do anything, make sure indexing isn't in progress */
	rval = decomfind(snapname);
	if (rval) {
		return (samrerr(SE_DECOMP_GOING, snapname));
	}

	rval = samrcleanup(fsname, snapname, delete_all);
	return (rval);
}

/* ---------------------------------------------------------------- */
/* Version manipulation */

/* OBSOLETE - now going through the sam_file_util.c functions */
int
list_versions(
	ctx_t *c, char *fsname, char *dumpname, int maxentries, /*ARGSUSED*/
	char *filepath, char *restrictions, sqm_lst_t **versions)
{
	return (-1);
}

int
get_version_details(ctx_t *c, char *fsname, char *dumpname, /* ARGSUSED */
		    char *filepath, sqm_lst_t **details)
{
	return (-1);
}

int
search_versions(
	ctx_t *c, char *fsname, char *dumpname, int maxentries, /*ARGSUSED*/
	char *filepath, char *restrictions, char **jobid) /* ARGSUSED */
{
	return (-1);

}

int
get_search_results(ctx_t *c, char *fsname, sqm_lst_t **results) /* ARGSUSED */
{
	return (-1);
}

/* Restore an inode */
int
restore_inodes(
	ctx_t *c, 		/* ARGSUSED */
	char *fsname,
	char *dumpname,
	sqm_lst_t *filepaths,
	sqm_lst_t *dest,
	sqm_lst_t *copies,
	replace_t replace,
	char **jobid)
{
	int rval = 0;
	argbuf_t *arg;
	node_t *node;
	pthread_t  tid;
	char  *tjobid;
	dumpspec_t *dsp;

	/* Make sure we don't already have a thread in progress */
	pthread_mutex_lock(&search_mutex);
	if (search_active) {
		pthread_mutex_unlock(&search_mutex);
		return (samrerr(SE_RESTOREBUSY, "restore"));
	} else {
		search_active = 1; /* preclude other search/restores */
	}
	pthread_mutex_unlock(&search_mutex);

	Trace(TR_MISC, "restore_inodes %s %s", fsname, dumpname);

	/*
	 * We are now in "restore thread", having determined nobody else
	 * is doing a restore or search, so we can clobber these values
	 */
	restore_max = 0;
	restore_cur = 0;

	/* Set up locations */

	dsp = malloc(sizeof (dumpspec_t));
	if (dsp == NULL) {
		setsamerr(SE_NO_MEM);
		rval = -1;
		goto err;
	}

	if (rval = set_dump(fsname, dumpname, dsp)) {
		free(dsp);
		goto err;
	}

	/*
	 * Before trying to restore, sanity check lists.
	 * As a side effect, this fills in restore_max.
	 */
	rval = restore_check(filepaths, copies, dest, dsp);

	if (rval) {
		close_dump(dsp);
		free(dsp);
		goto err;
	}

	/* Passed checks, so start up a thread to run the restore */
	/* Set up argument block to pass arguments to the thread. */
	arg = mallocer(sizeof (restbuf_t));
	if (arg == NULL) {
		close_dump(dsp);
		free(dsp);
		rval = -1;
		goto err;
	}

	strlcpy(arg->r.fsname, fsname, MAXNAMELEN);
	strlcpy(arg->r.dumpname, dumpname, MAXPATHLEN);

	/* set up the dump information from set_dump() */
	arg->r.dsp = dsp;

	/* Copy lists because caller frees them up on us */
	arg->r.filepaths = lst_create();
	node = filepaths->head;
	while (node != NULL) {
		lst_append(arg->r.filepaths, copystr(node->data));
		node = node->next;
	}

	arg->r.dest = lst_create();
	node = dest->head;
	while (node != NULL) {
		lst_append(arg->r.dest, copystr(node->data));
		node = node->next;
	}

	arg->r.copies = lst_create();
	node = copies->head;
	while (node != NULL) {
		lst_append(arg->r.copies, copyint(*(int *)node->data));
		node = node->next;
	}

	/* set the desired conflict resolution */
	arg->r.replace = replace;

	rval = start_activity(restorelist, restorekill, SAMRTHR_R, arg, jobid);

	tjobid = copystr(*jobid);
	strlcpy(arg->r.xjobid, *jobid, MAXNAMELEN);

	pthread_create(&tid, &samr_pth_attr, samr_restore, tjobid);
	set_pid_or_tid(*jobid, 0, tid);

	/* Return while restore is in progress */
	return (0);

err:
	pthread_mutex_lock(&search_mutex);
	search_active = 0;	/* allow other search/restores */
	pthread_mutex_unlock(&search_mutex);
	return (rval);
}

/*
 * Mark a snapshot as "keep forever".  A so-marked snapshot will
 * not be deletable with the "Delete Dump" request from the GUI,
 * or purged as a result of a retention schedule.
 *
 * Name must be fully qualified with the path, but will be missing
 * any .dmp* extension.
 */
int
set_snapshot_locked(ctx_t *c, char *snapname)		/* ARGSUSED */
{
	int	rval;

	/* If there's a decompression task in progress, fail */
	rval = decomfind(snapname);
	if (rval) {
		return (samrerr(SE_DECOMP_GOING, snapname));
	}

	rval = set_snap_lock_state(snapname, TRUE);

	if (rval != 0) {
		return (samrerr(SE_SNAP_LOCK_FAILED, snapname));
	}

	return (0);
}

/*
 * Clear the "keep forever" flag.  The specified snapshot will
 * now be deletable with the "Delete Dump" request from the GUI,
 * and available to be purged as a result of a retention schedule.
 *
 * Name must be fully qualified with the path, but will be missing
 * any .dmp* extension.
 */
int
clear_snapshot_locked(ctx_t *c, char *snapname)		/* ARGSUSED */
{
	int	rval;

	/* If there's a decompression task in progress, fail */
	rval = decomfind(snapname);
	if (rval) {
		return (samrerr(SE_DECOMP_GOING, snapname));
	}

	rval = set_snap_lock_state(snapname, FALSE);

	if (rval != 0) {
		return (samrerr(SE_SNAP_UNLOCK_FAILED, snapname));
	}

	return (0);
}

/*
 * Worker function for set_snapshot_locked and clear_snapshot_locked
 *
 * The boolean "lock" should be TRUE to lock the snapshot,
 * and FALSE to unlock it.
 */
static int
set_snap_lock_state(char *snapname, boolean_t lock)
{
	int		rval;
	int		rnval = 0;
	char		namebuf[BUFFSIZ];
	char		lkname[BUFFSIZ];
	struct stat64	sbuf;
	int		i = 0;
	char		*sp;
	char		*ep;
	char		*lsp;
	char		*lep;

	if (ISNULL(snapname)) {
		return (-1);
	}

	snprintf(namebuf, sizeof (namebuf), "%s.dmp", snapname);
	snprintf(lkname, sizeof (lkname), "%s.lk.dmp", snapname);

	/* set up the string pointers */
	if (lock) {
		sp = &namebuf[0];
		lsp = &lkname[0];
	} else {
		sp = &lkname[0];
		lsp = &namebuf[0];
	}

	ep = sp + strlen(sp);
	lep = lsp + strlen(lsp);

	rval = stat64(sp, &sbuf);
	if (rval == 0) {
		rval = rename(sp, lsp);
		if (rval != 0) {
			return (rval);
		}
	}

	/* now check for compressed snaps */
	while (comp_exts[i] != NULL) {
		strlcpy(ep, comp_exts[i], BUFFSIZ);
		rval = stat64(sp, &sbuf);
		if (rval == 0) {
			strlcpy(lep, comp_exts[i], BUFFSIZ);
			rnval = rename(sp, lsp);
			if (rnval != 0) {
				break;
			}
		}
		i++;
	}

	/*
	 * If we failed renaming the compressed dump file, set everything
	 * back the way it was.
	 */
	if (rnval != 0) {
		*ep = '\0';
		*lep = '\0';

		if (stat64(lsp, &sbuf) == 0) {
			rename(lsp, sp);
		}

		if (comp_exts[i] != NULL) {
			strlcpy(ep, comp_exts[i], BUFFSIZ);
			strlcpy(lep, comp_exts[i], BUFFSIZ);
			if (stat64(lsp, &sbuf) == 0) {
				rename(lsp, sp);
			}
		}
		return (rnval);
	}

	return (0);
}

/*
 *  Returns a key-value string for each indexed snapshot of
 *  the form "name=<snapshot name>,date=<snapshot date>
 *
 *  snapdir is optional.  If not specified, returns all the
 *  indexed snapshots for the filesystem regardless of where the
 *  .dmp file is located.
 */
int
get_indexed_snapshots(
	ctx_t		*c,		/* ARGSUSED */
	char		*fsname,
	char		*snapdir,
	sqm_lst_t		**results)
{
	int		st;
	snapspec_t	*snapArray = NULL;
	uint32_t	count;
	int		i;
	size_t		namelen;
	char		*ptr;
	char		buf[2048];
	char		*basep;

	if (ISNULL(fsname, results)) {
		return (-1);
	}

	*results = lst_create();
	if (*results == NULL) {
		return (-1);
	}

	/*
	 * Contact the database to see if any snaps are indexed
	 * Failure here is ok, means no indices are available at the moment
	 */
	st = get_snapshot_status(fsname, &snapArray, &count);

	if (st != 0) {
		/* no indexed snaps */
		return (0);
	}

	/* sort the returned array */
	qsort(snapArray, count, sizeof (snapspec_t), qsort_descending);

	if (snapdir != NULL) {
		namelen = strlen(snapdir);
	}

	for (i = 0; i < count; i++) {
		ptr = snapArray[i].snapname;

		/* only want indexed snaps here */
		if (snapArray[i].snapState != INDEXED) {
			continue;
		}

		if ((snapdir != NULL) && (*snapdir != '\0')) {
			basep = strrchr(ptr, '/');

			if (basep == NULL) {
				/* not fully qualified */
				continue;
			}

			/* avoid substring matches */
			if ((namelen != (basep - ptr)) ||
			    (strncmp(ptr, snapdir, namelen) != 0)) {
				continue;
			}
		}

		snprintf(buf, sizeof (buf), "name=%s,date=%ld",
		    ptr, snapArray[i].snaptime);

		ptr = copystr(buf);
		if (ptr == NULL) {
			st = -1;
			break;
		}
		st = lst_append(*results, ptr);
		if (st != 0) {
			free(ptr);
			break;
		}
	}

	if (st != 0) {
		lst_free_deep(*results);
		*results = NULL;
	}

	return (st);
}

/*
 *  Returns a list of directories where indexed snapshots are
 *  stored.
 */
int
get_indexed_snapshot_directories(
	ctx_t		*c,		/* ARGSUSED */
	char		*fsname,
	sqm_lst_t		**results)
{
	int		st;
	snapspec_t	*snapArray = NULL;
	uint32_t	count;
	int 		i;
	node_t		*node;
	char		*dirp;
	char		*ptr;
	char		buf[MAXPATHLEN + 1] = {0};

	if (ISNULL(fsname, results)) {
		return (-1);
	}

	*results = lst_create();
	if (*results == NULL) {
		return (-1);
	}

	/* Get directory specified in the snapshot schedule, if any */
	st = getdumpdir(fsname, buf);
	if (st == 0) {
		ptr = strdup(buf);
		if (ptr == NULL) {
			setsamerr(SE_NO_MEM);
			st = -1;
			goto done;
		}

		st = lst_append(*results, ptr);
		if (st != 0) {
			free(ptr);
			goto done;
		}
	}

	/*
	 * Contact the database to see if any snaps are indexed
	 * Failure here is ok, means no indices are available at the moment
	 */
	st = get_snapshot_status(fsname, &snapArray, &count);

	if (st != 0) {
		/* no indexed snaps */
		return (0);
	}

	for (i = 0; i < count; i++) {
		dirp = dirname(snapArray[i].snapname);

		/* reject snaps with invalid paths */
		if ((dirp == NULL) || (strcmp(dirp, ".") == 0)) {
			continue;
		}

		/* Weed out metrics-only snapshots */
		if (snapArray[i].snapState == METRICS) {
			continue;
		}

		for (node = (*results)->head; node != NULL; node = node->next) {
			if (node->data == NULL) {
				continue;
			}

			if (strcmp(node->data, dirp) == 0) {
				/* duplicate, we're done here */
				break;
			}
		}

		if (node == NULL) {
			ptr = strdup(dirp);
			if (ptr == NULL) {
				setsamerr(SE_NO_MEM);
				st = -1;
			} else {
				st = lst_append(*results, ptr);
				if (st != 0) {
					free(ptr);
				}
			}
			if (st != 0) {
				break;
			}
		}
	}

done:
	if (st != 0) {
		lst_free_deep(*results);
		*results = NULL;
	}

	return (st);
}


/*
 * Helper function to sort the array of returned snapshots
 * in descending order by time.
 */
static int
qsort_descending(const void *in_a, const void *in_b)
{
	time_t	a = 0;
	time_t	b = 0;

	if (in_a != NULL) {
		a = ((snapspec_t *)in_a)->snaptime;
	}

	if (in_b != NULL) {
		b = ((snapspec_t *)in_b)->snaptime;
	}

	return (a - b);
}

/* TEMPORARY FUNCTION - Translates snapsched to CSD-style string */
static int
snapsched_to_CSDstring(snapsched_t *sched, char *buf, size_t buflen)
{
	int		a;
	int		i;
	struct tm	tmtime;
	time_t		csdtime;
	uint_t		pval;
	char		c;

	if (ISNULL(sched, buf)) {
		return (-1);
	}

	translate_date(sched->starttime, &tmtime);
	csdtime = mktime(&tmtime);

	translate_period(sched->periodicity, EXTENDED_PERIOD_UNITS,
	    &pval, &c);

	a = snprintf(buf, buflen, "name_prefix=%s,frequency=%dP%d",
	    sched->id.fsname, csdtime, (int)pval);

	if (sched->duration[0] != '\0') {
		a += snprintf(buf + a, buflen - a, ",retainfor=%s",
		    sched->duration);
	}

	a += snprintf(buf + a, buflen - a,
	    ",location=%s,names=%s,autoindex=%d,disabled=%d",
	    sched->location, sched->namefmt, sched->autoindex,
	    sched->disabled);

	if (sched->prescript[0] != '\0') {
		a += snprintf(buf + a, buflen - a,
		    ",prescript=%s,prescrfatal=%d",
		    sched->prescript, sched->prescriptFatal);
	}

	if (sched->postscript[0] != '\0') {
		a += snprintf(buf + a, buflen - a,
		    ",postscript=%s", sched->postscript);
	}

	if (sched->compress != 0) {
		char	*cstr;

		if (sched->compress == 1) {
			cstr = "gzip";
		} else if (sched->compress == 2) {
			cstr = "compress";
		} else {
			cstr = "none";
		}
		a += snprintf(buf + a, buflen - a,
		    ",compress=%s", cstr);
	}

	if (sched->logfile[0] != '\0') {
		a += snprintf(buf + a, buflen - a,
		    ",logfile=%s", sched->logfile);
	}

	if (sched->excludeDirs[0][0] != '\0') {
		a += snprintf(buf + a, buflen - a, ",excludedirs=%s",
		    sched->excludeDirs[0]);
		for (i = 1; i < 10; i++) {
			if (sched->excludeDirs[i][0] == '\0') {
				break;
			}
			a += snprintf(buf + a, buflen - a,
			    ":%s", sched->excludeDirs[i]);
		}
	}

	return (0);
}
