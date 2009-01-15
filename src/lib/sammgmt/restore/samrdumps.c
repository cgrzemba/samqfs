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
#pragma ident	"$Revision: 1.62 $"

/*
 * samrdumps.c - Library for reading/searching samfsdumps
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <fnmatch.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>

#include "pub/lib.h"
#include "pub/stat.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/process_job.h"
#include "pub/mgmt/restore.h"
#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "sam/fs/ino.h"
#include "sam/fs/dirent.h"
#include "sam/sam_trace.h"
#include "sam/lib.h"
#include "mgmt/util.h"
#include "mgmt/private_file_util.h"
#include "src/fs/include/bswap.h"
#include "src/fs/cmd/dump-restore/csd.h"
#include "mgmt/cmn_csd.h"
#include "mgmt/restore_int.h"
#include "pub/mgmt/fsmdb_api.h"
#include "mgmt/file_details.h"
#include "pub/mgmt/file_util.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#define	BUFFSIZ MAXPATHLEN
#define	MAPSIZ	0x400000	/* Preferred size of mapped dump chunk (4MB) */

/* Constants used while restoring inode and staging file in one operation */
#define	STAGE_AS_AT_DUMP	3000	/* stage if file online at dump time */
#define	DONT_STAGE		2000	/* don't stage */
#define	SAM_CHOOSES_COPY	1000	/* don't stage from a specific copy */

/* Macro to propagate and return error value */
#define	RVAL(X) {rval = X; if (rval) return (rval); }

extern void restorecleanup(void *ptr);

extern pthread_mutex_t search_mutex;
extern sqm_lst_t *search_results;
extern int search_active;
extern uint64_t restore_max;
extern uint64_t restore_cur;

/* Forward reference */

static int
do_restore_inode(
	char		*filename,	/* path where file is to be restored */
	filedetails_t	*details,	/* ptr to file info */
	dumpspec_t	*ds,		/* info about samfsdump */
	replace_t	replace		/* if & how file should be replaced */
);

static int
restore_node(filedetails_t *idxnode, int *copy, char *reldest, char *mountpt,
	dumpspec_t *dsp, replace_t replace);

static int
restore_children(char *dir_name, int *copy, char *dest, char *mountpt,
	dumpspec_t *dsp, replace_t replace, boolean_t count_only);

/*
 * rlog - write a timestamped entry to the restore log.
 */
void
rlog(FILE *logfil, char *text, char *str1, char *str2)
{
	time_t now;
	struct tm nowtm;
	char buff[BUFFSIZ];

	/*
	 * don't set samerrno/errmsg.  It will probably overwrite the
	 * real error status we're trying to log.
	 */
	if (logfil == NULL) {
		return;
	}

	now = time(NULL);	/* timestamp of NOW */
	localtime_r(&now, &nowtm);
	strftime(buff, BUFFSIZ, "%e %b %Y %T %Z ", &nowtm);
	(void) strlcat(buff, text, BUFFSIZ);
	(void) strlcat(buff, "\n", BUFFSIZ); /* Add an end-of-line */

	if (str1 != NULL) {
		(void) fprintf(logfil, buff, str1, str2);
	} else {
		/*
		 * no other args, avoid use of the printf() functions to
		 * properly handle pathnames with '%' characters in them.
		 */
		fputs(buff, logfil);
	}
	fflush(logfil);
}

/*
 * set_dump - Select a specific dump (and index) to open and check.
 */

