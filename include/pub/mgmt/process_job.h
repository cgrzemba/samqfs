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
#ifndef	_PROCESS_JOB_H
#define	_PROCESS_JOB_H

#pragma ident	"$Revision: 1.25 $"


#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

#include "mgmt/cmn_csd_types.h"
#include <stdio.h>

/*
 * This enum is deprecated. It should not be used with any function other than
 * destroy_process. Even that usage may end in the 4.4 time frame.
 *
 * Types before PROC_JOBS_BEGIN are not process jobs because they have other
 * jobs created for them. They should therefore not be returned in the list of
 * process jobs.
 *
 * Jobs between PTYPE_PROC_JOBS_BEGIN and PTYPE_PROC_JOB are
 * all process jobs.
 *
 * Anything not in the list is an PTYPE_OTHER_JOB.
 * PTYPE_PROC_JOBS_BEGIN, PTYPE_PROC_JOB, PTYPE_OTHER_JOB
 * cannot be used as arguments to destroy_process.
 */
typedef enum {
	PTYPE_ALL_PROCS = -2,	/* filter key that matches all jobs */
	PTYPE_OTHER_JOB = -1,	/* any process that does not fit below. */
	PTYPE_ARCOPY,
	PTYPE_ARFIND,
	PTYPE_TPLABEL,
	PTYPE_SAMFSCK,
	PTYPE_MAX,
	PTYPE_PROC_JOBS_BEGIN,	/* defines beginning of proc jobs range */
	PTYPE_UMOUNT,
	PTYPE_MOUNT,
	PTYPE_SAMMKFS,
	PTYPE_ARCHIVER_CLI,
	PTYPE_SHAREFSD,
	PTYPE_PROC_JOB,		/* end of proc jobs range and search key */
} proctype_t;



/*
 * Process Job Filter Strings
 *
 * Built up as comma separated key value pairs all of which must match for
 * the job to match the filter.
 *
 * Current filter keys:
 * pid=n,
 * ppid=n,
 * argstr=stringtomatch,
 * ptype=proctype_t,
 * fname=file_that_was_execed
 */
#define	FILTER_TYPE_ARGS "type=%s,argstr=%s"
#define	FILTER_PID "pid=%d"
#define	FILTER_PID_PPID "pid=%d,ppid=%d"
#define	FILTER_TYPE "type=%s"


int
get_process_jobs(
ctx_t		*ctx,
char		*filter,
sqm_lst_t	**job_list);	/* list of proc_job_t */


/*
 * DESCRIPTION:
 *   cancel a SAM-FS job by killing the process specified
 * PARAMS:
 *   ctx_t *	IN   - context object
 *   pid_t	IN   - process id to be killed
 *   proctype_t	IN   - process type to be killed
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int destroy_process(ctx_t *ctx, pid_t pid, proctype_t ptype);


/*
 * Job control routines. These look through database of restore-related
 * (and possibly other sources later) activities, returning details to
 * be displayed by the GUI as jobs
 */


/* Type strings for activities */

#define	SAMRNONE	"SAMRNONE"
#define	SAMRDECOMPRESS	"SAMRDECOMPRESS"
#define	SAMRSEARCH	"SAMRSEARCH"
#define	SAMRRESTORE	"SAMRRESTORE"
#define	SAMRCRONTAB	"SAMRCRONTAB"
#define	SAMRDUMP	"SAMRDUMP"

#define	SAMDFSD		"SAMDFSD"
#define	SAMDARCHIVERD	"SAMDARCHIVERD"
#define	SAMDARFIND	"SAMDARFIND"
#define	SAMDSTAGERD	"SAMDSTAGERD"
#define	SAMDSTAGEALLD	"SAMDSTAGEALLD"
#define	SAMDAMLD	"SAMDAMLD"
#define	SAMDSCANNERD	"SAMDSCANNERD"
#define	SAMDROBOTSD	"SAMDROBOTSD"
#define	SAMDGENERICD	"SAMDGENERICD"
#define	SAMDSTKD	"SAMDSTKD"
#define	SAMDIBM3494D	"SAMIBM3494D"
#define	SAMDSONYD	"SAMSONYD"
#define	SAMDMGMTD	"SAMDMGMTD"
#define	SAMDCATSERVERD	"SAMDCATSERVERD"

