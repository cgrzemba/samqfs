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
#pragma ident	"$Revision: 1.31 $"

#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fnmatch.h>
#include <pthread.h>
#include <wait.h>
#include <errno.h>
#include <signal.h>
#include <pub/mgmt/types.h>
#include <pub/mgmt/sqm_list.h>
#include <pub/mgmt/error.h>
#include <sam/sam_trace.h>
#include "mgmt/util.h"
#include <pub/mgmt/restore.h>
#include <pub/mgmt/process_job.h>
#include "mgmt/cmd_dispatch.h"

#define	BUFFSIZ		MAXPATHLEN

/* Variables to keep track of threads */

/* List of active threads and mutex to control them */
static samrthread_t *samrtlist = NULL;
static pthread_mutex_t samrlock_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t *samrlock = &samrlock_mutex;

pthread_once_t samr_once_control = PTHREAD_ONCE_INIT;
static void samr_once(void);

static samrthread_t *find_this_activity(char *jobid);

int samrjobnum = 0x80000;	/* jobid to return to user */

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* List of activity type names. */

char *activitytypes[activitytypemax] = {
	SAMRNONE,		/* SAMRTHR_E */
	SAMRDECOMPRESS,		/* SAMRTHR_D */
	SAMRSEARCH,		/* SAMRTHR_S */
	SAMRRESTORE,		/* SAMRTHR_R */
	SAMRCRONTAB,		/* SAMRTHR_C */
	SAMRDUMP,		/* SAMRTHR_U */

	SAMDFSD,		/* SAMD_FSD */
	SAMDARCHIVERD,		/* SAMD_ARCHIVERD */
	SAMDARFIND,		/* SAMD_ARFIND */
	SAMDSTAGERD,		/* SAMD_STAGERD */
	SAMDSTAGEALLD,		/* SAMD_STAGEALLD */
	SAMDAMLD,		/* SAMD_AMLD */
	SAMDSCANNERD,		/* SAMD_SCANNERD */
	SAMDCATSERVERD,		/* SAMD_CATSERVERD */
	SAMDMGMTD,		/* SAMD_MGMTD */
	SAMDROBOTSD,		/* SAMD_ROBOTSD */
	SAMDGENERICD,		/* SAMD_GENERICD */
	SAMDSTKD,		/* SAMD_STKD */
	SAMDIBM3494D,		/* SAMD_IBM3494D */
	SAMDSONYD,		/* SAMD_SONYD */

	SAMDSHAREFSD,		/* SAMD_SHAREFSD */
	SAMDRMTSERVER,		/* SAMD_RMTSERVERD */
	SAMDRMTCLIENT,		/* SAMD_RMTCLIENTD */
	SAMDRFT,		/* SAMD_RFTD */
	SAMDRELEASER,		/* SAMD_RELEASERD */
	SAMDRECYCLER,		/* SAMD_RECYCLERD */

	SAMPFSCK,		/* SAMP_FSCK */
	SAMPTPLABEL,		/* SAMP_TPLABEL */
	SAMPMOUNT,		/* SAMP_MOUNT */
	SAMPUMOUNT,		/* SAMP_UMOUNT */
	SAMPSAMMKFS,		/* SAMP_SAMMKFS */
	SAMPARCHIVERCLI,	/* SAMP_ARCHIVERCLI */

	SAMARELEASEFILES,	/* SAMA_RELEASEFILES */
	SAMAARCHIVEFILES,	/* SAMA_ARCHIVEFILES */
	SAMARUNEXPLORER,	/* SAMA_RUNEXPLORER */
	SAMASTAGEFILES,		/* SAMA_STAGEFILES */
	SAMADISPATCHJOB		/* SAMA_DISPATCHJOB */
};

/* RPC callable routine to get a list of what's going on */