int
set_dump(char *fsname, char *dumpname, dumpspec_t *dsp)
{
	char		dumppath[MAXPATHLEN+1];
	char		dumpdir[MAXPATHLEN+1];
	int		rval;
	csd_hdrx_t	hdr;
	char		*dirp;
	char		*namep;
	boolean_t	datapossible;
	int		fd;
	snapspec_t	*snapArray = NULL;
	uint32_t	count;
	int		i;

	memset(dsp, 0, sizeof (dumpspec_t));

	/*
	 * If dumpname is not fully qualified, then prepend
	 * the directory specified in the dump schedule.
	 */
	if (dumpname[0] == '/') {
		strlcpy(dumpdir, dumpname, sizeof (dumpdir));
		dirp = dirname(dumpdir);
		namep = dumpdir + (strlen(dirp) + 1);
	} else {
		/* Find the dump-containing directory */
		rval = getdumpdir(fsname, dumpdir);
		if (rval) {
			Trace(TR_MISC, "set_dump getdumpdir failure %s",
			    samerrmsg);
			return (rval);
		}

		dirp = dumpdir;
		namep = dumpname;
	}

	rval = get_snap_name(dirp, namep, dumppath, NULL, NULL, NULL);
	if (rval != 0) {
		return (samrerr(SE_BAD_DUMP_FILE, dumpname));
	}

	/*  Use open64() as gzopen() can't handle very large files */
	fd = open64(dumppath, O_RDONLY);
	if (fd < 0) {
		close_dump(dsp);
		Trace(TR_MISC, "set_dump opendmp %s failed.", dumppath);
		return (samrerr(SE_NOTAFILE, dumppath));
	}

	dsp->fildmp = gzdopen(fd, "rb");
	if (dsp->fildmp == NULL) {
		close_dump(dsp);
		Trace(TR_MISC, "set_dump opendmp %s failed.", dumppath);
		return (samrerr(SE_NOTAFILE, dumppath));
	}

	/* Read dump file header to verify and determine byte endiannness */
	rval = common_get_csd_header(dsp->fildmp, &dsp->byteswapped,
	    &datapossible, &hdr);
	if (rval != 0) {
		close_dump(dsp);
		Trace(TR_MISC, "Corrupt dump file %s", dumppath);
		return (samrerr(SE_BAD_DUMP_FILE, dumppath));
	}

	dsp->csdversion = hdr.csd_header.version;
	dsp->snaptime = hdr.csd_header.time;
	strlcat(dsp->fsname, fsname, sizeof (dsp->fsname));

	/* find the index information */
	rval = get_snapshot_status(fsname, &snapArray, &count);
	if (rval == 0) {
		snprintf(dsp->snapname, sizeof (dumpdir), "%s/%s", dirp, namep);
		for (i = 0; i < count; i++) {
			if (strcmp(dsp->snapname, snapArray[i].snapname) == 0) {
				if (snapArray[i].snapState == INDEXED) {
					dsp->numfiles = snapArray[i].numEntries;
				} else {
					rval = -1;
				}
				break;
			}
		}
		if (dsp->numfiles == 0) {
			rval = -1;
		}
		free(snapArray);
	}

	if (rval != 0) {
		close_dump(dsp);
		Trace(TR_MISC, "Corrupt dump file %s", dumppath);
		return (samrerr(SE_BAD_DUMP_FILE, dumppath));
	}

	return (0);
}

/*
 * close_dump - When the caller is finished with a dump file opened
 *		with set_dump(), call this function which will close
 *		open filedescriptors and munmap the remaining region.
 */

int
close_dump(dumpspec_t *dsp)
{
	if (dsp->logfil != NULL) {
		fclose(dsp->logfil);
	}
	if (dsp->fildmp != NULL) {
		gzclose(dsp->fildmp);
	}

	memset(dsp, 0, sizeof (dumpspec_t));

	return (0);
}

#if 0
/* TODO:  Replace with recursive search in database */
/*
 * Search for a name in a directory's children. Even though list is sorted,
 * do not depend on it, compare every single name before giving up. The
 * callers of this routine expect to find their target, and the error return
 * really is an exception case.
 */
static off64_t
search_list(off64_t disk_loc, char *name, int length, dumpspec_t *dsp)
{
	return (0);		/* Didn't find it, return nothing */
}
#endif


/* Restore a file specified by an index node pointer */

