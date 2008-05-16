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
#ifndef	_LOAD_H
#define	_LOAD_H

#pragma ident   "$Revision: 1.14 $"

/*
 * load.h -- SAMFS APIs to configure media loading priorities and to view
 * pending load requests.
 */


#include <sys/types.h>

#include "sam/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/types.h"

/*
 * ****************************************************
 *		structs
 * ****************************************************
 */

/* pending_load_info_t flags definitions */
#define	LD_BUSY			0x000001  /* entry is busy don't touch */
#define	LD_IN_USE		0x000002  /* entry in use (occupied) */
#define	LD_P_ERROR		0x000004  /* clear vsn requested */
#define	LD_WRITE		0x000008  /* write access request flag */
#define	LD_FS_REQ		0x000010  /* file system requested */
#define	LD_BLOCK_IO		0x000020  /* use block IO for optical/tape IO */
#define	LD_STAGE		0x000040  /* stage request flag */
#define	LD_FLIP_SIDE		0x000080  /* if flipside already mounted */


/*
 * structure for data related to pending load requests.
 */
typedef struct pending_load_info {

	int	   id;			/* preview id (formerly slot) */
	ushort_t   flags;		/* status flags - see above def. */
	ushort_t   count;		/* num of requests for this media */
	equ_t	   robot_equ;	/* equipment if robot */
	equ_t	   remote_equ;	/* equipment if remote */
	mtype_t	   media;		/* media type */
	uint_t	   slot;		/* slot number if robot */
	time_t	   ptime;		/* time when request was created */
	time_t	   age;			/* elapsed time in seconds */
	float	   priority;	/* total priority of request */
	pid_t	   pid;			/* pid for requestor */
	uname_t	   user;		/* user name of requestor */
	vsn_t	   vsn;			/* volume serial number */
} pending_load_info_t;


/*
 * ****************************************************
 *		functions
 * ****************************************************
 */

/*
 * DESCRIPTION:
 *   get information about all pending load requests
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   sqm_lst_t **	OUT  - a list of pending_load_info_t structures
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 *   -2 w/ empty list -- no libraries configured (no preview shared memory)
 */
int get_pending_load_info(ctx_t *, sqm_lst_t **pending_load_infos);


/*
 * DESCRIPTION:
 *   cancel a load request based on vsn and index in pending queue
 * PARAMS:
 *   ctx_t *	IN   - context object
 *   vsn_t	IN   - vsn
 *   int	IN   - index in pending queue
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int clear_load_request(ctx_t *, vsn_t vsn, int index);

/*
 * Free methods.
 */
void free_pending_load_info(pending_load_info_t *load_info);
void free_list_of_pending_load_info(sqm_lst_t *load_info_list);


#endif	/* _LOAD_H */
