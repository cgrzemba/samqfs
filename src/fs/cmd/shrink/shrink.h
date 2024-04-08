/*
 * shrink.h - remove/relaser equipment (ordinal) from file system
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

#ifndef	_SAM_FS_SHRINK_H
#define	_SAM_FS_SHRINK_H

#ifdef sun
#pragma ident "$Revision: 1.5 $"
#endif

#ifndef MAXPATHLEN
#define	MAXPATHLEN	1024
#endif

#define	MAX_THREADS	1024		/* Maximum number of threads */
#define	MAX_QUEUE	1024		/* Maximum size of I/O request queue */

/*
 * There is only one control structure. It describes the thread pool.
 * The control structure holds the head and tail of the I/O request work
 * queue.
 */
typedef struct thread {
	int num;		/* The thread number or position in control */
	pthread_t tid;		/* The thread id */
	int fd;			/* file descriptor for the I/O input file */
	int fdo;		/* file descriptor for the I/O output file */
	int waitcount;		/* Num times waiting for open to complete */
} thr_t;

typedef struct control {
	struct work *queue_head;	/* Request I/O work queue head */
	struct work *queue_tail;	/* Request I/O work queue tail */
	pthread_mutex_t queue_full_lock; /* Req I/O work queue full mutex */
	pthread_mutex_t queue_lock;	/* Request I/O work queue full mutex */
	pthread_cond_t queue_not_empty;	/* Req I/O work condition not empty */
	pthread_cond_t queue_not_full;	/* Req I/O work condition not full */
	pthread_cond_t queue_empty;	/* Request I/O work condition empty */
	time_t log_time;		/* Last log time in seconds */
	int block_size;			/* Size of buffer 1 <= nMB <= 16 */
	int streams;			/* Number of threads */
	int busy_files;			/* No. of busy files on this eq */
	int unarchived_files;		/* No. of unarchived files on this eq */
	int queue_size;			/* Current # of entries in the queue */
	int io_outstanding;		/* Current # of outstanding requests */
	int shutdown;			/* Any thread requested to stop */
	int command;			/* Release or Remove command */
	int total_errors;		/* No. of files w/error on this eq */
	boolean_t do_not_execute;	/* Do not release or remove */
	boolean_t stage_files;		/* Stage files */
	boolean_t stage_partial;	/* Stage partial */
	equ_t eq;			/* Equipment */
	ushort_t ord;			/* Equipment ordinal */
	int 	ord2;		/* Optional destination ordinal for remove */
	char mountpoint[MAXPATHLEN];	/* Mount point */
	char log_pathname[MAXPATHLEN];	/* Log pathname */
	int display_all_files;		/* Display all processed files in log */
	FILE *log;			/* Stream for logging */
	struct sam_fs_info *mp;		/* File system mount table */
	thr_t thr[MAX_THREADS];		/* Array of thread information */
} control_t;


/*
 * The I/O request work queue is a forward linked list with the head
 * and tail in the control structure.
 *
 * work_t is a single request on the I/O request work queue. It includes
 * the I/O request parameters: size, byte_offset, and the pointer to
 * the buffer.
 */
typedef struct work {
	struct work *next;		/* Forward link list pointer */
	pthread_mutex_t work_lock;	/* Mutex for this I/O request entry */
	pthread_cond_t cv_work_done;	/* Condition for this work completed */
	int busy;			/* Set when I/O is outstanding */
	int first;			/* Set to indicate read NOT complete */
	int num;			/* entry num [0..(queue_size-1)] */
	ssize_t size;			/* Block size in bytes */
	int64_t byte_offset;		/* Byte offset in file */
	char *buffer;			/* Buffer pointer */
	int ret_errno;			/* Error number returned */
} work_t;


/*
 * Specifies the input parameters. This is the parameter block.
 */
typedef struct parameter_block {
	int64_t file_size;		/* File size */
	int64_t block_count;		/* Block count */
	int64_t block_size;		/* Block size */
	int error_limit;		/* error limit */
	int read_threads;		/* Number of read threads */
	int queue_size;			/* Request I/O work queue size */
	int active_threads;		/* Active number of threads */
} pblock_t;

#endif	/* _SAM_FS_SHRINK_H */
