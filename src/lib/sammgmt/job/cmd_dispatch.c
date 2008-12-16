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
#pragma ident   "$Revision: 1.5 $"


static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <rpc/rpc.h>
#include <rpc/clnt_stat.h>
#include <rpc/xdr.h>
#include <thread.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/time_impl.h>
#include <sys/errno.h>
#include <errno.h>
#include "sam/sam_trace.h"
#include "mgmt/sammgmt.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "mgmt/util.h"
#include "mgmt/cmd_dispatch.h"
#include "pub/mgmt/process_job.h"


/* Structure to store the thread pool information */
typedef struct tpool_s {
	pthread_t	*threads;
	int		max;
	int		current;	/* number of threads in pool */
	int		started;	/* number of threads in pool */
} tpool_t;


#define	POST_PHASE_EXECUTING	0x1
#define	POST_PHASE_FAILED	0x2
#define	POST_PHASE_SUCCEEDED	0x4
#define	POST_PHASE_EXECUTED	0x6


/*
 * Structure to store the work created from the user request to
 * perform a operation on multiple shared clients. When freeing this
 * struct the caller must be aware that the targets array, args and
 * job_id are kept as part of the dispatch job and must not be freed
 * with this struct
 */
typedef struct wpool_s {
	pthread_mutex_t	mutex;
	pthread_cond_t	cond_que;		/* wait for work to be queued */
	tpool_t		*tpool; 		/* thread pool information */
	int		index;			/* processed input count */
	int		qsize;			/* queue size */
	int		quit;			/* quit if done (not cancel) */
	int		target_count;		/* target count */
	char		**targets;		/* hosts to call- do not free */
	int		func_id;
	char		*job_id;		/* job id - do not free */
	void		*args;			/* args - do not free */
	int		(*do_work)(op_req_t *arg, char *host);
} wpool_t;



static int display_multihost_job_status(samrthread_t *ptr, char **result);

static int get_type_specific_output(dispatch_job_t *dj, char *buf,
    int buf_size);

static void free_response_array(op_rsp_t *responses, int count);

static int wpool_init(int req_type, char *job_id, void *args, char **targets,
    int target_count, int tpool_size, wpool_t **wp);

static int wpool_destroy(wpool_t *wp);

static void *worker_thread(void *arg);

static void strip_newlines(char *str);

static int get_operation_error(dispatch_id_t func_id, boolean_t partial);

static int remove_dispatch_job(struct samrthread_s *ptr);

static samrpc_client_t *setup_rpc_client(char *host, struct timeval *tm);

static int destroy_rpc_client(samrpc_client_t *clnt);

static void free_tpool(tpool_t *tpool);

static void free_wpool(wpool_t *wpool);


char *dipspatch_op_names[MAX_DISPATCH_ID] = {
	DSP_MOUNT,
	DSP_UMOUNT,
	DSP_CHANGE_MOUNT_OPTIONS,
	DSP_ADD_CLIENTS,
	DSP_GROW_FS,
	DSP_REMOVE_CLIENTS,
	DSP_SET_ADV_NET_CONFIG,
};




static pthread_once_t dsp_attr_once_control = PTHREAD_ONCE_INIT;
static void dsp_attr_once(void);
static pthread_attr_t	dsp_attr;	/* detach threads */