int
list_activities(
	ctx_t *c,		/* ARGSUSED */
	int maxentries,		/* Maximum number of strings to return */
	char *rest,		/* fnmatch pattern to exclude returns */
	sqm_lst_t **activities)	/* Returned list of descriptor strings */
{
	samrthread_t *ptr;
	char *bufptr;
	int rval;
	sqm_lst_t *l;
	char restrictions[MAXPATHLEN];


	/* Fully wild-card selector */
	if (rest != NULL)
		snprintf(restrictions, MAXPATHLEN, "*%s*", rest);
	else
		strlcpy(restrictions, "*", MAXPATHLEN);

	*activities = lst_create();
	pthread_mutex_lock(samrlock); /* ++ LOCK samrtlist */

	ptr = samrtlist;
	while ((ptr != NULL) && ((*activities)->length < maxentries)) {
		rval = ptr->details(ptr, &bufptr); /* Ask about a task */
		if (rval == 0) {
			if (fnmatch(restrictions, bufptr, 0))
				free(bufptr); /* Doesn't match */
			else
				lst_append(*activities, bufptr);
		}
		ptr = ptr->next;
	}
	pthread_mutex_unlock(samrlock); /* ++ UNLOCK samrtlist */

	/* get and append process jobs that match the restrictions */
	if (get_process_jobs(NULL, rest, &l) != 0) {
		return (-1);
	} else if (l->length != 0) {
		if (lst_concat(*activities, l) != 0) {
			return (-1);
		}
	}

	return (0);
}

/* RPC callable routine to kill off an existing task */

int
kill_activity(
	ctx_t *c,		/* ARGSUSED */
	char *activityid,	/* identifying string of process to kill */
	char *type)		/* to verify correct process selected */
{
	samrthread_t *ptr;

	Trace(TR_MISC, "Requesting kill of activity %s", activityid);

	pthread_mutex_lock(samrlock);	/* ++ LOCK samrtlist */
	ptr = find_this_activity(activityid);

	/* If we didn't find ourselves, return an error */
	if (ptr == NULL) {
		pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
		return (samrerr(SE_NO_SUCH_ACTIVITY, activityid));
	}

	/*
	 * Call the destroy routine if it exists, otherwise consider the job
	 * uncancelable.
	 */
	if (ptr->destroy == NULL) {
		pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
		return (0);
	}

	/*
	 * The activity exists and has a destroy function, so remove it from
	 * the list, then kill it
	 */
	if (ptr->prev) {
		ptr->prev->next = ptr->next;
	} else {			/* we're top of the list */
		samrtlist = ptr->next;
	}
	if (ptr->next) {
		ptr->next->prev = ptr->prev;
	}
	pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */

	/* destroy must be called outside of the lock */
	if (ptr->destroy(ptr) != 0) {
		Trace(TR_ERR, "kill activity failed %s", activityid);
	}

	/*
	 * Note that currently any type specific malloced fields in args
	 * must be freed in the cleanup(destroy) functions
	 */
	if (ptr->args != NULL)
		free(ptr->args);
	if (ptr->jobid != NULL)
		free(ptr->jobid);
	free(ptr);		/* Free rest of datastructure */

	return (0);
}

/*
 * Utility routines to handle fork creation and state transitions
 */

/*
 * Activity is ending, free up datastructure, end thread AND
 * free the input argument jobid
 */
void
end_this_activity(
	char *jobid)	/* !free(jobid) called within! */
{
	samrthread_t	*ptr = NULL;

	pthread_mutex_lock(samrlock);		/* ++ LOCK samrtlist */

	ptr = find_this_activity(jobid);

	if (ptr != NULL) {
		if (ptr->prev) {
			ptr->prev->next = ptr->next;
		} else {			/* we're top of the list */
			samrtlist = ptr->next;
		}
		if (ptr->next) {
			ptr->next->prev = ptr->prev;
		}
		/*
		 * NOTE: even though some args contain pointers, do not free
		 * the data pointed to here. With the current model that
		 * data is shared with any callers of samr_get_args.
		 */
		if (ptr->args != NULL)
			free(ptr->args);

		if (ptr->jobid != NULL)
			free(ptr->jobid);
		free(ptr);		/* Free rest of datastructure */
	}

	pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */

	/* Consider pushing the responsiblity for this to the caller */
	free(jobid);

}

