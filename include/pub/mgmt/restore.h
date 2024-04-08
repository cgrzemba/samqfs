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
#ifndef _RESTOREUI_H
#define	_RESTOREUI_H

#pragma ident	"$Revision: 1.41 $"

/*
 * restoreui.h - structures and entries for interfacing with file restore.
 */


#include <zlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>

#include "mgmt/config/common.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/cmn_csd_types.h"
#include "pub/mgmt/process_job.h"

/* Global definitions and structures */

#define	RESTORELOG	VAR_DIR"/restore.log"
#define	CSDFILENAME	CFG_DIR"/csd.cmd"

/* Structure for holding dump file and dump index information during restore */
typedef struct dumpspec_s {
	char		snapname[MAXPATHLEN+1];	/* name of snapshot */
	char		fsname[256];		/* filesystem name */
	gzFile		fildmp;		/* gzFile struct ptr for dump file */
	FILE		*logfil;	/* Log file */
	int		csdversion;	/* CSD version */
	boolean_t	byteswapped;	/* Dump is other-endian */
	uint64_t	numfiles;	/* number of files in snapshot */
	sam_time_t	snaptime;	/* time recorded in snapshot */
} dumpspec_t;

/*
 * ----------------------------------------------------------------
 * Function definitions
 */

/* CSD setup */
int
set_csd_params(
	ctx_t *c,
	char *fsname,
	char *parameters);

int
get_csd_params(
	ctx_t *c,
	char *fsname,
	char **parameters);	/* Returned string */

/* Dump information retrieval */
/* OBSOLETE - use list_dumps_by_dir() */
int
list_dumps(
	ctx_t *c,
	char *fsname,
	sqm_lst_t **dumps);	/* Returned list of strings */

int
list_dumps_by_dir(
	ctx_t *c,
	char *fsname,
	char *usepath,		/* directory where dumps are located */
	sqm_lst_t **dumps);	/* Returned list of strings */

int
get_dump_status(
	ctx_t *c,
	char *fsname,
	sqm_lst_t *dumps,
	sqm_lst_t **status);	/* Returned list of strings */

int
get_dump_status_by_dir(
	ctx_t *c,
	char *fsname,
	char *usepath,		/* directory where dumps are located */
	sqm_lst_t *dumps,
	sqm_lst_t **status);	/* Returned list of strings */


int
decompress_dump(
	ctx_t *c,
	char *fsname,
	char *dumpname,
	char **jobid);		/* Returned jobid for long-duration task */

int
cleanup_dump(
	ctx_t *c,
	char *fsname,
	char *dumppath);

int
delete_dump(
	ctx_t *c,
	char *fsname,
	char *dumppath);

int
take_dump(
	ctx_t *c,
	char *fsname,
	char *dumpname,
	char **jobid);		/* Returned jobid for long-duration task */


/* Version manipulation */
int
list_versions(
	ctx_t *c,
	char *fsname,
	char *dumpname,
	int maxentries,
	char *filepath,
	char *restrictions,
	sqm_lst_t **versions);	/* Returned list of strings */


/*
 * get_version_details returns five strings, the first of which is
 * a set of keyword=value pairs. The keywords are:
 * - protection
 * - size
 * - user
 * - group
 * - created
 * - modified
 */

int
get_version_details(
	ctx_t *c,
	char *fsname,
	char *dumpname,
	char *filepath,
	sqm_lst_t **details);	/* Returned list of strings */

int
search_versions(
	ctx_t *c,
	char *fsname,
	char *dumpname,
	int maxentries,
	char *filepath,
	char *restrictions,
	char **jobid);		/* Returned jobid */

int
get_search_results(
	ctx_t *c,
	char *fsname,
	sqm_lst_t **versions);	/* Returned list of strings */

/*
 * Restore an inode and stage the files (optional)
 * The list copies either specifies DONT_STAGE, SAM_CHOOSES_COPY or
 * which archive copy (0-3) is to be staged for each file in filepaths
 */