int
send_request(op_req_t *req, char *host) {
	char *func_name = "send_request";
	samrpc_client_t	*samrpc_clnt	= NULL;
	samrpc_result_t	result;
	struct timeval	tm;
	enum clnt_stat	stat;
	ctx_t		ctx;
	char		*err_msg = NULL;
	int		ret_val = 0;
	handle_request_arg_t hr_arg;


	tm.tv_sec	= 120;
	tm.tv_usec	= 0;


	if (ISNULL(req, host)) {
		Trace(TR_ERR, "snd:sending request failed:%d %s",
		    samerrno, samerrmsg);
		ret_val = -1;
		goto err;
	}

	Trace(TR_MISC, "snd:sending request to: %s", host);

	if (req->func_id < 0 || req->func_id >= MAX_DISPATCH_ID) {
		samerrno = SE_FUNCTION_NOT_SUPPORTED;
		snprintf(samerrmsg, MAX_MSG_LEN,  GetCustMsg(samerrno),
		    req->func_id);
		Trace(TR_ERR, "snd:function not supported %s",
		    host);
		ret_val = -1;
		goto err;
	}


	/* Setup the result structs */
	memset((char *)&result, 0, sizeof (result));


	samrpc_clnt = setup_rpc_client(host, &tm);
	if (samrpc_clnt == NULL) {
		Trace(TR_ERR, "snd:Sending Request failed:%d %s-> %s",
		    samerrno, samerrmsg, host);
		ret_val = -1;
		goto err;
	}

	/*
	 * Setup the argument to handle request. It need not be
	 * malloced unless response handling is made asynchronous.
	 * Do not share the context object with other threads
	 * The host will get overwritten.
	 */
	hr_arg.ctx  = &ctx;
	hr_arg.req = req;
	ctx.handle = samrpc_clnt;
	ctx.dump_path[0] = '\0';
	ctx.read_location[0] = '\0';
	ctx.user_id[0] = '\0';


	/* Check the handle */
	if (!ctx.handle ||
	    !(ctx.handle->clnt) ||
	    !(ctx.handle->svr_name)) {
		samerrno = SE_RPC_INVALID_CLIENT_HANDLE;
		(void) snprintf(samerrmsg, MAX_MSG_LEN,
			"Invalid client handle");
		Trace(TR_ERR, "snd:%s invalid client handle", func_name);
		ret_val = -1;
		goto err;
	}

	if (req->job_id) {
		/*
		 * If handle_request is made asynch this function
		 * should change to host_called and be called after
		 * the remote call.
		 */
		if (calling_host(req->job_id, host, req->host_num) != 0) {
			Trace(TR_ERR, "Internal problem with dispatch job"
			    "setting host-called status failed");
			return (-1);
		}
	}


	Trace(TR_MISC, "snd:do remote call");
	if ((stat = clnt_call(samrpc_clnt->clnt, samrpc_handle_request,
	    (xdrproc_t)xdr_handle_request_arg_t, (caddr_t)&hr_arg,
	    (xdrproc_t)xdr_samrpc_result_t, (caddr_t)&result, tm))
	    != RPC_SUCCESS) {

		Trace(TR_ERR, "snd:Unable to send request to remote client");
		ret_val = -1;
		SERVERSIDE_CLIENT_SET_RPC_ERROR(ctx, func_name, stat, host)
		goto err;
	}

	Trace(TR_MISC, "snd:back from remote call");

	if (result.status != 0) {
		samerrno = result.samrpc_result_u.err.errcode;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    result.samrpc_result_u.err.errmsg);
		Trace(TR_ERR, "%s: %s %s", (host != NULL) ?
			host : "NULL", func_name, samerrmsg);
		ret_val = -1;
	} else {

		if (result.svr_timestamp != SAMRPC_TIMESTAMP_NO_UPDATE) {
			ctx.handle->timestamp = result.svr_timestamp;
		}
		Trace(TR_MISC, "ret_val = %d", result.status);
		ret_val = result.status;
	}


	/*
	 * Note: normal processing intentionally continues
	 * through the code after 'err:'
	 */
err:

	/*
	 * If response processing moves to asynchronous, calling_host
	 * should be changed to host_called and conditionally inserted here.
	 *
	 * If the job completed or returned an error we should call
	 * host_called and host_responded.  If processing is
	 * continuing we should only call host_called
	 *
	 * Note also that with asynch responses there would be a race
	 * to update the job between host_called and host_responded.
	 *
	 * It must be mitigated by handling processing of job
	 * completion either through host_called or host_responded
	 * functions if the state of the job indicates that it is
	 * complete when the function is called.
	 */
	if (req->job_id) {
		if (ret_val == -1) {
			if (host_responded(req->job_id, host, req->host_num,
			    samerrno, samerrmsg) != 0) {
				Trace(TR_ERR, "Internal problem with dispatch"
				    " job setting response failed");
				ret_val = -1;
			}
		}

		/* This if is only needed if response handling is synchronous */
		if (ret_val == 0) {
			if (host_responded(req->job_id, host,
			    req->host_num, 0, NULL) != 0) {
				Trace(TR_ERR, "Internal problem with dispatch"
				    " job setting response failed");
				ret_val = -1;
			}
		}
	}
	destroy_rpc_client(samrpc_clnt);
	return (ret_val);
}



/*
 * This worker thread checks the queue to see if there are any hosts in the
 * host table on which the operation is to be performed. It chooses the next
 * host entry from the host table and sends a remote request.
 *
 * When the thread completes the remote request dispatch, there is either more
 * work to do or the queue is empty depending on the number of entries in the
 * host table (identified by qsize and index)>
 *
 * If the thread finds nothing to do, it will shut down when a certain
 * (predetermined) period of time passes without any new work appearing, or when
 * it has been signalled to be shutdown by the creator.
 *
 * The thread is created with the attribute set to PTHREAD_CREATE_DETACHED so
 * that when the thread terminates, any resources it used can be immediately
 * reclaimed.
 */