static int
restore_node(filedetails_t *idxnode, int *copy, char *reldest, char *mountpt,
	dumpspec_t *dsp, replace_t replace)
{
	int rval;
	char opts[4];
	char dest[MAXPATHLEN] = {0};

	if (*reldest == '/') {
		strlcpy(dest, reldest, MAXPATHLEN); /* Already absolute path */
	} else {
		snprintf(dest, MAXPATHLEN, "%s/", mountpt);

		/*
		 * strip out the relative path - then use strlcat() to add
		 * the path to be restored to avoid problems with % in
		 * pathnames.
		 */
		if ((reldest[0] == '.') && (reldest[1] == '/')) {
			strlcat(dest, reldest+2, MAXPATHLEN);
		} else {
			strlcat(dest, reldest, MAXPATHLEN);
		}
	}

	rval = do_restore_inode(dest, idxnode, dsp, replace);

	restore_cur++;		/* Count up number of files restored */

	switch (rval) {
		case  0:
			break;
		case -3:
			return (samrerr(SE_RESTORE_NOT_SAMFS, dirname(dest)));
			break;	/* NOTREACHED */
		case -2:
			return (samrerr(SE_FILE_ALREADY_EXISTS, dest));
			break;	/* NOTREACHED */
		case -1:
		default:
			return (samrerr(SE_IDRESTOREFAIL, dest));
			break;	/* NOTREACHED */
	}

	/* Stage the file if requested */
	if (*copy != DONT_STAGE) {
		opts[0] = 'i';	/* Specify immediate staging, no waiting */

		if ((*copy == SAM_CHOOSES_COPY) ||
		    ((*copy == STAGE_AS_AT_DUMP) &&
		    (!(idxnode->summary.flags & FL_OFFLINE)))) {
			opts[1] = 0;	/* No specific copy, just do it */
		} else {
			opts[1] = '1' + *copy;
			opts[2] = 0;
		}
		sam_stage(dest, opts); /* Ignore return value */
	}

	return (0);		/* Restore succeeded. */
}