/* Utility process to kill off a fork job when necessary */
int
kill_fork(samrthread_t *ptr) {
	int rval;

	rval = signal_process(ptr->pid, NULL, SIGTERM);

	return (rval);
}



/* Utility routine to create a fork/thread descriptor datastructure */
/* to track processes. */
int
start_activity(
	int (details)(samrthread_t *, char **),
	int (destroy)(samrthread_t *),
	activitytype_t type,
	argbuf_t   *args,
	char  **jobidp)
{
	samrthread_t *samtp;
	char jobstr[BUFFSIZ];

	/* set up the pthread attributes to be used hereafter */
	pthread_once(&samr_once_control, samr_once);

	/* Create samrthread_t structure */

	samtp = mallocer(sizeof (samrthread_t));
	if (samtp == NULL)
		return (-1);

	/* Initialize */
	samtp->start = time(NULL); 	/* Start time */
	samtp->pid = 0;
	samtp->tid = 0;
	samtp->type = type;		/* specified by caller */
	samtp->details = details;
	samtp->destroy = destroy;
	samtp->args = args;
	samtp->next = NULL;
	samtp->prev = NULL;

	/* Link structure into active list */
	pthread_mutex_lock(samrlock);	/* ++ LOCK */
	samrjobnum++;			/* ++ Allocate a jobid */
	snprintf(jobstr, BUFFSIZ, "%d", samrjobnum); /* ++ */
	samtp->jobid = copystr(jobstr);	/* Keep copy of ascii string */

	/*
	 * copy the jobid prior to insertion into list so if copy fails
	 * it is easier to cleanup
	 */
	*jobidp = copystr(jobstr);

	if (samtp->jobid == NULL || *jobidp == NULL) {
		if (*jobidp) {
			free(*jobidp);
			*jobidp = NULL;
		}
		if (samtp->jobid) {
			free(samtp->jobid);
		}
		free(samtp);

		/* caller is responsible for freeing any malloced args */

		pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
		return (-1);
	}

	if (samrtlist == NULL) {
		samrtlist = samtp;
	} else {
		/* Insert structure into list */
		samtp->next = samrtlist;
		samrtlist->prev = samtp;
		samrtlist = samtp;
	}
	pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */

	return (0);
}


/* Function to set the pid and or tid samrthread_t fields */
int
set_pid_or_tid(
	char		*jobid,
	pid_t		pid,
	pthread_t	tid)
{

	samrthread_t	*ptr;

	if (jobid == NULL) {
		return (samrerr(SE_NO_SUCH_ACTIVITY, 0));
	}

	pthread_mutex_lock(samrlock);

	ptr = find_this_activity(jobid);

	if (ptr != NULL) {
		if (pid) {
			ptr->pid = pid;
		}
		if (tid) {
			ptr->tid = tid;
		}
	}

	pthread_mutex_unlock(samrlock);

	if (ptr == NULL) {
		return (samrerr(SE_NO_SUCH_ACTIVITY, jobid));
	}

	return (0);
}

/* Function to get the arguments associated with an activity */
argbuf_t *
samr_get_args(
	char 		*jobid)
{
	argbuf_t	*retarg = NULL;
	samrthread_t	*ptr;

	if (jobid == NULL) {
		return (NULL);
	}

	pthread_mutex_lock(samrlock);

	ptr = find_this_activity(jobid);

	if ((ptr) && (ptr->args)) {
		retarg = mallocer(sizeof (*(ptr->args)));
	}
	/*
	 * This seems dubious because some args contain pointers to
	 * other data. This will work as long as the elements are not
	 * modified and care is taken to not double free them.
	 */
	if (retarg) {
		memcpy(retarg, ptr->args, sizeof (*(ptr->args)));
	}

	pthread_mutex_unlock(samrlock);

	return (retarg);
}