static void *worker_thread(void *arg) {

	wpool_t		*wpool		= (wpool_t *)arg;
	pthread_t	tid		= pthread_self();
	int		st;
	op_req_t	op_req;
	char		hostname[128];



	/* Lock mutex to access shared data */
	Trace(TR_DEBUG, "worker[%u] - attempt to lock mutex", tid);

	st = pthread_mutex_lock(&wpool->mutex);
	if (st != 0) {
		Trace(TR_ERR, "worker[%u] - could not lock mutex", tid);
		return (NULL);
	}

	wpool->tpool->current++;	/* number of current threads in pool */
	wpool->tpool->started++;	/* flag to indicate work has started */
	Trace(TR_DEBUG, "worker[%u] - locked mutex", tid);

	for (;;) {
		/* Mutex relocked at end of loop */

		/*
		 * If the queue is empty signal the condition and die
		 */
		if (wpool->qsize == 0) {
			wpool->tpool->current--;
			if (wpool->tpool->current == 0) {
				Trace(TR_DEBUG, "worker[%u] - broadcast signal",
				    tid);
				pthread_cond_broadcast(&wpool->cond_que);
			}
			Trace(TR_DEBUG, "worker[%u] - shut down. current[%d]",
			    tid, wpool->tpool->current);
			Trace(TR_DEBUG, "worker[%u] - unlock mutex", tid);
			pthread_mutex_unlock(&wpool->mutex);
			return (NULL);
		}

		/* pick work from the queue */
		if (wpool->index < wpool->target_count) {
			Trace(TR_DEBUG, "worker[%u] - pick work[%d]: %s",
			    tid, wpool->index, wpool->targets[wpool->index]);

			strlcpy(hostname, wpool->targets[wpool->index], 128);
			op_req.host_num = wpool->index;
			op_req.job_id = wpool->job_id;
			op_req.func_id = wpool->func_id;
			op_req.args = wpool->args;

			wpool->qsize--;
			wpool->index++;

			/* Unlock the mutex to do the actual work */
			Trace(TR_DEBUG, "worker[%u] - unlock mutex", tid);
			st = pthread_mutex_unlock(&wpool->mutex);
			if (st != 0) {
				Trace(TR_ERR,
				    "worker[%u] - could not unlock mutex", tid);
				wpool->tpool->current--;
				return (NULL);
			}

			Trace(TR_MISC, "calling work thread for %s", hostname);

			/* Failures of the do_work are handled inside it */
			(*(wpool->do_work))(&op_req, hostname);

			/* relock the mutex for another itteration */
			st = pthread_mutex_lock(&wpool->mutex);
			if (st != 0) {
				Trace(TR_ERR,
				    "worker[%u] - could not lock mutex", tid);
				return (NULL);
			}
		}

		Trace(TR_DEBUG, "worker[%u] - look for more work[%d]",
		    tid, wpool->qsize);
	}

}


static int
wpool_destroy(wpool_t *wp) {

	int st, st1;

	Trace(TR_DEBUG, "destroy pool - attempt to lock mutex");
	st = pthread_mutex_lock(&wp->mutex);
	if (st != 0) {
		Trace(TR_DEBUG, "wpool_destroy: Failed to lock mutex");
		return (st);
	}
	Trace(TR_DEBUG, "destroy pool - lock mutex");

	/*
	 * If there are still threads working, wait on the
	 * condition variable
	 */
	while (wp->tpool->current > 0 || wp->tpool->started == 0) {
		Trace(TR_MISC, "destroy pool: current threads[%d] started[%d]",
		    wp->tpool->current, wp->tpool->started);
		st = pthread_cond_wait(&wp->cond_que, &wp->mutex);
		if (st != 0) {
			Trace(TR_ERR, "wpool_destroy:"
			    "Failed trying to wait for signal");
			pthread_mutex_unlock(&wp->mutex);
			return (st);
		}
		Trace(TR_DEBUG, "destroy pool: received signal");
	}

	st = pthread_mutex_unlock(&wp->mutex);
	if (st != 0) {
		Trace(TR_ERR, "wpool_destory: Failed to unlock mutex");
		return (st);
	}

	st	= pthread_mutex_destroy(&wp->mutex);
	st1	= pthread_cond_destroy(&wp->cond_que);

	free_wpool(wp);

	Trace(TR_MISC, "destroy pool: exiting");
	return (st ? st : st1);

}

/*
 * Initialize the work pool
 */