/* Restore child nodes. Recursive. */
static int
restore_children(char *dir_name, int *copy, char *dest, char *mountpt,
	dumpspec_t *dsp, replace_t replace, boolean_t count_only)
{
	int		st;
	char		childdest[MAXPATHLEN + 1];
	char		msgbuf[MAX_MSGBUF_SIZE] = {0};
	char		catmsg[MAX_MSGBUF_SIZE] = {0};
	sqm_lst_t		*lstp = NULL;
	sqm_lst_t		*dirlist = NULL;
	uint32_t	morefiles = 0;
	restrict_t	filter;
	filedetails_t	*details;
	char		*ptr;
	char		startFrom[MAXPATHLEN + 1];
	node_t		*node;
	char		*lastFile = NULL;
	char		*startp;
	char		*destp;

	dirlist = lst_create();
	if (dirlist == NULL) {
		return (-1);
	}

	memset(&filter, 0, sizeof (restrict_t));

/* TODO:  Add testcancel points and cleanup function */

	startFrom[0] = '\0';

	/*
	 * get file info from the database in chunks so as not to
	 * overwhelm ourselves if we're restoring a huge directory
	 */
	do {
		lstp = lst_create();
		if (lstp == NULL) {
			goto done;
		}
		st = list_snapshot_files(dsp->fsname, dsp->snapname,
		    dir_name, startFrom, filter, 0, 2048, FALSE,
		    &morefiles, lstp);

		if (st != 0) {
			/* list_snapshot_files doesn't set samerrmsg */
			snprintf(msgbuf, sizeof (msgbuf),
			    GetCustMsg(SE_RESTORE_FAILED), "");
			snprintf(catmsg, sizeof (catmsg), "%d %s ",
			    st, strerror(st));
			strlcat(msgbuf, catmsg, sizeof (msgbuf));
			strlcat(msgbuf, dir_name, sizeof (msgbuf));
			rlog(dsp->logfil, msgbuf, NULL, NULL);
			PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
			    SE_RESTORE_FAILED, LOG_ERR,
			    msgbuf, NOTIFY_AS_FAULT);
			goto done;
		}

		/*
		 * Create new destination path - add filename with
		 * strlcat to avoid possible problems with % in
		 * the pathname.
		 */
		strlcpy(childdest, dest, MAXPATHLEN + 1);
		strlcat(childdest, "/", MAXPATHLEN + 1);
		destp = childdest + strlen(childdest);

		for (node = lstp->head; node != NULL; node = node->next) {
			details = node->data;

			if (details == NULL) {
				continue;
			}

			/*
			 * save the path name in case we need it to get
			 * more files
			 */
			lastFile = details->file_name;

			*destp = '\0';
			strlcat(childdest, details->file_name, MAXPATHLEN + 1);

			/* if we're only counting, don't call restore_node */

			/* Restore the child node */
			if (!count_only) {
				st = restore_node(details, copy, childdest,
				    mountpt, dsp, replace);
				if (st != 0) {
					strlcpy(catmsg,
					    GetCustMsg(SE_RESTORE_FAILED),
					    sizeof (catmsg));
					snprintf(msgbuf, sizeof (msgbuf),
					    catmsg, samerrmsg);
					rlog(dsp->logfil, msgbuf, NULL, NULL);
					PostEvent(DUMP_CLASS,
					    DUMP_INTERRUPTED_SUBCLASS,
					    SE_RESTORE_FAILED, LOG_ERR,
					    msgbuf, NOTIFY_AS_FAULT);
					/* file already exists isn't fatal */
					if (samerrno ==
					    SE_FILE_ALREADY_EXISTS) {
						st = 0;
					}
				}
			}

			if (S_ISDIR(details->prot)) {
				lst_append(dirlist, details->file_name);
				/* ensure not doubly deleted */
				details->file_name = NULL;
			}
		}

		if (count_only) {
			restore_max += lstp->length;
		}

		if (lastFile) {
			strlcpy(startFrom, lastFile, sizeof (startFrom));
		}
		lst_free_deep_typed(lstp, FREEFUNCCAST(free_file_details));
		lstp = NULL;
	} while (morefiles > 0);

	/* restore any directories we found along the way */

	strlcpy(startFrom, dir_name, sizeof (startFrom));
	strlcat(startFrom, "/", sizeof (startFrom));
	startp = startFrom + strlen(startFrom);

	for (node = dirlist->head; node != NULL; node = node->next) {
		ptr = (char *)node->data;
		*startp = '\0';

		/* Create new source path */
		strlcat(startp, ptr, sizeof (startFrom));

		/* Create new destination path */
		snprintf(childdest, MAXPATHLEN + 1, "%s/%s", dest, ptr);

		/* this will log individual errors for failure */
		(void) restore_children(startFrom, copy, childdest, mountpt,
		    dsp, replace, count_only);
	}

done:
	if (lstp != NULL) {
		lst_free_deep_typed(lstp, FREEFUNCCAST(free_file_details));
	}
	if (dirlist != NULL) {
		lst_free_deep(dirlist);
	}

	return (st);
}