/*
 * Function to set up a pthread_attr_t so all threads created
 * by samr are created PTHREAD_CREATE_DETACHED (no need to join)
 */
static void
samr_once(
	void)
{

	pthread_attr_init(&samr_pth_attr);

	pthread_attr_setdetachstate(&samr_pth_attr, PTHREAD_CREATE_DETACHED);
}

/*
 * Function to find the samrthread_t structure associated with
 * a given jobid.  Must be called with the samrlock mutex locked.
 */
static samrthread_t *
find_this_activity(
	char	*jobid)
{

	samrthread_t 	*ptr = NULL;

	if (jobid == NULL) {
		return (NULL);
	}

	ptr = samrtlist;

	while (ptr != NULL) {
		if (strcmp(jobid, ptr->jobid) == 0) {
			break;
		}
		ptr = ptr->next;
	}

	return (ptr);
}


/*
 * Function to determine if a decompress job is already in progress for a
 * specific dump. Walks through list of activities to see if in progress.
 */

int
decomfind(char *dumpname) {
	samrthread_t	*ptr;
	char		buf[MAXPATHLEN + 1];
	char		*bufp;


	if ((dumpname == NULL) || (dumpname[0] == '\0')) {
		return (0);
	}

	/* strip off the .dmp[xxx] part of the provided path */
	strlcpy(buf, dumpname, sizeof (buf));
	while ((bufp = strrchr(buf, '.')) != NULL) {
		*bufp = '\0';
		bufp++;
		if ((strncmp(bufp, "dmp", 3)) == 0) {
			break;
		}
	}

	pthread_mutex_lock(samrlock);

	ptr = samrtlist;
	while (ptr != NULL) {
		if ((ptr->type == SAMRTHR_D) &&
		    (strcmp(ptr->args->d.dumppath, buf) == 0))
			break;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(samrlock);
	if (ptr)
		return (1);	/* Found a decompress with that name */
	else
		return (0);	/* No decompress with that name */
}

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
bounded_activity_wait(
int *status,
int threshold,
char *jobid,
pid_t pid,
void *cl_args,
void * (*cleanup_child)(void *))
{

	/* variables to control the transition to asynchronous */
	int		total_wait = 0;
	int		ret_val = 0;
	pthread_t	tid;
	timespec_t	waitspec = {1, 0};

	Trace(TR_DEBUG, "about to enter wait for process %ld", pid);
	if (ISNULL(status, jobid, cl_args, cleanup_child)) {
		Trace(TR_ERR, "bounded wait err: %d %s", samerrno, samerrmsg);
		return (-1);
	}
	/*
	 * wait for the process without hanging and leave it as you
	 * found it so the cleanup function can handle its completion
	 * error as it sees fit.
	 */
	for (;;) {
		ret_val = waitpid(pid, status, WNOHANG | WNOWAIT);
		if (ret_val == -1 && errno != EINTR) {
			/* waitpid hit an error */
			break;
		} else if (ret_val == pid) {
			/* the process is done */
			break;
		}

		nanosleep(&waitspec, NULL);
		total_wait += 1;

		if (total_wait >= threshold) {
			Trace(TR_MISC, "going asynch for %s(%ld)",
			    Str(jobid), pid);

			/* go asynchronous */
			pthread_create(&tid, &samr_pth_attr, cleanup_child,
			    cl_args);

			set_pid_or_tid(jobid, 0, tid);
			return (-2);
		}
	}

	/* the process either exited (error or success) or waitpid failed */

	/* call the cleanup function in this thread */
	cleanup_child(cl_args);

	if (ret_val < 0 || !WIFEXITED(*status) || WEXITSTATUS(*status)) {
		/*
		 * wait failed OR process abnormally terminated OR
		 * process exited with non zero exit code
		 */
		return (-1);
	} else {
		return (0);
	}
}

/*
 * Helper function to wait for processes execed by
 * exec_get_output to complete and to close the output
 * and error pipes.
 */
void *
cleanup_after_exec_get_output(void *arg)
{
	int status;
	exec_cleanup_info_t *cl = (exec_cleanup_info_t *)arg;

	Trace(TR_MISC, "cleanup about to wait for %s(%ld)",
	    cl->func, cl->pid);

	/* wait for completion */
	while (waitpid(cl->pid, &status, 0) < 0) {
		if (errno != EINTR) {
			Trace(TR_ERR, "waitpid %s(%ld) returned: %d\n",
			    cl->func, cl->pid, errno);
			break;
		}
	}

	/* nobody is around for a return so simply trace exit status */
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			Trace(TR_ERR, "cleanup for %s(%ld) exited: %d\n",
			    cl->func, cl->pid, WEXITSTATUS(status));

		} else {
			Trace(TR_MISC, "cleanup for %s exited success",
			    cl->func);
		}
	} else {
		Trace(TR_ERR, "cleanup %s(%ld) %s:%d %d", cl->func, cl->pid,
		    "abnormally terminated/stopped", WIFSTOPPED(status),
		    (WIFSIGNALED(status) ? WTERMSIG(status) : 0));
	}

	/* close streams and deallocate arg */
	if (cl->streams[0]) {
		fclose(cl->streams[0]);
	}
	if (cl->streams[1]) {
		fclose(cl->streams[1]);
	}

	/* dup the jobid because end_this_activity will free it */
	end_this_activity(strdup(cl->job_id));
	free(cl);
	Trace(TR_MISC, "clean up exiting");

	return (NULL);
}