#define	SAMDSHAREFSD	"SAMDSHAREFSD"
#define	SAMDRMTSERVER	"SAMDRMTSERVER"
#define	SAMDRMTCLIENT	"SAMDRMTCLIENT"
#define	SAMDRFT		"SAMDRFT"
#define	SAMDRELEASER   	"SAMDRELEASER"
#define	SAMDRECYCLER	"SAMDRECYCLER"

#define	SAMPFSCK	"SAMPFSCK"
#define	SAMPTPLABEL	"SAMPTPLABEL"
#define	SAMPMOUNT	"SAMPMOUNT"
#define	SAMPUMOUNT	"SAMPUMOUNT"
#define	SAMPSAMMKFS	"SAMPSAMMKFS"
#define	SAMPARCHIVERCLI	"SAMPARCHIVERCLI"

#define	SAMARELEASEFILES "SAMARELEASEFILES"
#define	SAMAARCHIVEFILES "SAMAARCHIVEFILES"
#define	SAMARUNEXPLORER	"SAMARUNEXPLORER"
#define	SAMASTAGEFILES	"SAMASTAGEFILES"
#define	SAMADISPATCHJOB "SAMADISPATCHJOB"

/*
 * DESCRIPTION:
 *   Provide a list of activities on remote node
 * PARAMS:
 *   ctx_t *	IN   - context object
 *   maxentries	IN   - Maximum number of entries to be returned
 *   restrict	IN   - Match-pattern string to limit exclude returns
 *   results	OUT  - List of strings describing activities. These strings
 *			 are keyword=value pairs, containing some of:
 *				activityid
 *				starttime
 *				type
 *				fsname
 *				details
 *				parentid
 *				description
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
list_activities(
	ctx_t *c,
	int maxentries,		/* Don't return too huge a list */
	char *restrict,		/* Restrictions for pre-pruning list */
	sqm_lst_t **results); /* Returned list of strings */


/*
 * DESCRIPTION:
 *   Kill an activity on a remote node.
 * PARAMS:
 *   ctx_t *	IN   - context object
 *   activityid	IN   - Identifier for activity to kill off
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */

int
kill_activity(
	ctx_t *c,
	char *activityid,	/* Activityid returned from list */
	char *type);		/* Type of activity, just to verify */



/*
 * Below here the functions and datastructures are not public
 */

/*
 * wait up to max_wait seconds or until the process identified by
 * arg_sub_str, type, pid, ppid has begun execution.
 */
int
wait_for_proc_start(
char		*filter,	/* see proc job filter description */
int		max_wait,	/* Maximum amount of time to wait */
int		interval);	/* How often to check for process */


/*
 * Internal datastructures for activity tracking - to hold arguments
 * for cross-thread calls. Type specific.
 */

