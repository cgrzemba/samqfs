/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident	"$Revision: 1.28 $"

#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <pthread.h>
#include <fcntl.h>
#include <wait.h>
#include "pub/mgmt/types.h"
#include "pub/mgmt/error.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "pub/mgmt/restore.h"
#include "pub/mgmt/process_job.h"

#define	BUFFSIZ		MAXPATHLEN

extern pthread_mutex_t search_mutex;
extern int search_active;
extern uint64_t restore_max;
extern uint64_t restore_cur;

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* Utility routine to display dump job status */
int
dumplist(samrthread_t *ptr, char **result)
{
	char buffer[BUFFSIZ*3];

	snprintf(buffer, sizeof (buffer), "activityid=%s,starttime=%d,"
	    "fsname=%s,dumpname=%s,type=%s",
	    ptr->jobid, ptr->start, ptr->args->u.fsname,
	    ptr->args->u.dumpname, activitytypes[ptr->type]);
	*result = copystr(buffer);
	return (0);
}

void *
dumpwait(void* jobid)
{
	argbuf_t *arg;
	int status;
	FILE *logfil = NULL;
	int fd;
	char catmsg[MAX_MSGBUF_SIZE];

	arg = samr_get_args((char *)jobid);

	/* Wait for fork to finish */
	while (waitpid(arg->u.pid, &status, 0) < 0) {
		if (errno != EINTR) {
			break;
		}
	}

	/* If samfsdump failed, log the fact and remove the dump */
	if ((!WIFEXITED(status)) || (WEXITSTATUS(status) != 0)) {
		snprintf(catmsg, sizeof (catmsg), GetCustMsg(SE_DUMP_FAILED),
		    arg->u.dumpname);

		if ((fd = open(RESTORELOG, O_WRONLY|O_APPEND|O_CREAT, 0644))
		    != -1) {
			logfil = fdopen(fd, "a");
		}

		if (logfil != NULL) {
			rlog(logfil, catmsg, arg->u.dumpname, NULL);
			fclose(logfil);
		}

		strlcat(catmsg, " ", sizeof (catmsg));
		strlcat(catmsg, GetCustMsg(SE_SEE_RESTORE_LOG),
		    sizeof (catmsg));
		PostEvent(DUMP_CLASS, DUMP_INTERRUPTED_SUBCLASS,
		    SE_DUMP_FAILED, LOG_ERR,
		    catmsg, NOTIFY_AS_FAULT | NOTIFY_AS_EMAIL);

		unlink(arg->u.dumpname);
	}

	end_this_activity((char *)jobid);	/* Clean up */

	free(arg);

	return (NULL);
}


/* Utility routine to display status of indexing/decompression task */
int
decomlist(samrthread_t *ptr, char **result)
{
	char buffer[BUFFSIZ*2];

	snprintf(buffer, sizeof (buffer), "activityid=%s,starttime=%d,"
	    "fsname=%s,details=%s,type=%s,pid=%d",
	    ptr->jobid, ptr->start, ptr->args->d.fsname,
	    ptr->args->d.dumppath, activitytypes[ptr->type], ptr->pid);
	*result = copystr(buffer);

	return (0);
}

/* Utility routine to kill off a indexing/decompress task */

int
decomkill(samrthread_t *ptr)
{
	if (ptr->tid)		/* Cancel controlling process */
		pthread_cancel(ptr->tid);

	if (ptr->pid)		/* Cancel any subsidiary fork */
		kill_fork(ptr);

	return (0);
}

void
decomcleanup(void *ptr) {
	argbuf_t *arg = ptr;
	char *freeable_id;

	/*
	 * Note that end_this_activity frees its input arg so dup the jobid
	 * before freeing the argbuf
	 */
	freeable_id = strdup(arg->d.xjobid);
	free_argbuf(SAMRTHR_D, arg);
	end_this_activity(freeable_id);
}

/* Utility routine to display status of restore task */
int
restorelist(samrthread_t *ptr, char **result)
{
	char buffer[BUFFSIZ*3];
	int  copy = *(int *)ptr->args->r.copies->head->data;

	snprintf(buffer, sizeof (buffer), "activityid=%s,starttime=%ld,"
	    "fsname=%s,dumpname=%s,type=%s,filename=%s,"
	    "filestodo=%llu,filesdone=%llu,replaceType=%d,"
	    "destname=%s,copy=%d",
	    ptr->jobid, ptr->start, ptr->args->r.fsname,
	    ptr->args->r.dumpname, activitytypes[ptr->type],
	    ptr->args->r.filepaths->head->data,
	    restore_max, restore_cur, ptr->args->r.replace,
	    ptr->args->r.dest->head->data, copy);
	*result = copystr(buffer);

	return (0);
}

int
restorekill(samrthread_t *ptr)
{
	Trace(TR_MISC, "Killing restore thread %d", ptr->tid);
	pthread_cancel(ptr->tid);
	return (0);
}

/* Pthread cancellation handler. Clean up restore on exit */

void
restorecleanup(void *ptr)
{
	argbuf_t *arg = ptr;	/* Argument block in use */
	char *freeable_id;

	Trace(TR_MISC, "Cancelling restore job; ptr %p", ptr);

	/* close the dump file, index and logfile */
	close_dump(arg->r.dsp);

	pthread_mutex_lock(&search_mutex);
	search_active = 0;		/* Indicate search/restore is done */
	pthread_mutex_unlock(&search_mutex);

	/*
	 * Note that end_this_activity frees its input arg so dup the jobid
	 * before freeing the argbuf. Unless that changes.
	 */
	freeable_id = strdup(arg->r.xjobid);

	free_argbuf(SAMRTHR_R, arg);

	end_this_activity(freeable_id);
}