void
free_argbuf(int type, argbuf_t *args) {
	if (args != NULL) {

		/*
		 * Switch on type and handle any type specific malloced
		 * fields.
		 */
		switch (type) {
		case SAMRTHR_R:
			if (args->r.filepaths) {
				lst_free_deep(args->r.filepaths);
				args->r.filepaths = NULL;
			}
			if (args->r.dest) {
				lst_free_deep(args->r.dest);
				args->r.dest = NULL;
			}
			if (args->r.copies) {
				lst_free_deep(args->r.copies);
				args->r.copies = NULL;
			}
			if (args->r.dsp) {
				free(args->r.dsp);
				args->r.dsp = NULL;
			}

			break;
		case SAMA_RELEASEFILES:
			if (args->rl.filepaths) {
				lst_free_deep(args->rl.filepaths);
			}
			break;
		case SAMA_ARCHIVEFILES:
			if (args->a.filepaths) {
				lst_free_deep(args->a.filepaths);
			}
			break;
		case SAMA_STAGEFILES:
			if (args->a.filepaths) {
				lst_free_deep(args->a.filepaths);
			}
			break;
		}
		/*
		 * Handles all arg bufs that don't contain
		 * malloced data:
		 * arg.d, arg.s, arg.u, args.e
		 */
		free(args);
	}
}


/*
 * Function to update the job struct when a host has been called
 * asynchronously and did not return an error on the initial call.
 * Note: this function is in job_control because it needs access to the
 * samrlock mutex.
 */
