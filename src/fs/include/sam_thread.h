/*
 *	sam_thread.h - thread table for the SAM file system.
 *
 *	Defines the structure of the thread tables. The thread tables are in the
 *	mount table.
 *
 */

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

#ifndef	_SAM_FS_THREAD_H
#define	_SAM_FS_THREAD_H

#ifdef sun
#pragma ident "$Revision: 1.38 $"
#endif




/*
 * ----- sam_thr_t - holds thread information for SAM-FS threads.
 *
 * sam_blk_thread     - Fills the block pools.
 * . Put free blocks in the blocks pools for allocation by this mount point.
 * . 1 thread / file system.
 * . sam_thr_t block is in the mount table.
 *
 * sam_reclaim_thread -
 * . Put free inodes in the inode pool for allocation by this mount point.
 * . Reclaim freed blocks for this mount point.
 * . Inactivates the eligible inactive inodes.
 * . 1 thread / file system.
 * . sam_thr_t inode is in the mount table.
 */

/*
 * Requested event set for the filesystem thread.
 */
#define	SAM_THR_NULL		0x0000	/* No event */
#define	SAM_THR_MOUNT		0x0001	/* Start up thread */
#define	SAM_THR_UMOUNTING	0x0002	/* Idle thread */
#define	SAM_THR_UMOUNT		0x0004	/* Terminate thread */


/*
 * State set by the file system thread.
 */
#define	SAM_THR_INIT		0x0000
#define	SAM_THR_EXIST		0x0001
#define	SAM_THR_DEAD		0x0002

#include "sam_list.h"

typedef struct _sam_queue {
	uint64_t items;
	uint64_t wmark;
	uint64_t current_item;
	uint64_t next_item;
#ifdef sun
	struct list list;
#endif
#ifdef linux
	struct list_head list;
#endif
} sam_queue_t;

/* Mutex order: mutex, put_mutex. */
typedef struct sam_thr {
	kmutex_t	mutex;		/* mutex for this struct */
	kmutex_t	put_mutex;	/* mutex for the put cv */
	kcondvar_t	put_cv;		/* cv, for putting, this thread puts */
	kcondvar_t	get_cv;		/* cv - for getting from this thread */
	int		put_wait;	/* This thread is waiting on put_cv */
	int		wait;		/* Cnt threads waiting on this thread */
	ushort_t	busy;		/* This thread is busy */
	ushort_t	no_threads;	/* Count of thread. */
	ushort_t	flag;		/* Mount/Idle/Umount flag */
	ushort_t	state;		/* This thread's current state */
	sam_queue_t queue;		/* msg queue for put */
} sam_thr_t;

/*
 * Miscellaneous constants used for scheduling.
 *
 * SAM_INACTIVE_DEFER_SECS should be larger than the NFS client
 * NFS3_JUKEBOX_DELAY value to properly handle NFS client stage
 * requests.  This value is 10 seconds for Solaris NFS clients.
 */

#define	SAM_EXPIRE_MIN_TICKS (hz/2)	/* Throttle for expire tasks */
#define	SAM_EXPIRE_MAX_TICKS (hz*45)	/* Most time between expire task runs */
#define	SAM_INACTIVE_MIN_TICKS (hz*4)	/* Throttle for inactivate task */
#define	SAM_INACTIVE_DEFER_SECS 15	/* Delay before inactivating inode */
#define	SAM_RESYNCING_RETRY_SECS 2	/* Check ready-to-resync this often */
#define	MAX_TASKQ_DISPATCH_ATTEMPTS	3

/*
 * The following definitions are used by the new task queue mechanisms.
 */

struct sam_schedule_entry;

#ifdef sun
typedef void (*sam_task_func_t) (struct sam_schedule_entry *);
#endif /* sun */
#ifdef linux
typedef int (*sam_task_func_t) (void *);
#endif /* linux */

typedef struct sam_schedule_queue {
	struct sam_schedule_entry *forw, *back;
} sam_schedule_queue_t;

typedef struct sam_schedule_entry {
	sam_schedule_queue_t queue;	/* Forw and back ptrs, must be first */
	struct sam_mount *mp;		/* Mnt point for this task, or NULL */
	sam_task_func_t func;		/* Function this task will run */
	void *arg;			/* Argument for task, if any */
	int64_t start;			/* Scheduled start of this task */
#ifdef sun
	timeout_id_t id;		/* Timeout ID for this task */
#endif /* sun */
#ifdef linux
	struct task_struct *task;
	kcondvar_t wq;		/* wait_queue_head_t */
	pid_t	id;			/* process id */
	clock_t	ticks;
	int signaled;
#endif /* linux */
} sam_schedule_entry_t;

#ifdef linux
sam_schedule_entry_t *task_begin(void *);
#endif /* linux */

/*
 * Flag values.
 *
 * SAM_SCHEDULE_TASK flags are used to signal particular tasks that
 * they have received new data to process, or to indicate that a task
 * is running.  Some tasks may not use these flags, but they can be
 * used as a convenience if the information is not otherwise available.
 */

#define	SAM_SCHEDULE_EXIT 0x0001	/* Shutting down scheduling queue */

#define	SAM_SCHEDULE_TASK_CLIENT_RECLAIM_RUNNING	0x00010000
#define	SAM_SCHEDULE_TASK_SERVER_RECLAIM_RUNNING	0x00040000
#define	SAM_SCHEDULE_TASK_THAWING			0x00100000
#define	SAM_SCHEDULE_TASK_RESYNCING			0x00200000
#define	SAM_SCHEDULE_TASK_SYNC_CLOSE_INODES		0x00400000

void sam_taskq_init(void);
void sam_taskq_destroy(void);
void sam_taskq_start_mp(struct sam_mount *mp);
void sam_taskq_stop_mp(struct sam_mount *mp);
int sam_taskq_add_ret(sam_task_func_t func, struct sam_mount *mp, void *arg,
	clock_t ticks);
void sam_taskq_add(sam_task_func_t func, struct sam_mount *mp, void *arg,
	clock_t ticks);
void sam_taskq_remove(sam_schedule_entry_t *entry);
void sam_taskq_remove_nocount(sam_schedule_entry_t *entry);
void sam_taskq_uncount(struct sam_mount *mp);
int sam_taskq_dispatch(sam_schedule_entry_t *entry);
void sam_init_schedule_entry(sam_schedule_entry_t *entry, struct sam_mount *mp,
	sam_task_func_t func, void *arg);
#endif /* _SAM_FS_THREAD_H */