/* List of types of argument structure, stored in (samrthread_t)->type */
typedef enum {
	UNKNOWN_ACTIVITY = -1,	/* Unknown Type */
	SAMRTHR_E = 0,		/* Empty */
	SAMRTHR_D = 1,		/* Decompress dump activity */
	SAMRTHR_S = 2,		/* Search activity */
	SAMRTHR_R = 3,		/* Restore activity */
	SAMRTHR_C = 4,		/* Crontab update activity */
	SAMRTHR_U = 5,		/* Immediate dump activity */

	SAMD_FSD	= 6,	/* File System Daemon */
	SAMD_ARCHIVERD	= 7,	/* Archive Daemon */
	SAMD_ARFIND	= 8,	/* Arfind Daemon (1 per fs) */
	SAMD_STAGERD	= 9,	/* Stager Daemon */
	SAMD_STAGEALLD	= 10,	/* Stageall Daemon */
	SAMD_AMLD	= 11,	/* Automated Library Daemon */
	SAMD_SCANNERD	= 12,	/* Stand alone drive monitor daemon */
	SAMD_CATSERVERD	= 13,	/* Catalog Daemon */
	SAMD_MGMTD	= 14,	/* Management Daemon */
	SAMD_ROBOTSD	= 15,	/* Media Changer Daemon */
	SAMD_GENERICD	= 16,	/* Media Changer Daemon */
	SAMD_STKD	= 17,	/* Media Changer Daemon */
	SAMD_IBM3494D	= 18,	/* Media Changer Daemon */
	SAMD_SONYD	= 19,	/* Media Changer Daemon */
	SAMD_SHAREFSD	= 20,	/* SAMD_SHAREFSD */
	SAMD_RMTSERVER	= 21,	/* SAMD_RMTSERVERD */
	SAMD_RMTCLIENT	= 22,	/* SAMD_RMTCLIENTD */
	SAMD_RFT	= 23,	/* SAMD_RFTD */
	SAMD_RELEASER	= 24,	/* SAMD_RELEASERD */
	SAMD_RECYCLER	= 25,  	/* SAMD_RECYCLERD */

	SAMP_FSCK	= 26,  	/* sam fsck process */
	SAMP_TPLABEL	= 27,	/* tplabel process */
	SAMP_MOUNT	= 28,	/* mount process */
	SAMP_UMOUNT	= 29,	/* umount process */
	SAMP_SAMMKFS	= 30,	/* sammkfs process */
	SAMP_ARCHIVERCLI = 31,	/* archiver cli process */

	SAMA_RELEASEFILES = 32, /* release files activity */
	SAMA_ARCHIVEFILES = 33, /* archive files activity */
	SAMA_RUNEXPLORER = 34,	/* samexplorer activity */
	SAMA_STAGEFILES	 = 35,	/* stage files activity */
	SAMA_DISPATCH_JOB = 36	/* command dispatcher job */
} activitytype_t;

/* Define max range of above enum */
#define	activitytypemax SAMA_DISPATCH_JOB+1

/* Decompress datastructure */
typedef struct decombuf_s {
	char fsname[MAXNAMELEN];
	char dumppath[MAXPATHLEN];
	char xjobid[MAXNAMELEN];
} decombuf_t;


/* Search datastructure */
typedef struct srchbuf_s {
	int maxentries;
	char fsname[MAXNAMELEN];
	char dumpname[MAXPATHLEN];
	char restrictions[MAXPATHLEN];
	char xjobid[MAXNAMELEN];
} srchbuf_t;

/* Restore datastructure */
typedef struct restbuf_s {
	char fsname[MAXNAMELEN];
	char dumpname[MAXPATHLEN];
	sqm_lst_t *filepaths;
	sqm_lst_t *dest;
	sqm_lst_t *copies;
	replace_t replace;
	void* dsp;
	char xjobid[MAXNAMELEN];
} restbuf_t;

/* Immediate dump job */
typedef struct dumpbuf_s {
	char fsname[MAXNAMELEN];
	char dumpname[MAXPATHLEN];
	pid_t pid;
} dumpbuf_t;

/* release files job */
typedef struct releasebuf_s {
	sqm_lst_t *filepaths;
	int32_t options;
	int32_t partial_sz;
} releasebuf_t;


/* archive files job */
typedef struct archivebuf_s {
	sqm_lst_t *filepaths;
	int32_t options;
} archivebuf_t;

/* run explorer job */
typedef struct explorerbuf_s {
	char location[MAXPATHLEN];
	int  log_lines;
} explorerbuf_t;

/* stage files job */
typedef struct stagebuf_s {
	sqm_lst_t *filepaths;
	int32_t options;
} stagebuf_t;

typedef struct dispatchbuf_s {
	void *job;
} dispatchbuf_t;


/* Define a union of all the above structures */
typedef union {
	decombuf_t d;		/* SAMRTHR_D */
	srchbuf_t s;		/* SAMRTHR_S */
	restbuf_t r;		/* SAMRTHR_R */
	dumpbuf_t u;		/* SAMRTHR_U */
	releasebuf_t rl;	/* SAMA_RELEASE_FILES */
	archivebuf_t a;		/* SAMA_ARCHIVE_FILES */
	explorerbuf_t e;	/* SAMA_RUN_EXPLORER */
	stagebuf_t st;		/* SAMA_STAGE_FILES */
	dispatchbuf_t db;	/* SAMA_DISPATCH_JOB */
} argbuf_t;