/* Sanity check restore list */
int
restore_check(sqm_lst_t *filepaths, sqm_lst_t *copies,
    sqm_lst_t *dest, dumpspec_t *dsp)
{
	node_t *destp;
	node_t *pathp;
	node_t *copyp;
	int which;
	struct sam_stat	sb;
	int	rval;
	char	buf[MAXPATHLEN + 1];
	char	*slash_loc;

	/* Make sure both lists are the same length */
	if ((filepaths->length != dest->length) ||
	    (filepaths->length != copies->length)) {
		return (samrerr(SE_MISMATCHEDARGS, ""));
	}

	destp = dest->head;
	pathp = filepaths->head;
	copyp = copies->head;

	/*
	 * Sanity check to make sure all source files exist and destinations
	 * are viable and don't already exist.
	 */
	while (destp != NULL) {
		/*
		 * Count how many nodes this will be.
		 */
		if (strcmp(".", pathp->data) != 0) {
#ifdef COUNT_REQUESTED_NODES
			/* include the start point in the count */
			restore_max = 1;
			(void) restore_children(pathp->data, 0, "",
			    "", dsp, REPLACE_NEVER, TRUE);
#else
			/*
			 * counting can be very time consuming and cause
			 * the browser to time out.  The 'right' answer for
			 * very large directories is to have the number of
			 * entries stored in the database.  This approach,
			 * rather than walking the whole thing, should be
			 * investigated in the post-4.6 timeframe.  For now,
			 * the 4.6 GUI will handle the special case of
			 * max = 0 when reporting restore job status.
			 */
			restore_max = 0;
#endif	/* COUNT_REQUESTED_NODES */
		} else {
			/* whole file system, use numnodes from index */
			restore_max = dsp->numfiles;
		}

		/* Verify copy selected is legal */
		which = *(int *)(copyp->data);
		if ((which != DONT_STAGE) && (which != SAM_CHOOSES_COPY) &&
		    (which != STAGE_AS_AT_DUMP)) {
			if ((which < 0) || (which > 3))
				return (samrerr(SE_BADSTAGE, destp->data));
		}

		/* validate that destination directory is on a SAM filesystem */

		rval = sam_stat(destp->data, &sb, sizeof (sb));
		if ((rval == 0) && (!SS_ISSAMFS(sb.attr))) {
			return (samrerr(SE_RESTORE_NOT_SAMFS, destp->data));
		}

		/*
		 * Restore makes the full directory path when required, so
		 * keep looking until we find an existing directory
		 */
		strlcpy(buf, destp->data, sizeof (buf));
		if (rval != 0) {
			while ((slash_loc = strrchr(buf, '/')) != NULL) {
				*slash_loc = '\0';
				rval = sam_stat(buf, &sb, sizeof (sb));
				if (rval == 0) {
					break;
				}
			}

			/*
			 * rval will be non-zero only if no part of the
			 * path exists.  This really shouldn't happen...
			 */
			if ((rval != 0) || (!SS_ISSAMFS(sb.attr))) {
				return (samrerr(SE_RESTORE_NOT_SAMFS,
				    destp->data));
				break;
			}

		}
		destp = destp->next;
		pathp = pathp->next;
		copyp = copyp->next;
	}

	return (0);
}


/* wrapper to match pthread_cleanup_push interface */
static void
free_file_details_lst(void *l) {
	lst_free_deep_typed((sqm_lst_t *)l, FREEFUNCCAST(free_file_details));
}


/*
 * Interface to request a list of restores
 * Note that this is called only under an independent thread.
 */