int
calling_host(char *job_id, char *host, int host_num) {

	samrthread_t *ptr;
	dispatch_job_t *dj;

	if (ISNULL(job_id, host)) {
		Trace(TR_ERR, "failed to find the job: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "updating dispatch job %s with response from %s", job_id,
	    host);

	pthread_mutex_lock(samrlock);	/* ++ LOCK samrtlist */
	ptr = find_this_activity(job_id);

	/* If we didn't find the job, return an error */
	if (ptr == NULL) {
		pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
		samrerr(SE_NO_SUCH_ACTIVITY, job_id);
		Trace(TR_ERR, "failed to find the job: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	if (ptr->type != SAMA_DISPATCH_JOB) {
		pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
		setsamerr(SE_INVALID_ACTIVITY_TYPE);
		Trace(TR_ERR, "failed to find the job: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	dj = (dispatch_job_t *)ptr->args->db.job;
	if (host_num >= dj->host_count) {
		pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
		setsamerr(SE_INVALID_HOST_ID_IN_RESPONSE);
		Trace(TR_ERR, "Host number outside of range");
		return (-1);
	}
	dj->hosts_called++;


	/*
	 * Only upgrade the status if a higher status has not been set.
	 * Otherwise leave it alone since there is no guarantee that host_called
	 * will be executed prior to receiving an asychronous response.
	 */
	if (dj->responses[host_num].status == OP_NOT_YET_CALLED) {
		dj->responses[host_num].status = OP_PENDING;
	}
	if (dj->overall_status == DJ_INITIALIZING) {
		dj->overall_status = DJ_PENDING;
	}
	pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */

	return (0);
}



/*
 * Function to call when a host responds. It would also be appropriate
 * if the responses are asynchronous but the host returns an error on
 * the initial call to handle_request
 *
 * If passed with error_number != 0 the result will be
 * interpreted as an error. Otherwise it is interpreted as a
 * response.
 */
int
host_responded(
char *job_id,
char *host,
int host_num,
int error_number,
char *result) {

	samrthread_t *ptr;
	dispatch_job_t *dj;

	/* result is checked for NULL later */
	if (ISNULL(job_id, host)) {
		Trace(TR_ERR, "failed to find the job: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "updating dispatch job %s with response from %s",
	    job_id, host);

	pthread_mutex_lock(samrlock);	/* ++ LOCK samrtlist */
	ptr = find_this_activity(job_id);

	if (ptr == NULL) {
		pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
		Trace(TR_ERR, "failed to find the job: %d %s",
		    samerrno, samerrmsg);

		return (samrerr(SE_NO_SUCH_ACTIVITY, job_id));
	}

	dj = (dispatch_job_t *)ptr->args->db.job;

	/* Set the error or response for the current host in the job */
	if (host_num < dj->host_count) {
		/* Increment the hosts that have responded */
		dj->hosts_responded++;


		dj->responses[host_num].error = error_number;
		if (error_number == 0) {
			dj->responses[host_num].status = OP_SUCCEEDED;
		} else {
			dj->responses[host_num].status = OP_FAILED;
		}

		/* A result is not mandatory */
		if (result != NULL) {
			dj->responses[host_num].result = copystr(result);
			if (dj->responses[host_num].result == NULL) {
				pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
				Trace(TR_ERR, "error duplicating result: %d %s",
				    samerrno, samerrmsg);
				return (-1);
			}
		}
	} else {
		pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */
		Trace(TR_ERR, "Host number outside of range");
		setsamerr(SE_INVALID_HOST_ID_IN_RESPONSE);
		return (-1);
	}


	/*
	 * If all hosts have responded, call the post_phase handler to do any
	 * necessary work on this host and set the end time and
	 * overall status.
	 */
	if (dj->hosts_responded == dj->host_count) {
		if (dj->post_phase != NULL) {
			int ret;

			dj->overall_status = DJ_POST_PHASE_PENDING;

			/*
			 * It would be good to do the post phase handling
			 * with the mutex unlocked but it is not currently
			 * safe to do so.
			 */
			ret = dj->post_phase(dj);

			/* Set status */
			if (ret == -1) {
				dj->overall_status = DJ_POST_PHASE_FAILED;
				dj->overall_error_num = samerrno;
				dj->overall_error_msg = copystr(samerrmsg);
			} else {
				dj->overall_status = DJ_POST_PHASE_SUCCEEDED;
			}
			dj->endtime = time(0);

		} else {
			dj->overall_status = DJ_DONE;
			dj->endtime = time(0);
		}
	}


	pthread_mutex_unlock(samrlock);	/* ++ UNLOCK */

	return (0);
}
