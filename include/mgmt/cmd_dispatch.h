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
#pragma ident   "$Revision: 1.3 $"

#ifndef	_CMD_DISPATCH_H
#define	_CMD_DISPATCH_H

#include <time.h>
#include <sys/types.h>
#include "pub/mgmt/types.h"

/*
 * Command Dispatcher - for use in a Shared QFS set up
 *
 * In a shared QFS system, certain operations such as add client, grow fs,
 * shrink fs, change mount options, mount and unmount have to be carried out
 * on all the shared clients. Inorder to respond efficiently to these requests,
 * and scale when the number of shared clients is in thousands, these operations
 * are multiplexed in parallel to the shared clients:-
 *
 * 1. These operations are implemented as jobs, with the provision to view the
 *    status and progress of the operation at any time.
 * 2. These operations could be made cancelable.
 * 3. The completed job status is made available even after the job is completed
 * 4. The user can request to purge the status of a completed operation.
 * 5. The calls to the clients could be made asynch.
 */

typedef enum dispatch_id_s {
	CMD_MOUNT,
	CMD_UMOUNT,
	CMD_CHANGE_MOUNT_OPTIONS,
	CMD_CREATE_ARCH_FS,
	CMD_GROW_FS,
	CMD_REMOVE_FS,
	CMD_SET_ADV_NET_CONFIG,
	MAX_DISPATCH_ID
} dispatch_id_t;

#define	DSP_MOUNT	"DSP_MOUNT"
#define	DSP_UMOUNT	"DSP_UMOUNT"
#define	DSP_CHANGE_MOUNT_OPTIONS "DSP_CHANGE_MOUNT_OPTS"
#define	DSP_ADD_CLIENTS	"DSP_ADD_CLIENTS"
#define	DSP_GROW_FS "DSP_GROW_FS"
#define	DSP_REMOVE_CLIENTS "DSP_REMOVE_CLIENTS"
#define	DSP_SET_ADV_NET_CONFIG "DSP_SET_ADV_NET_CONFIG"


/*
 * Data structure to hold the parameters to multiplex commands to a
 * client
 */
typedef struct op_req_s {
	dispatch_id_t func_id;	/* function identifier */
	char	*job_id;	/* id of the job */
	int	host_num;	/* index of target host in job's hosts array */
	void	*args;		/* pointer to per-function type argument */
} op_req_t;


/* Status for calls to a single host */
typedef enum op_status_s {
	OP_NOT_YET_CALLED,
	OP_PENDING,
	OP_SUCCEEDED,
	OP_FAILED
} op_status_t;


typedef	struct op_rsp_s {
	op_status_t	status;
	int		error;		/* samerrno */
	char		*result;	/* error msg or results */
} op_rsp_t;

typedef enum dj_status {
	DJ_INITIALIZING,
	DJ_PENDING,
	DJ_POST_PHASE_PENDING,
	DJ_POST_PHASE_FAILED,
	DJ_POST_PHASE_SUCCEEDED,
	DJ_DONE
} dj_status_t;


typedef struct dispatch_job_s {
	dispatch_id_t	func_id;
	void		*args;
	time_t		starttime;
	time_t		endtime;	/* 0 means not done */
	int		host_count;
	char		**hosts;
	int		hosts_responded;
	int		hosts_called;
	dj_status_t	overall_status;
	int		overall_error_num;
	char		*overall_error_msg;
	int (*post_phase)(struct dispatch_job_s *); /* executed when done */
	op_rsp_t	*responses; /* calloced array of host_count size */
} dispatch_job_t;


/*
 * Dispatch the job to the specified hosts.
 * Returns the numeric id of the job.
 * If this function returns -1 the
 * caller needs to free the target hosts and the arguments. Otherwise they
 * should not.
 */
int
multiplex_request(ctx_t *c, dispatch_id_t req_type, char **target_hosts,
    int target_host_count, void *args, int (post_phase)(dispatch_job_t *));


/*
 * Function to send a request to a single host synchronously
 */
int send_request(op_req_t *req, char *host);

/*
 * This function is called remotely by the command dispatcher for each
 * target host.  handle_request decodes the arguments from the wire
 * format and kicks off a thread to execute the operation
 * asynchronously.
 */
int
handle_request(ctx_t *c,  op_req_t *req);


void free_dispatch_job(dispatch_job_t *dj);
void free_dispatch_func_args(void *args, dispatch_id_t func_id);

/*
 * Convert an array of clients that was passed as input to the rpc server to
 * one that can safely be used past the scope of the function.
 *
 * Takes an input array of strings and creates a duplicate of it by copying
 * the input strings into the new array and setting the inputs to NULL as it
 * progresses. The object is to disconnect the input array so that it can be
 * freed when the calling function returns but the output array and strings
 * can live on.
 *
 * Returns the number of hosts in the output array. This number can be different
 * than the client_count input if the array is gappy(contains NULLs) or
 * contains the local host.
 *
 * This function removes the current host from the output array because the
 * command dispatcher should never be used to call the current host. Callers
 * wanting to know if the current host was included can check the value of
 * found_local_host.
 */
int
convert_dispatch_hosts_array(char **clients, int client_count, char ***output,
    boolean_t *found_local_host);


/*
 * Function to call whenever an error results from calling a host.
 * For instance if a host returns an error on the initial call
 * to handle request or when it returns an error or result
 * through handle response.
 *
 * If passed with error_number != 0 the result will be
 * interpreted as an error. Otherwise it is interpreted as a
 * response.
 */
int host_responded(char *job_id, char *host, int host_num,
    int error_number, char *result);


/*
 * Function to update the job struct when a host is about to
 * be called.
 */
int
calling_host(char *job_id, char *host, int host_num);


#endif	/* _CMD_DISPATCH_H */