void *
samr_restore(void* jobid)
{
	node_t		*destp;
	node_t		*pathp;
	node_t		*oldpathp;
	node_t		*copyp;
	int		rval;
	argbuf_t	*arg;
	char		mountpt[BUFFSIZ];
	dumpspec_t	*dsp;
	char		msgbuf[MAX_MSGBUF_SIZE] = {0};
	char		catmsg[MAX_MSGBUF_SIZE] = {0};
	int		fd;
	sqm_lst_t		*filelst = NULL;
	char		*ptr;
	size_t		end;

	arg = samr_get_args((char *)jobid);

	if (arg == NULL) {

		pthread_mutex_lock(&search_mutex);
		search_active = 0;  /* Indicate search/restore is done */
		pthread_mutex_unlock(&search_mutex);

		end_this_activity((char *)jobid);
		return (NULL);
	}
	Trace(TR_MISC, "restoring from %s %s",
	    arg->r.fsname, arg->r.dumpname);

	/*
	 * cleanup handler ends activity and frees arg but
	 * jobid was malloced in the function that called
	 * pthread_create and must be freed here.
	 */
	free(jobid);

	pthread_cleanup_push(&restorecleanup, arg);

	dsp = arg->r.dsp;

	rval = getfsmountpt(arg->r.fsname, mountpt, sizeof (mountpt));

	if (rval == 0) {
		/* Pick up pointers to starts of these lists */
		destp = arg->r.dest->head;
		copyp = arg->r.copies->head;
	} else {
		Trace(TR_MISC, "Restore init failed %s", samerrmsg);
		goto done;
	}

	/* Open log file */
	if ((fd = open64(RESTORELOG, O_WRONLY|O_APPEND|O_CREAT, 0644)) != -1) {
		dsp->logfil = fdopen(fd, "a"); /* Open logfile for append */
	}
	if (dsp->logfil == NULL) {
		/* must return or a crash will follow on rlog */
		rval = samrerr(SE_NOTAFILE, RESTORELOG);
		goto done;
	}

	/* Log the fact that we are starting a restore */
	strlcpy(catmsg, GetCustMsg(SE_START_RESTORE), sizeof (catmsg));
	snprintf(msgbuf, sizeof (msgbuf), catmsg, destp->data, samerrmsg);
	rlog(dsp->logfil, catmsg, destp->data, samerrmsg);

	/* strip off any trailing slashes on the destinations */
	for (pathp = destp; pathp != NULL; pathp = pathp->next) {
		ptr = (char *)pathp->data;

		if (ptr == NULL) {
			continue;
		}

		end = strlen(ptr) - 1;
		while ((ptr[end] == '/') || (ptr[end] == '.')) {
			ptr[end] = '\0';
		}
	}

	/*
	 * if we're restoring the whole filesystem, don't restore the
	 * root node, just call into the restore_children func.
	 */
	if (strcmp((char *)(arg->r.filepaths->head->data), ".") == 0) {
		restore_children(".", copyp->data,
		    destp->data, mountpt, dsp, arg->r.replace, FALSE);
		goto done;
	}


	/* get the details for the requested files */
	rval = collect_file_details_restore(dsp->fsname, dsp->snapname,
	    "", arg->r.filepaths, 0, &filelst);

	if (rval != 0) {
		/* we could probably do a better error */
		strlcpy(catmsg, GetCustMsg(SE_RESTORE_FAILED), sizeof (catmsg));
		snprintf(msgbuf, sizeof (msgbuf), catmsg, samerrmsg);
		strlcat(msgbuf, " ", sizeof (msgbuf));
		strlcat(msgbuf, destp->data, sizeof (msgbuf));
		rlog(dsp->logfil, msgbuf, NULL, NULL);
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_RESTORE_FAILED, LOG_ERR, msgbuf, NOTIFY_AS_FAULT);
		Trace(TR_MISC, "Restore init failed %s", samerrmsg);
		goto done;
	}

	/* Push a cleanup handler for the filelst */
	pthread_cleanup_push(&free_file_details_lst, filelst);

	for (pathp = filelst->head, oldpathp = arg->r.filepaths->head;
	    (pathp != NULL) && (oldpathp != NULL);
	    pathp = pathp->next, oldpathp = oldpathp->next) {

		filedetails_t	*details = (filedetails_t *)pathp->data;

		/*
		 * Restore this file/directory/link/whatever.
		 * If it's a directory, also restore children
		 */
		rval = restore_node(
		    details, copyp->data, destp->data, mountpt,
		    dsp, arg->r.replace);
		if (rval) {
			/*
			 * file already exists isn't fatal reset rval for
			 * later
			 */
			if (samerrno == SE_FILE_ALREADY_EXISTS) {
				rval = 0;
			}

			strlcpy(catmsg, GetCustMsg(SE_RESTORE_FAILED),
			    sizeof (catmsg));
			snprintf(msgbuf, sizeof (msgbuf), catmsg, samerrmsg);
			rlog(dsp->logfil, msgbuf, NULL, NULL);
			PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
			    SE_RESTORE_FAILED, LOG_ERR,
			    msgbuf, NOTIFY_AS_FAULT);
		}

		/* Restore any children */
		if ((rval == 0) && (S_ISDIR(details->prot))) {
			restore_children((char *)oldpathp->data, copyp->data,
			    destp->data, mountpt, dsp, arg->r.replace,
			    FALSE);
		}

		strlcpy(catmsg, GetCustMsg(SE_FINISH_RESTORE), sizeof (catmsg));
		snprintf(msgbuf, sizeof (msgbuf), catmsg, " ", samerrmsg);
		strlcat(msgbuf, destp->data, sizeof (msgbuf));
		rlog(dsp->logfil, msgbuf, NULL, NULL);

		destp = destp->next;
		copyp = copyp->next;
	}

	/* pop file list free handler */
	pthread_cleanup_pop(1);

done:
	/* pop the cleanup handler */
	pthread_cleanup_pop(1);

	return (NULL);
}