static int
wpool_init(
int req_type,
char *job_id,
void *args,
char **targets,
int target_count,
int tpool_size,
wpool_t **wp) {

	int	i;
	int	st;


	if (ISNULL(job_id, args, targets, wp)) {
		Trace(TR_ERR, "Failed to initialize work pool: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "begin initializing work pool");
	*wp = (wpool_t *)mallocer(sizeof (wpool_t));
	if (*wp == NULL) {
		Trace(TR_ERR, "Failed to initialize work pool: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/* Initialize mutex */
	st = pthread_mutex_init(&(*wp)->mutex, NULL);
	if (st != 0) {
		setsamerr(SE_POOL_INIT_FAILED);
		Trace(TR_ERR, "Failed to initialize mutex");
		free(*wp);
		return (st);
	}

	/* Initialize condition variable */
	st = pthread_cond_init(&(*wp)->cond_que, NULL);
	if (st != 0) {
		setsamerr(SE_POOL_INIT_FAILED);
		Trace(TR_ERR, "Failed to initialize condition");
		pthread_mutex_destroy(&(*wp)->mutex);
		free(*wp);
		return (st);
	}

	(*wp)->tpool = (tpool_t *)mallocer(sizeof (tpool_t));
	if ((*wp)->tpool == NULL) {
		Trace(TR_ERR, "Failed to create tpool");
		pthread_mutex_destroy(&(*wp)->mutex);
		pthread_cond_destroy(&(*wp)->cond_que);
		free(*wp);
		return (-1);
	}
	(*wp)->tpool->threads =
	    (pthread_t *)mallocer(sizeof (pthread_t) * tpool_size);

	if ((*wp)->tpool->threads == NULL) {
		pthread_mutex_destroy(&(*wp)->mutex);
		pthread_cond_destroy(&(*wp)->cond_que);
		free((*wp)->tpool);
		Trace(TR_ERR, "Failed to create tpool");
		return (-1);
	}
	(*wp)->tpool->max	= tpool_size;
	(*wp)->tpool->current	= 0;
	(*wp)->tpool->started	= 0;

	(*wp)->index		= 0;
	(*wp)->qsize		= target_count;

	(*wp)->targets		= targets;
	(*wp)->target_count 	= target_count;
	(*wp)->func_id		= req_type;
	(*wp)->job_id		= job_id;

	(*wp)->args		= args;
	(*wp)->do_work		= send_request;

	for (i = 0; i < tpool_size && i <= target_count; i++) {
		st = pthread_create(
		    &((*wp)->tpool->threads[i]),
		    &dsp_attr,
		    worker_thread,
		    (void *)(*wp));
	}

	Trace(TR_DEBUG, "done initializing work pool");
	return (0);
}


int
multiplex_request(
ctx_t		*ctx,
dispatch_id_t	func_id,
char		**target_hosts,
int		target_host_count,
void		*args,
int (post_phase)(dispatch_job_t *)) {

	wpool_t		*wp;
	pthread_t	cleanup_thread;
	int		st;
	char		*job_id;
	int		int_job_id;
	char		*end_ptr;
	argbuf_t	*ab;
	dispatch_job_t	*dj;
	op_rsp_t	*resp;


	if (ISNULL(ctx, target_hosts, args)) {
		Trace(TR_ERR, "multiplex job failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}


	pthread_once(&dsp_attr_once_control, dsp_attr_once);

	/*
	 * Create and populate the dispatch specific activity
	 * argument structure
	 */
	dj = (dispatch_job_t *)mallocer(sizeof (dispatch_job_t));
	if (dj == NULL) {
		Trace(TR_ERR, "multiplex job failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	resp = (op_rsp_t *)calloc(target_host_count, sizeof (op_rsp_t));
	if (resp == NULL) {
		free(dj);
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "multiplex job failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}
	dj->responses = resp;

	dj->func_id = func_id;
	dj->args = args;
	dj->starttime = time(0);
	dj->host_count = target_host_count;
	dj->hosts = target_hosts;
	dj->post_phase = post_phase;
	ab = (argbuf_t *)mallocer(sizeof (dispatchbuf_t));
	if (ab == NULL) {
		/*
		 * set the args and hosts in the dispatch job to NULL
		 * so the caller can free them. This more cleanly
		 * matches the memory management model of the library.
		 */
		dj->hosts = NULL;
		dj->args = NULL;
		free_dispatch_job(dj);
		Trace(TR_ERR, "multiplex job failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}
	ab->db.job = (void *)dj;

	Trace(TR_DEBUG, "Creating a thread to listen for client reponses...");


	if (start_activity(display_multihost_job_status, remove_dispatch_job,
		SAMA_DISPATCH_JOB, ab, &job_id) != 0) {
		dj->args = NULL;
		dj->hosts = NULL;
		free_dispatch_job(dj);
		free(ab);
		Trace(TR_ERR, "multiplex job failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	/* Convert the job_id string returned by start_activity to an int */
	errno = 0;
	int_job_id = (int)strtol(job_id, &end_ptr, 10);
	if (errno != 0 || *end_ptr != '\0') {
		end_this_activity(job_id);
		dj->args = NULL;
		dj->hosts = NULL;
		free_dispatch_job(dj);
		free(ab);
		setsamerr(SE_INVALID_JOB_ID);
		Trace(TR_ERR, "multiplex job failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	if (wpool_init(func_id, job_id, args, target_hosts,
	    target_host_count, 4, &wp) != 0) {
		end_this_activity(job_id);
		dj->args = NULL;
		dj->hosts = NULL;
		free_dispatch_job(dj);
		free(ab);
		Trace(TR_ERR, "multiplexing the request failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);

	}

	/* start a thread to wait for the job to finish */
	st = pthread_create(&cleanup_thread, &dsp_attr,
	    ((void * (*)(void *))wpool_destroy), wp);
	if (st != 0) {
		/*
		 * Don't return an error since the job is supposedly running
		 */
		Trace(TR_ERR, "wpool_destroy failed: %d", st);
	}


	return (int_job_id);
}


static void
dsp_attr_once(void) {

	pthread_attr_init(&dsp_attr);
	pthread_attr_setdetachstate(&dsp_attr, PTHREAD_CREATE_DETACHED);
}


int
display_multihost_job_status(samrthread_t *ptr, char **result) {
	int buf_size = 2 * MAXPATHLEN;
	char buf[buf_size];
	int added = 0;
	int error_cnt = 0;
	int ok_cnt = 0;
	int pending_cnt;
	dispatch_job_t *dj;
	char *res_string;
	size_t res_size;
	int i;
	int error_num;

	if (ISNULL(ptr, result) || ISNULL(ptr->args) || ISNULL(ptr->args->db)) {
		Trace(TR_ERR, "failed creating multi-host job status: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	dj = (dispatch_job_t *)ptr->args->db.job;


	/* Determine if the job is still pending */
	pending_cnt = dj->hosts_called - dj->hosts_responded;

	/* Add all of the basic information to the string */

	added += snprintf(buf, buf_size, "type=%s,operation=%s,activity_id=%s,"
	    "starttime=%ld,host_count=%d,hosts_responding=%d,"
	    "hosts_pending=%d,", activitytypes[ptr->type],
	    dipspatch_op_names[dj->func_id], ptr->jobid, ptr->start,
	    dj->host_count, dj->hosts_responded, pending_cnt);

	added += get_type_specific_output(dj, buf + added, buf_size - added);

	res_string = copystr(buf);
	if (res_string == NULL) {
		Trace(TR_ERR, "failed creating multi-host job status: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * Get the size of the buffer holding the result string. So add 1
	 * for the '\0'.
	 */
	res_size = strlen(res_string) + 1;

	/*
	 * Loop three times instead of growing 3 separate strings.
	 * First add host errors.
	 */
	for (i = 0; i < dj->host_count; i++) {
		if (dj->responses != NULL &&
		    dj->responses[i].status == OP_FAILED &&
		    dj->responses[i].error != 0) {

			if (dj->hosts[i] != NULL) {
				error_cnt++;
				if (error_cnt == 1) {
					STRLCATGROW(res_string,
					    ",error_hosts=",
					    res_size);
				}

				STRLCATGROW(res_string, dj->hosts[i],
				    res_size);
				STRLCATGROW(res_string, " ",
				    res_size);
			}
		}
	}

	/*
	 * second- add errors for the error hosts.
	 */
	if (error_cnt > 0) {
		for (i = 0; i < dj->host_count; i++) {


			if (dj->responses != NULL &&
			    dj->responses[i].status == OP_FAILED &&
			    dj->responses[i].error != 0 &&
				dj->hosts[i] != NULL) {

				char err_num[10];

				/* append: ,hostname = errornum */
				STRLCATGROW(res_string, ",", res_size);
				STRLCATGROW(res_string, dj->hosts[i], res_size);
				STRLCATGROW(res_string, "=", res_size);
				snprintf(err_num, sizeof (err_num), "%d",
				    dj->responses[i].error);
				STRLCATGROW(res_string, err_num, res_size);

				/* append: quoted error messages */
				if (dj->responses[i].result != NULL) {
					strip_newlines(dj->responses[i].result);

					STRLCATGROW(res_string, " \"",
					    res_size);

					STRLCATGROW(res_string,
					    dj->responses[i].result,
					    res_size);

					STRLCATGROW(res_string, "\" ",
					    res_size);
				}

			}
		}
	}

	/* Third: Build the list of OK hosts */
	for (i = 0; i < dj->host_count; i++) {
		if (dj->hosts != NULL && dj->responses != NULL &&
		    dj->responses[i].status == OP_SUCCEEDED) {
			ok_cnt++;
			if (ok_cnt == 1) {
				STRLCATGROW(res_string, ",ok_hosts=",
				    res_size);
			}
			if (dj->hosts[i] != NULL) {
				STRLCATGROW(res_string, dj->hosts[i],
				    res_size);
				STRLCATGROW(res_string, " ",
				    res_size);
			}
		}
	}

	/* Add the status of the operation */
	if (dj->overall_status == DJ_INITIALIZING) {
		STRLCATGROW(res_string, ",status=initializing", res_size);
	} else if (dj->overall_status == DJ_PENDING) {
		/* The operation is pending */
		STRLCATGROW(res_string, ",status=pending", res_size);

	} else if (dj->overall_status == DJ_POST_PHASE_PENDING) {
		STRLCATGROW(res_string, ",status=post_phase_pending",
		    res_size);
	} else if ((dj->overall_status == DJ_DONE ||
	    dj->overall_status == DJ_POST_PHASE_SUCCEEDED) &&
	    error_cnt == 0) {

		/* The operation succeeded */
		STRLCATGROW(res_string, ",status=success", res_size);
	} else if (dj->overall_status == DJ_POST_PHASE_FAILED) {
		STRLCATGROW(res_string, ",status=failure", res_size);
		error_num = get_operation_error(dj->func_id, B_FALSE);
	} else {
		if (error_cnt == dj->host_count) {
			/* The operation failed */
			STRLCATGROW(res_string, ",status=failure", res_size);
			error_num = get_operation_error(dj->func_id, B_FALSE);
		} else {
			/* The operation failed on some hosts */
			STRLCATGROW(res_string, ",status=partial_failure",
			    res_size);
			error_num = get_operation_error(dj->func_id, B_TRUE);
		}

		STRLCATGROW(res_string, ",error=", res_size);
		snprintf(buf, sizeof (buf), "%d", error_num);
		STRLCATGROW(res_string, buf, res_size);
		STRLCATGROW(res_string, ",error_msg=", res_size);
		STRLCATGROW(res_string, "\"", res_size);
		STRLCATGROW(res_string, GetCustMsg(error_num), res_size);
		STRLCATGROW(res_string, "\"", res_size);

	}

	*result = res_string;
	return (0);
}

static int
get_operation_error(dispatch_id_t func_id, boolean_t partial) {
	int err = -1;

	switch (func_id) {
	case CMD_MOUNT: {
		if (partial) {
			err = SE_DSP_MOUNT_PARTIAL;
		} else {
			err = SE_DSP_MOUNT_FAILURE;
		}
		break;
	}
	case CMD_UMOUNT: {
		if (partial) {
			err = SE_DSP_UNMOUNT_PARTIAL;
		} else {
			err = SE_DSP_UNMOUNT_FAILURE;
		}
		break;
	}
	case CMD_CHANGE_MOUNT_OPTIONS: {
		if (partial) {
			err = SE_DSP_CH_MNT_OPTS_PARTIAL;
		} else {
			err = SE_DSP_CH_MNT_OPTS_FAILURE;
		}

		break;
	}
	case CMD_CREATE_ARCH_FS: {
		if (partial) {
			err = SE_DSP_CREATEFS_PARTIAL;
		} else {
			err = SE_DSP_CREATEFS_FAILURE;
		}
		break;
	}
	case CMD_GROW_FS: {
		if (partial) {
			err = SE_DSP_GROWFS_PARTIAL;
		} else {
			err = SE_DSP_GROWFS_FAILURE;
		}

		break;
	}
	case CMD_REMOVE_FS: {
		if (partial) {
			err = SE_DSP_REMOVEFS_PARTIAL;
		} else {
			err = SE_DSP_REMOVEFS_FAILURE;
		}
		break;
	}
	case CMD_SET_ADV_NET_CONFIG: {
		if (partial) {
			err = SE_DSP_ADVNETCFG_PARTIAL;
		} else {
			err = SE_DSP_ADVNETCFG_FAILURE;
		}
		break;
	}
	default: {
		err = -1;
		Trace(TR_ERR, "There is no support for the function's specific"
		    " error %d", func_id);
	}
	}

	return (err);
}


static void
strip_newlines(char *str) {
	char *cur;
	if (str == NULL) {
		return;
	}
	while ((cur = strpbrk(str, "\n")) != NULL) {
		*cur = ' ';
	}
}


static int
get_type_specific_output(dispatch_job_t *dj, char *buf, int buf_size) {

	int added = 0;
	char *fs_name = NULL;


	if (ISNULL(dj, buf)) {
		return (0); /* zero becuase nothing added */
	}
	/* For now only the fs name is included */
	switch (dj->func_id) {
	case CMD_MOUNT: {
		string_arg_t *mo_arg =
		    (string_arg_t *)dj->args;

		fs_name = mo_arg->str;
		break;
	}
	case CMD_UMOUNT: {
		string_bool_arg_t *um_arg =
		    (string_bool_arg_t *)dj->args;
		fs_name = um_arg->str;
		break;
	}
	case CMD_CHANGE_MOUNT_OPTIONS: {
		change_mount_options_arg_t *cm_arg =
		    (change_mount_options_arg_t *)dj->args;
		fs_name = cm_arg->fsname;
		break;
	}
	case CMD_CREATE_ARCH_FS: {
		create_arch_fs_arg_t *cafs_arg =
		    (create_arch_fs_arg_t *)dj->args;
		fs_name = cafs_arg->fs_info->fi_name;
		break;
	}
	case CMD_GROW_FS: {
		grow_fs_arg_t *gfs_arg = (grow_fs_arg_t *)dj->args;
		fs_name = gfs_arg->fs->fi_name;
		break;
	}
	case CMD_REMOVE_FS: {
		string_arg_t *rmfs_arg = (string_arg_t *)dj->args;
		fs_name = rmfs_arg->str;
		break;
	}
	case CMD_SET_ADV_NET_CONFIG: {
		string_strlst_arg_t *advnet_arg =
		    (string_strlst_arg_t *)dj->args;
		fs_name = advnet_arg->str;
		break;
	}
	default: {
		Trace(TR_ERR, "There is no support for the function's specific"
		    " options %d", dj->func_id);
		return (0);
	}
	}


	if (fs_name != NULL && *fs_name != '\0') {
		added = snprintf(buf, buf_size, "fsname = %s", fs_name);
	}

	return (added);
}


void
free_dispatch_func_args(void *args, dispatch_id_t func_id) {
	if (args == NULL) {
		return;
	}

	switch (func_id) {
	case CMD_MOUNT: {
		xdr_free(xdr_string_arg_t,
		    (char *)args);
		break;
	}
	case CMD_UMOUNT: {
		xdr_free(xdr_string_bool_arg_t,
		    (char *)args);
		break;
	}
	case CMD_CHANGE_MOUNT_OPTIONS: {
		xdr_free(xdr_change_mount_options_arg_t,
		    (char *)args);
		break;
	}
	case CMD_CREATE_ARCH_FS: {
		xdr_free(xdr_create_arch_fs_arg_t,
		    (char *)args);
		break;
	}
	case CMD_GROW_FS: {
		xdr_free(xdr_grow_fs_arg_t, (char *)args);
		break;
	}
	case CMD_REMOVE_FS: {
		xdr_free(xdr_string_arg_t, (char *)args);
		break;
	}
	case CMD_SET_ADV_NET_CONFIG: {
		xdr_free(xdr_string_strlst_arg_t,
		    (char *)args);
		break;
	}
	default: {
		Trace(TR_ERR, "There is no support for the function's "
		    "specific options %d", func_id);
	}
	}

}

void
free_dispatch_job(dispatch_job_t *dj) {
	if (dj == NULL) {
		return;
	}

	/* free the function specific args */
	free_dispatch_func_args(dj->args, dj->func_id);

	/* free the hosts */
	if (dj->hosts) {
		free_string_array(dj->hosts, dj->host_count);
		dj->hosts = NULL;
	}

	/* free the responses array */
	if (dj->responses) {
		free_response_array(dj->responses, dj->host_count);
		dj->responses = NULL;
	}
	if (dj->overall_error_msg) {
		free(dj->overall_error_msg);
	}
	free(dj);
}


static void
free_response_array(op_rsp_t *responses, int count) {
	int i;

	if (responses == NULL) {
		return;
	}

	for (i = 0; i < count; i++) {
		if (responses[i].result != NULL) {
			free(responses[i].result);
		}
	}
	free(responses);
}


/*
 * Takes an input array of strings and creates a duplicate of it by copying
 * the input strings into a new array and setting the inputs to NULL as it
 * progresses. The object is to disconnect the input array so that it can be
 * freed when the calling function returns but the output array can live on
 * past the scope.
 *
 * Returns the number of hosts in the output array.
 *
 * This function removes the current host from the output array because the
 * command dispatcher should never be used to call the current host. Callers
 * wanting to know if the current host was included can check the return value
 * if the return value is equal to the client_count - 1 the current host has
 * been removed.
 */
int
convert_dispatch_hosts_array(
char **clients,
int client_count,
char ***output,
boolean_t *found_local_host) {

	static char host_name[MAXHOSTNAMELEN] = "";
	static int host_name_len = 0;
	int host_cnt = 0;
	char **hosts_to_call;
	int i;

	if (ISNULL(clients, output, found_local_host)) {
		Trace(TR_ERR, "Making dispatch hosts array failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	if (client_count == 0) {
		setsamerr(SE_DSP_NO_CLIENTS);
		Trace(TR_ERR, "Making dispatch hosts array failed: %s",
		    samerrmsg);
		return (-1);
	}

	*output = NULL;
	*found_local_host = B_FALSE;

	/* Fetch the host name the first time the function function executes */
	if (host_name[0] == '\0') {
		if (gethostname(host_name, MAXHOSTNAMELEN) != 0) {
			setsamerr(SE_UNABLE_TO_GET_HOSTNAME);
			Trace(TR_ERR, "Making dispatch hosts array failed:"
			    "%d %s", samerrno, samerrmsg);

			return (-1);
		}
	}
	host_name_len = strlen(host_name);

	/*
	 * Allocate an array big enough for all clients- some may be
	 * stripped out durring the copying.
	 */
	hosts_to_call = (char **)calloc(client_count, sizeof (char *));
	if (hosts_to_call == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "Making dispatch hosts array failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/* Copy the hosts strings over to the array */
	for (i = 0; i < client_count; i++) {

		/* don't copy over NULL or empty entries */
		if (clients[i] == NULL || *clients[i] == '\0') {
			continue;
		}

		/*
		 * If the local host is found continue without making
		 * an entry in the output array. The first part of the
		 * if condition prevents repeatedly evaluating
		 * the other conditions
		 */
		if (!*found_local_host &&
		    (strlen(clients[i]) == host_name_len) &&
		    (strncmp(clients[i], host_name, host_name_len) == 0)) {
			*found_local_host = B_TRUE;
			continue;
		}
		hosts_to_call[host_cnt] = clients[i];
		clients[i] = NULL;
		host_cnt++;
	}

	/*
	 * No actual hosts transferred to the array so free it.
	 */
	if (host_cnt == 0) {
		free(hosts_to_call);
		*output = NULL;

		/*
		 * If local host wasn't found either return -1 since ther
		 * is no work to do.
		 */
		if (!(*found_local_host)) {
			setsamerr(SE_DSP_NO_CLIENTS);
			Trace(TR_ERR, "Making dispatch hosts array failed: %s",
			    samerrmsg);
			return (-1);
		}
	} else {
		*output = hosts_to_call;
	}
	return (host_cnt);

}


static int
remove_dispatch_job(struct samrthread_s *ptr) {
	dispatch_job_t *dj = (dispatch_job_t *)ptr->args->db.job;

	/*
	 * Check to see if the job is done. If not do not
	 * remove it for now. The dispatch jobs are not yet cancelable.
	 */
	if (dj->hosts_responded != dj->host_count &&
	    dj->overall_status < DJ_POST_PHASE_FAILED) {
		samerrno = SE_DSP_OP_NOT_COMPLETE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "removing dispatch job failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}


	/* Don't free the argbuf itself, but free all of its malloced members */
	free_dispatch_job(dj);

	return (0);
}


static samrpc_client_t *
setup_rpc_client(char *host, struct timeval *tm) {
	samrpc_client_t	*samrpc_client	= NULL;
	char		*err_msg = NULL;

	Trace(TR_DEBUG, "snd:created client");

	if (ISNULL(host, tm)) {
		Trace(TR_ERR, "snd:Sending Request failed:%d %s-> %s", samerrno,
		    samerrmsg, host);
		return (NULL);
	}

	samrpc_client = (samrpc_client_t *)mallocer(sizeof (samrpc_client_t));
	if (samrpc_client == NULL) {
		Trace(TR_ERR, "snd:Sending Request failed:%d %s-> %s", samerrno,
		    samerrmsg, host);
		return (NULL);
	}
	memset(samrpc_client, 0, sizeof (samrpc_client_t));

	samrpc_client->svr_name = copystr(host);
	if (samrpc_client->svr_name == NULL) {
		Trace(TR_ERR, "snd:Sending Request failed:%d %s-> %s", samerrno,
		    samerrmsg, host);
		return (NULL);
	}
	samrpc_client->clnt = clnt_create_timed(host, SAMMGMTPROG,
	    SAMMGMTVERS, "tcp", tm);
	if (samrpc_client->clnt == (CLIENT *)NULL) {
		samerrno = SE_RPC_CANNOT_CREATE_CLIENT;
		err_msg = clnt_spcreateerror(host);
		snprintf(samerrmsg, MAX_MSG_LEN, err_msg);
		Trace(TR_ERR, "snd:Sending Request failed:%d %s-> %s", samerrno,
		    samerrmsg, host);
		free(samrpc_client->svr_name);
		free(samrpc_client);
		return (NULL);
	}

	clnt_control((samrpc_client->clnt), CLSET_TIMEOUT, (char *)tm);


	return (samrpc_client);
}


static int
destroy_rpc_client(samrpc_client_t *rc) {

	if (ISNULL(rc)) {
		Trace(TR_ERR, "snd:destroy clnt failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	if (rc->clnt) {
		clnt_destroy(rc->clnt);
	}
	if (rc->svr_name) {
		free(rc->svr_name);
	}
	free(rc);

	Trace(TR_DEBUG, "snd:destroyed clnt");

	return (0);
}

static void
free_wpool(wpool_t *wpool) {

	if (wpool) {
		if (wpool->tpool) {
			free_tpool(wpool->tpool);
		}
		free(wpool);
	}
}

static void
free_tpool(tpool_t *tpool) {
	if (tpool) {
		if (tpool->threads) {
			free(tpool->threads);
		}
		free(tpool);
	}
}