void free_argbuf(int type, argbuf_t *args);


/* Datastructure to keep track of threads */

typedef struct samrthread_s {
	struct samrthread_s *next; /* list links */
	struct samrthread_s *prev;
	int (*details)(struct samrthread_s *ptr, char **result);
	int (*destroy)(struct samrthread_s *ptr);
	pid_t pid;		/* Process we need to wait for */
	pthread_t tid;		/* Thread id running this code */
	char *jobid;		/* Job string we returned to caller */
	time_t start;		/* Time thread was created */
	int type;		/* Type of argument structure following */
	argbuf_t *args;		/* Pointer to per-type argument structure */
} samrthread_t;

/* Internal routines to initiate and close activity datastructures */

/*
 * end_this_activity() is to be called by an activity thread to clean
 * up it's activity database and exit the thread cleanly
 */

void
end_this_activity(
	char *jobid);

/*
 * start_activity() allocates the datastructure describing a thread/fork.
 */

int
start_activity(
	int (details)(samrthread_t *, char **),	/* Routine to describe job */
	int (destroy)(samrthread_t *),		/* Routine to kill job */
	activitytype_t type,			/* Activity type */
	argbuf_t   *args,			/* Argument (optional) */
	char	**jobidp);			/* Returned activity ID */


/*
 * kill_fork() kills off a forked job controlled by activity database.
 * Often passed as "destroy" argument to start_activity().
 */
int
kill_fork(
	samrthread_t *);	/* Activity database descriptor */

/*
 * Array of process names. Defined in job_control.c to avoid replicating.
 */
extern char *activitytypes[];

/* Function to set the pid and or tid samrthread_t fields */
int
set_pid_or_tid(
	char		*jobid,
	pid_t		pid,
	pthread_t	tid);


/* Function to get the arguments associated with an activity */
argbuf_t *
samr_get_args(
	char	*jobid);

/*
 * Global to set the thread creation attribute
 * detachstate so all threads created by samr are created
 * PTHREAD_CREATE_DETACHED (no need to join)
 *
 * Initialization is done in a pthread_once function called
 * when start_activity is called.  IF threads are to be created
 * detached outside of the activity list paradigm, then this
 * pthread_once call should be made somewhere else or that
 * function will need to set the pthread attributes itself.
 */

pthread_attr_t 		samr_pth_attr;


/*
 * Argument to cleanup_after_exec_get_output
 */
typedef struct exec_cleanup_info {
	pid_t pid;
	char job_id[MAXNAMELEN];
	char func[128];
	FILE *streams[2];
} exec_cleanup_info_t;

/*
 * Function to wait for an activity exec'ed by exec_get_output to exit
 * and to clean up after it does.
 */
void * cleanup_after_exec_get_output(void *arg);


/*
 * Wait for up to <threshold> seconds for a exec-ed process to finish.
 * If the process completes the cleanup function is called to cleanup
 * and the status is returned.
 *
 * If the procees did not complete the cleanup function is launched in a new
 * thread to wait for the process to complete asynchronously and control
 * is retuned to the caller with a -2 return.
 *
 * The cleanup function must continue to wait on the pid and cleanup
 * after the process exits by closing file descriptors, freeing any
 * associated memory and calling end_this_activity for the job.
 *
 * start_activity must be called prior to calling this function.
 *
 * Returns:
 *	 0  process exited successfully, cleanup_child has been called
 *	-1  process exited with an error, cleanup_child has been called
 *	-2  process did not yet complete. The cleanup function has been
 *	    called to wait for it.
 */
int
bounded_activity_wait(int *status, int threshold, char *jobid,
    pid_t pid, void *cl_args, void * (*cleanup_child)(void *));




#endif	/* _PROCESS_JOB_H */
