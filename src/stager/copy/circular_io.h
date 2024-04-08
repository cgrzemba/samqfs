/*
 * circular_io.h - circular i/o definitions
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

#ifndef CIRCULAR_IO_H
#define	CIRCULAR_IO_H

#pragma ident "$Revision: 1.3 $"

/* Forward declarations. */
struct BlockState; typedef struct BlockState BlockState_t;

/*
 * Define a circular buffer with two pointers 'in' and 'out' to
 * indicate the next available position for depositing data and
 * the position that contains the next data to be retrieved.
 * A producer thread deposits data into the 'in' position and
 * advances the pointer 'in', and each consumer retrieves the
 * the data in position 'out' and advances the pointer 'out'.
 *
 * A producer must wait until the buffer is not full, deposit its
 * its data, and then notify the consumer that the buffer is not empty.
 *
 * A consumer must wait until the buffer is not empty, retrieve its
 * data, and then notify the producer that the buffer is not full.
 *
 * A circular buffer is shared by two threads this requires a
 * mutex lock.
 */

typedef struct CircularBuffer {
	char	*cb_first;		/* fwa for start of buffer */

	int	cb_blockSize;		/* media block size */
	int	cb_numBuffers;		/* number of block size buffers */
	size_t	cb_bufSize;		/* circular buffer size */

	int	cb_in;			/* next empty slot in the buffer */
	int	cb_out;			/* next available data slot */

	pthread_mutex_t	cb_lock;	/* protect buffer */

	boolean_t	cb_notEmpty;	/* set true if buffer is not empty */
	pthread_cond_t	cb_empty;

	boolean_t	cb_notFull;	/* set true if buffer is not full */
	pthread_cond_t	cb_full;

	/*
	 * State of data block. One entry for each data buffer.
	 *
	 * Block position and any error encountered for a data block.
	 * The caller will set/get the block number for a block of data.
	 * Is used to check if a block is already available in the
	 * circular buffer and avoid media positioning.
	 */
	BlockState_t	*cb_state;
} CircularBuffer_t;

/* State of data block in circular i/o buffer. */
struct BlockState {
	int		bs_blkno;	/* block no for data buffer */
	int		bs_errno;	/* errno for block */
};

/*
 * Stage io thread control.
 * Structure for communication between io threads.
 */
typedef struct IoThreadInfo {
	ushort_t	io_flags;

	/* Definitions for staging from removable media. */
	equ_t		io_drive;	/* staged drive number */
	int		io_rmFildes;	/* removable media file descriptor */

	/* Definitions for staging from remote media or disk archives. */
	SamrftImpl_t	*io_rftHandle;	/* file transfer handle */

	/*
	 * A medium allocation unit is defined as the size of the smallest
	 * addressable part on a medium that can be accessed.  On tapes,
	 * this is a block.  On opticial, this is a sector.
	 */
	int		io_blockSize;	/* allocation unit size in bytes */
	int		io_numBuffers;	/* number of buffers to allocate */
	int		io_position;	/* current media position */

	/* Definitions for file being staged. */
	FileInfo_t	*io_file;	/* file to stage */
	longlong_t	io_size;	/* number of bytes to stage */
	int		io_offset;	/* block offset to file's tar header */

	/*
	 * Notify archive read thread that file is ready to stage.
	 * Notify waiter that archive read thread finished staging file.
	 */
	ThreadState_t	io_readReady;
	ThreadState_t	io_readDone;

	/*
	 * Notify double buffer thread that file is ready to stage.
	 * Notify waiter that double buffer thread finished staging file.
	 */
	ThreadState_t	io_moveReady;
	ThreadState_t	io_moveDone;

	/*
	 * Notify write thread that file is ready to stage.
	 * Notify waiter that write thread finished staging file.
	 */
	ThreadState_t	io_writeReady;
	ThreadState_t	io_writeDone;

	CircularBuffer_t	*io_reader;
	CircularBuffer_t	*io_writer;

} IoThreadInfo_t;

/* Flags defined for IoThreadInfo structure. */
enum {
	IO_disk		= 1 << 0,	/* staging from disk */
	IO_stk5800	= 1 << 1,	/* staging from stk 5800 disk */
	IO_samremote	= 1 << 2,	/* staging from removable media on */
					/* another host (SAM remote) */
	IO_cancel	= 1 << 3,	/* cancel staging */
	IO_error	= 1 << 4	/* error encountered during stage */
};
#define	IO_diskArchiving (IO_disk | IO_stk5800)


/* Define prototypes in circular_io.c */
CircularBuffer_t *CircularIoConstructor(int numBuffers, int blockSize,
    boolean_t lockbuf);
void CircularIoDestructor(CircularBuffer_t *buffer);
void CircularIoReset(CircularBuffer_t *buffer);
char *CircularIoWait(CircularBuffer_t *buffer, int *len);
void CircularIoAdvanceIn(CircularBuffer_t *buffer);
char *CircularIoAvail(CircularBuffer_t *buffer, int *len, int *error);
void CircularIoAdvanceOut(CircularBuffer_t *buffer);
char *CircularIoGetIn(CircularBuffer_t *buffer, int residual);
char *CircularIoGetOut(CircularBuffer_t *buffer, int residual);

void CircularIoSetBlock(CircularBuffer_t *buffer, char *ptr, int blockNumber);
void CircularIoSetError(CircularBuffer_t *buffer, char *ptr, int error);

int CircularIoGetBlock(CircularBuffer_t *buffer, char *ptr);
int CircularIoGetError(CircularBuffer_t *buffer, char *ptr);

char *CircularIoStartBlockSearch(CircularBuffer_t *buffer, int blockNumber,
	int *len);

int CircularIoSlot(CircularBuffer_t *buffer, char *ptr);

#endif	/* CIRCULAR_IO_H */
