/*
 *	dvt.h - device verification test defines and structs.
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
#ifndef _DVT_DVT_H
#define	_DVT_DVT_H

#pragma ident "$Revision: 1.12 $"



#define	   MAX_THREADS	64	/* Maximum number of threads */
#define	   MAX_QUEUE	64	/* Maximum size of I/O request queue */


/*
 * There is only one control structure. It describes the thread pool.
 * The control structure holds the head and tail of the I/O request work
 * queue.
 */

typedef struct thread {
	int num;		/* The thread number or position in control */
	int tid;		/* The thread id */
	int fd;			/* The file descriptor for the I/O file */
} thr_t;

typedef struct control {
	struct work *queue_head;	/* Request I/O work queue head */
	struct work *queue_tail;	/* Request I/O work queue tail */
	mutex_t queue_lock;		/* Request I/O work queue mutex */
	mutex_t queue_full_lock;	/* Request I/O work queue full mutex */
	cond_t queue_not_empty;		/* Req I/O work condition not empty */
	cond_t queue_not_full;		/* Req I/O work condition not full */
	cond_t queue_empty;		/* Request I/O work condition empty */
	int queue_size;			/* Current # of entries in the queue */
	int io_outstanding;		/* Current # of outstanding requests */
	int shutdown;			/* Any thread requested to stop */
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
	struct work *next;	/* Forward link list pointer */
	mutex_t work_lock;	/* Mutex for this I/O request entry */
	cond_t cv_work_done;	/* Condition for this work completed */
	int write;		/* Set if writing, zero if reading */
	int busy;		/* Set when I/O is outstanding */
	int first;		/* Set to indicated read NOT completed */
	int num;		/* Number of work entry [0..(queue_size-1)] */
	int header_block;	/* Marks header block, the parameter block */
	int size;		/* Block size in bytes */
	offset_t byte_offset;	/* Byte offset in file */
	char *buffer;		/* Buffer pointer */
	char *cmp_buffer;	/* Compare buffer pointer */
	int ret_errno;		/* Errno - returned */
} work_t;


/*
 * Specfies the input parameters. This is the parameter block.
 * The parameter block is written at the beginning of the first
 * block of the file.
 */
typedef struct parameter_block {
	int pblock_size;	/* sizeof(struct parameter_block) */
	int count;		/* Block count */
	int size;		/* Block size */
	int stride;		/* Stride count */
	int byte_offset;	/* Byte offset */
	int option;		/* write, read or write/read */
	int pattern;		/* data pattern */
	int error_limit;	/* error limit */
	int directio;		/* 0 = no directio, 1 = yes directio */
	int preallocate;	/* 0 = no preallocate, 1 = yes preallocate */
	int write_threads;	/* Number of write threads */
	int read_threads;	/* Number of read threads */
	int queue_size;		/* Request I/O work queue size */
	int stripe_group;	/* Stripe group number, -1 if none */
	int verify_data;	/* Set to verify data */
	char file_name[MAXNAMELEN];	/* Full filename */
} pblock_t;

#endif /* _DVT_DVT_H */