/*
 * Interface to request a complete-dump search
 *
 * Note that this is called as an independent thread, and must
 * exit via end_this_activity() to clean up.
 */
void *
samr_search(void* jobid)
{
	if (jobid == NULL) {
		return (NULL);
	}

	/* need to re-implement this */

	return (NULL);
}

static int
do_restore_inode(
	char		*filename,	/* path where file is to be restored */
	filedetails_t	*details,	/* ptr to file info */
	dumpspec_t	*ds,		/* info about samfsdump */
	replace_t	replace		/* if & how file should be replaced */
)
{
	int		rval;
	int		file_data = 0;
	int		file_data_read;
	char		path[MAXPATHLEN + 1];
	char		name[MAXPATHLEN + 1];
	char		slink[MAXPATHLEN + 1];
	char		*slash_loc;
	int		skipping;
	struct sam_perm_inode perm_inode;
	struct sam_stat	sb;
	struct sam_vsn_section *vsnp;
	struct sam_ioctl_idtime idtime;
	void		*data;
	int		n_acls;
	aclent_t	*aclp;
	timestackvars_t	tvars = {0};
	permstackvars_t	pvars = {0};
	int		out_dir_fd = -1;
	int		dir_fd = -1;
	char		*dirp;
	size_t		namelen;

	/*
	 * Read the dump file - seek to the specified file offset
	 *
	 * Offset is the location of the actual inode.  To use the
	 * common read functions, we need to back-up the length of the
	 * provided name.
	 */
	namelen = strlen(details->file_name);

	gzseek(ds->fildmp, details->snapOffset - namelen, SEEK_SET);

	rval = common_csd_read(ds->fildmp, name, namelen,
	    ds->byteswapped, &perm_inode);

	if (rval != 0) {
		return (-1);
	}

	data = NULL;
	vsnp = NULL;
	aclp = NULL;
	n_acls = 0;

	if (details->summary.flags & FL_HASDATA) {
		file_data_read = 0;
		file_data++;
	} else {
		file_data_read = file_data = 0;
	}

	if (S_ISREQ(perm_inode.di.mode)) {
		data = malloc(perm_inode.di.psize.rmfile);
	}

	/* read_next needs error handling... --SG */
	common_csd_read_next(ds->fildmp, ds->byteswapped, ds->csdversion,
	    &perm_inode, &vsnp, slink, data, &n_acls, &aclp);

	/* skip system files, shouldn't happen with indexes */
	if (perm_inode.di.id.ino == 1 || perm_inode.di.id.ino == 4 ||
	    perm_inode.di.id.ino == 5) {
		goto skip_file;
	}

	/*
	 * Make sure that we don't restore into a non-SAM-FS filesystem.
	 *
	 * Search backwards through path for a viable SAM-FS path.
	 * by calling sam_stat() and checking the directory attributes
	 */

	rval = sam_stat(filename, &sb, sizeof (sb));

	strlcpy(path, filename, sizeof (path));
	dirp = dirname(path);

	if (rval == 0) {
		if (replace == REPLACE_NEVER) {
			/* can't do anything, file exists */
			rval = -2;
			goto skip_file;
		}

		if (!SS_ISSAMFS(sb.attr)) {
			rval = -3;
			goto skip_file;
		}
	} else {
		/*
		 * file doesn't exist
		 * loop through path looking for samfs, create
		 * path if necessary
		 */

		rval = sam_stat(path, &sb, sizeof (sb));
		if ((rval == 0) && (!SS_ISSAMFS(sb.attr))) {
			rval = -3;
			goto skip_file;
		}

		/* only mkdir if parent directory does not exist */
		if (rval != 0) {
			while ((slash_loc = strrchr(path, '/')) != NULL) {
				*slash_loc = '\0';
				rval = sam_stat(path, &sb, sizeof (sb));
				if (rval == 0) {
					break;
				}
			}

			if (rval != 0) {	/* no part of path exists? */
				rval = -1;
				goto skip_file;
			}

			if (!SS_ISSAMFS(sb.attr)) {
				rval = -3;
				goto skip_file;
			}

			strlcpy(path, filename, sizeof (path));
			dirp = dirname(path);

			rval = mkdirp(dirp, S_IRWXU | S_IRWXG | S_IRWXO);
			if (rval != 0) {
				rval = -1;
				goto skip_file;
			}
		}
	}

	/* Open the parent directory so times can be properly reset */
	idtime.id.ino = sb.st_ino;
	idtime.id.gen = sb.gen;
	idtime.atime = sb.st_atime;
	idtime.mtime = sb.st_mtime;
	idtime.xtime = sb.creation_time;
	idtime.ytime = sb.attribute_time;

	dir_fd = open64(dirp, O_RDONLY);

	/* ok, ready to start restoring ... */

	/*
	 * if segment index, we have to skip the data segment inodes
	 * before we can get to the dumped data.
	 */
	if (!(S_ISSEGI(&perm_inode.di) && file_data)) {
		common_sam_restore_a_file(ds->fildmp, filename, &perm_inode,
		    vsnp, slink, data, n_acls, aclp, file_data,
		    &file_data_read, &out_dir_fd, replace, &tvars, &pvars);
	}

	if (S_ISSEGI(&perm_inode.di)) {
		struct sam_perm_inode seg_inode;
		struct sam_vsn_section *seg_vsnp;
		int i;
		offset_t seg_size;
		int no_seg;
		sam_id_t seg_parent_id = perm_inode.di.id;

		/*
		 * If we are restoring the data, don't restore the data segment
		 * inodes.  This means we will lose the archive copies, if any.
		 */
		skipping = file_data;

		/*
		 * Read each segment inode. If archive copies overflowed,
		 * read vsn sections directly after each segment inode.
		 */

		seg_size = (offset_t)perm_inode.di.rm.info.dk.seg_size *
		    SAM_MIN_SEGMENT_SIZE;
		no_seg = (perm_inode.di.rm.size + seg_size - 1) / seg_size;
		for (i = 0; i < no_seg; i++) {
			gzread(ds->fildmp, &seg_inode, sizeof (seg_inode));
			/*
			 * cs_restore keeps going on the segments even
			 * on error.  We'll do that here too, but I'm not
			 * sure it's right.
			 */
			if (ds->byteswapped) {
				if (sam_byte_swap(
				    sam_perm_inode_swap_descriptor,
				    &seg_inode, sizeof (seg_inode))) {
					continue;
				}
			}
			if (!(SAM_CHECK_INODE_VERSION(seg_inode.di.version))) {
				continue;
			}

			if (skipping) {
				continue;
			}

			seg_inode.di.parent_id = seg_parent_id;
			seg_vsnp = NULL;
			common_csd_read_mve(ds->fildmp, ds->csdversion,
			    &seg_inode, &seg_vsnp, ds->byteswapped);
			common_sam_restore_a_file(ds->fildmp, filename,
			    &seg_inode, seg_vsnp, NULL, NULL, 0, NULL,
			    file_data, NULL, &out_dir_fd, replace, &tvars,
			    &pvars);
			if (seg_vsnp) {
				free(seg_vsnp);
			}
		}
		/*
		 * Now that we have skipped the data segment inodes
		 * we can restore the dumped data if any.
		 */
		if (file_data) {
			common_sam_restore_a_file(ds->fildmp, filename,
			    &perm_inode, vsnp, slink, data, n_acls, aclp,
			    file_data, &file_data_read, &out_dir_fd,
			    replace, &tvars, &pvars);
		}
	}

skip_file:
	if (data) {
		free(data);
	}
	if (vsnp) {
		free(vsnp);
	}
	if (aclp) {
		free(aclp);
	}
	if (file_data && file_data_read == 0) {
		common_skip_embedded_file_data(ds->fildmp);
	}

	common_pop_permissions_stack(&pvars);
	if (dir_fd > 0) {
		common_pop_times_stack(dir_fd, &tvars);
		ioctl(dir_fd, F_IDTIME, &idtime);
		close(dir_fd);
	}

	if (out_dir_fd > 0) {
		close(out_dir_fd);
	}

	return (rval);
}