int
restore_inodes(
	ctx_t *c,
	char *fsname,
	char *dumpname,
	sqm_lst_t *filepaths,	/* List of strings */
	sqm_lst_t *destinations,	/* List of strings */
	sqm_lst_t *copies,		/* list of integers */
	replace_t replace,		/* Conflict resolution */
	char **jobid);			/* Returned job id */

/*
 * Mark a snapshot as "keep forever".  A so-marked snapshot will
 * not be deletable with the "Delete Dump" request from the GUI,
 * or purged as a result of a retention schedule.
 *
 * Name must be fully qualified with the path, but will be missing
 * any .dmp* extension.
 */
int
set_snapshot_locked(ctx_t *c, char *snapname);

/*
 * Clear the "keep forever" flag.  The specified snapshot will
 * now be deletable with the "Delete Dump" request from the GUI,
 * and available to be purged as a result of a retention schedule.
 *
 * Name must be fully qualified with the path, but will be missing
 * any .dmp* extension.
 */
int
clear_snapshot_locked(ctx_t *c, char *snapname);


/* ---------------------------------------------------------------- */
/* Below stuff is not part of the interface, but is shared innards. */
/* ---------------------------------------------------------------- */

/* Type which needs to be defined as 64-bits in a file */
typedef long long timex_t;

/* Structure used to store a repetition-period */
typedef struct period {
	time_t start;
	time_t interval;
} period_t;

/*
 * Structure used to hold a retention period
 *
 * Unit may be
 *	D = days
 *	W = weeks
 *	M = months
 *	Y = years
 */
typedef struct keep_for {
	int16_t	val;
	char	unit;
} keep_for_t;

/* Structure to hold parsed csd entry */
typedef struct csdbuf_s {
	char location[MAXPATHLEN];
	char names[MAXPATHLEN];
	char prescript[MAXPATHLEN];
	char postscript[MAXPATHLEN];
	char compress[MAXPATHLEN];
	char logfile[MAXPATHLEN];
	char nameprefix[MAXPATHLEN];
	period_t frequency;
	time_t retention;		/* deprecated for retainfor */
	keep_for_t retainfor;
	boolean_t autoindex;
	boolean_t disabled;
	boolean_t prescrfatal;
	char excludedirs[10][MAXPATHLEN];
} csdbuf_t;


/* Function declarations needed for cross-calls within restore */

int parse_csdstr(char *str, csdbuf_t *csdbuf, int csdsize);

int getdumpdir(char *fsname, char *dumpdirname);

int *copyint(int value);

int walk_dumps(char *location, char *file, sqm_lst_t **dumps);

int restore_check(sqm_lst_t *filepaths, sqm_lst_t *copies, sqm_lst_t *dest,
	dumpspec_t *dsp);

int decomfind(char *dumpname);

int samrcleanup(char *fsname, char *dumppath, boolean_t delete_all);

int samr_listver(char *filepath, char *restrictions, int maxentries,
	sqm_lst_t **versions, dumpspec_t *dsp);

int samr_version_details(char *filepath, sqm_lst_t **details, dumpspec_t *dsp);

void *samr_restore(void *jobid);

void *samr_decomp(void *jobid);

void *dumpwait(void *jobid);

int set_dump(char *fsname, char *dumpname, dumpspec_t *dsp);

int close_dump(dumpspec_t *dsp);

int dumplist(samrthread_t *ptr, char **result);

int decomlist(samrthread_t *ptr, char **result);

int decomkill(samrthread_t *ptr);

void decomcleanup(void *ptr);

int restorelist(samrthread_t *ptr, char **result);

int restorekill(samrthread_t *ptr);

int get_snap_name(
	char		*usepath,
	char		*dmpname,
	char		*snappath,
	int		*locked,
	int		*compressed,
	struct stat64	*statbuf);

void rlog(FILE *logfil, char *text, char *str1, char *str2);

/*
 *  Returns a list of directories where indexed snapshots are
 *  stored.
 */
int
get_indexed_snapshot_directories(
	ctx_t		*c,
	char		*fsname,
	sqm_lst_t	**results);

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
	ctx_t		*c,
	char		*fsname,
	char		*snapdir,
	sqm_lst_t	**results);

#endif	/* _RESTOREUI_H */
