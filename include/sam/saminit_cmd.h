
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

#ifndef SAMCMD
#define	SAMCMD

/*
 * A cmd queue entry:
 *	cmd_cv: the cv that signals when the cmd has been picked up by sam-init.
 *	cmd_mutex: the lock for the above cv.
 *	forward: the forward link.
 *	back:	the backward link.
 *	qcmd_error:	the error code when sam-init doesn't pick up the cmd.
 *	cmd:	the cmd itself.
 */
typedef struct saminit_cmd_queue {
	kcondvar_t	cmd_cv;
	kmutex_t	cmd_mutex;
	struct saminit_cmd_queue	*forward;
	struct saminit_cmd_queue	*back;
	int		qcmd_error;
	int		qcmd_wait;
	int		blk_flag;
	sam_fs_fifo_t	cmd;
} saminit_cmd_queue_t;


/*
 * the cmd queue header:
 *	front:	the first cmd in the queue.
 *	end:	the last cmd in the queue.
 *	cmd_queue_mutex:	lock for the cmd queue.
 *	cmd_lockout_mutex:	lock for the cmd lock out logic
 *	cmd_lockout_cv:		cv to wait on when the cmd queue is locked.
 *	cmd_queue_cv:		cv for sam-init to wait on when there are
 *				no cmds in the queue.
 *	cmd_lock_flag:		cmd lock out flag.
 *	cmd_queue_timeout:	timeout value to determine if sam-init
 *				has timed out.
 */

typedef struct {
	saminit_cmd_queue_t	*front;
	saminit_cmd_queue_t	*end;
	kmutex_t		cmd_queue_mutex;
	kmutex_t		cmd_lockout_mutex;
	kcondvar_t		cmd_lockout_cv;
	kcondvar_t		cmd_queue_cv;
	int			cmd_lock_flag;
	clock_t			cmd_queue_timeout;
} saminit_cmd_queue_hdr_t;

/*
 * The command table:
 *	cmd_buffers:	a kmem_alloc'ed array of cmd buffers.
 *	cmd_buffer_free_list:	pointer to the first buffer on the free list.
 *	saminit_cmd_queue_hdr:	The cmd queue header.
 *	queue_mutex:	Free list mutex.
 *	global_mutex:	mutex for the command table.
 *	sam_init_pid:	The pid of sam-init.
 *	cmd_buffer_free_list_count:	Count of free list entries.
 */
typedef struct saminit_cmd_table {
	saminit_cmd_queue_t	*cmd_buffers;		/* command buffers */
	saminit_cmd_queue_t	*cmd_buffer_free_list;	/* free list of cmds */
	saminit_cmd_queue_hdr_t	saminit_cmd_queue_hdr;
	kmutex_t		queue_mutex;
	kmutex_t		global_mutex;
	int			sam_init_pid;
	int			cmd_buffer_free_list_count;
} saminit_cmd_table_t;

/* Number of sam-init cmd buffers */

#define	SAM_INIT_CMD_BUF	(100)

/*
 * return values from sam_init_status.  These are also defined in
 * include/pub/samapi.h
 */
#define	SAM_INIT_SHM_BAD	(-1)
#define	SAM_INIT_RUNNING	(0)
#define	SAM_INIT_SHUT_DOWN	(1)
#define	SAM_INIT_NOT_RESPONDING	(2)
#define	SAM_INIT_DEAD		(3)
#define	SAM_INIT_CANT_TELL	(4)
#define	SAM_INIT_SHM_GOOD	(5)

#endif /* SAMCMD */
