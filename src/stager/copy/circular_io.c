/*
 * circular_io.c - circular i/o buffering
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <values.h>
#include <assert.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#include "sam/sam_malloc.h"
#include "aml/sam_rft.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stager_threads.h"
#include "stager_shared.h"

#include "circular_io.h"

/* Private data. */
static char *findBlock(CircularBuffer_t *buffer, int blockNumber, int *slot);
static int getError(CircularBuffer_t *buffer, char *ptr);

/*
 * A circular i/o buffer is a data structure that uses a single, fixed-size
 * buffer as if it were connected end-to-end.  This structure lends itself
 * easily to buffer i/o data streams.  A circular buffer does not need to
 * have its elements shuffled around when one is consumed and is well
 * suited as a FIFO buffer.
 *
 * Full/empty buffer distinction.  Always keep one byte unallocated.  A
 * full buffer has at most buffer size -1 bytes.  If both pointers are
 * pointing at the same location, the buffer is empty.
 */

/*
 * Initialize i/o buffers.
 */
CircularBuffer_t *
CircularIoConstructor(
	int numBuffers,
	int blockSize,
	boolean_t lockbuf)
{
	int i;
	char *ptr;
	CircularBuffer_t *buffer;

	SamMalloc(buffer, sizeof (CircularBuffer_t));
	memset(buffer, 0, sizeof (CircularBuffer_t));

	PthreadMutexInit(&buffer->cb_lock, NULL);

	buffer->cb_notEmpty = B_TRUE;
	PthreadCondInit(&buffer->cb_empty, NULL);

	buffer->cb_notFull = B_TRUE;
	PthreadCondInit(&buffer->cb_full, NULL);

	buffer->cb_numBuffers = numBuffers;
	buffer->cb_bufSize = numBuffers * blockSize;
	buffer->cb_blockSize = blockSize;

	/* Allocate buffer. */
	if (lockbuf == B_TRUE) {
		SamValloc(buffer->cb_first, buffer->cb_bufSize);
		if (mlock(buffer->cb_first, buffer->cb_bufSize) < 0) {
			WarnSyscallError(HERE, "mlock", "");
			SamFree(buffer->cb_first);
			lockbuf = B_FALSE;
		}
	}

	if (lockbuf == B_FALSE) {
		SamMalloc(buffer->cb_first, buffer->cb_bufSize);
		memset(buffer->cb_first, 0, buffer->cb_bufSize);
	}

	/* Allocate state. One entry for each data buffer. */
	SamMalloc(buffer->cb_state, numBuffers * sizeof (BlockState_t));
	memset(buffer->cb_state, 0, numBuffers * sizeof (BlockState_t));

	CircularIoReset(buffer);

	Trace(TR_MISC, "Circular buffer");
	for (i = 0; i < numBuffers; i++) {
		ptr = buffer->cb_first + (blockSize * i);
		Trace(TR_MISC, "%d [0x%x]", i, (int)ptr);
	}

	return (buffer);
}

/*
 * Free i/o buffers.
 */
void
CircularIoDestructor(
	CircularBuffer_t *buffer)
{
	if (buffer != NULL) {
		if (buffer->cb_first != NULL) {
			SamFree(buffer->cb_first);
		}
		if (buffer->cb_state != NULL) {
			SamFree(buffer->cb_state);
		}
		SamFree(buffer);
	}
}

/*
 * Reset i/o buffers.
 */
void
CircularIoReset(
	CircularBuffer_t *buffer)
{
	int i;

	buffer->cb_notEmpty = B_TRUE;
	buffer->cb_notFull = B_TRUE;
	buffer->cb_in = buffer->cb_out = 0;
	for (i = 0; i < buffer->cb_numBuffers; i++) {
		buffer->cb_state[i].bs_blkno = -1;
		buffer->cb_state[i].bs_errno = 0;
	}
}

/*
 * A producer thread must wait until the buffer is not full.
 * Returns next available 'in' position.
 */
char *
CircularIoWait(
	CircularBuffer_t *buffer,
	int *len)
{
	char *buf;

	PthreadMutexLock(&buffer->cb_lock);

	for (;;) {
		int empty;

		/*
		 * Full/empty buffer distinction.  Always keep one byte
		 * unallocated.  A full buffer has at most bufSize -1 bytes.
		 * If both pointers are pointing at the same location,
		 * the buffer is empty.
		 */
		empty = buffer->cb_out - buffer->cb_in;
		if (empty <= 0) {
			empty += buffer->cb_bufSize;
		}
		if ((empty - 1) > buffer->cb_blockSize) {
			break;
		}

		/*
		 * Buffer is full. Wait for space.
		 */
		buffer->cb_notFull = B_FALSE;
		while (buffer->cb_notFull == B_FALSE) {
			PthreadCondWait(&buffer->cb_full, &buffer->cb_lock);
		}
	}

	*len = buffer->cb_blockSize;
	buf = buffer->cb_first + buffer->cb_in;
	CircularIoSetError(buffer, buf, 0);

	ASSERT_WAIT_FOR_DBX(buf != NULL);

	PthreadMutexUnlock(&buffer->cb_lock);

	return (buf);
}

/*
 * Called by producer thread.  Advance circular buffer's 'in' pointer
 * and notify consumer thread that the buffer is not empty.
 */
void
CircularIoAdvanceIn(
	CircularBuffer_t *buffer)
{
	PthreadMutexLock(&buffer->cb_lock);

	buffer->cb_in += buffer->cb_blockSize;
	if (buffer->cb_in >= buffer->cb_bufSize) {
		buffer->cb_in -= buffer->cb_bufSize;
	}

	/*
	 * Block is available.  Notify consumer thread that
	 * the buffer is not empty.
	 */
	buffer->cb_notEmpty = B_TRUE;
	PthreadCondSignal(&buffer->cb_empty);

	PthreadMutexUnlock(&buffer->cb_lock);
}

/*
 * A consumer thread must wait until the buffer is not empty.
 * Returns available data 'out' position.
 */
char *
CircularIoAvail(
	CircularBuffer_t *buffer,
	int *len,
	int *error)
{
	char *buf;
	int nbytes;

	PthreadMutexLock(&buffer->cb_lock);

	for (;;) {

		nbytes = buffer->cb_in - buffer->cb_out;
		if (nbytes < 0) {
			nbytes += buffer->cb_bufSize;
		}

		if (nbytes >= buffer->cb_blockSize) {
			/* At least one block available. */
			break;
		}

		/*
		 * Wait for buffer.
		 */
		buffer->cb_notEmpty = B_FALSE;
		while (buffer->cb_notEmpty == B_FALSE) {
			PthreadCondWait(&buffer->cb_empty, &buffer->cb_lock);
		}
	}

	buf = buffer->cb_first + buffer->cb_out;
	*len = buffer->cb_blockSize;
	*error = CircularIoGetError(buffer, buf);

	ASSERT_WAIT_FOR_DBX(buf != NULL);

	PthreadMutexUnlock(&buffer->cb_lock);

	return (buf);
}

/*
 * Called by consumer thread.  Advance circular buffer's 'out' pointer
 * and notify produce thread that the buffer is not full.
 */
void
CircularIoAdvanceOut(
	CircularBuffer_t *buffer)
{
		PthreadMutexLock(&buffer->cb_lock);

		buffer->cb_out += buffer->cb_blockSize;
		if (buffer->cb_out == buffer->cb_bufSize) {
			buffer->cb_out = 0;
		}

		/*
		 * Buffer space is available.  Notify producer thread that
		 * the buffer is not full.
		 */
		buffer->cb_notFull = B_TRUE;
		PthreadCondSignal(&buffer->cb_full);

		PthreadMutexUnlock(&buffer->cb_lock);
}

/*
 * Get circular buffer's 'in' pointer and adjusted for residual.
 * Residual is number of left over bytes in buffer.
 */
char *
CircularIoGetIn(
	CircularBuffer_t *buffer,
	int residual)
{
	char *buf;

	ASSERT(residual >= 0 && residual <= buffer->cb_blockSize);

	buf = buffer->cb_first + buffer->cb_in +
	    (buffer->cb_blockSize - residual);

	return (buf);
}

/*
 * Get circular buffer's 'out' pointer and adjusted for residual.
 * Residual is number of left over bytes in buffer.
 */
char *
CircularIoGetOut(
	CircularBuffer_t *buffer,
	int residual)
{
	char *buf;

	ASSERT(residual >= 0 && residual <= buffer->cb_blockSize);

	buf = buffer->cb_first + buffer->cb_out +
	    (buffer->cb_blockSize - residual);

	return (buf);
}

/*
 * Set block number.  Assign number to block at circular buffer's
 * pointer 'ptr'.
 */
void
CircularIoSetBlock(
    CircularBuffer_t *buffer,
	char *ptr,
	int blockNumber)
{
	int slot;

	slot =  (int)(ptr - buffer->cb_first) / buffer->cb_blockSize;
	ASSERT(slot < buffer->cb_numBuffers);
	buffer->cb_state[slot].bs_blkno = blockNumber;
}

/*
 * Set block error.  Assign errno to block at circular buffer's
 * pointer 'ptr'.
 */
void
CircularIoSetError(
    CircularBuffer_t *buffer,
	char *ptr,
	int error)
{
	int slot;

	slot =  (int)(ptr - buffer->cb_first) / buffer->cb_blockSize;
	ASSERT(slot < buffer->cb_numBuffers);
	buffer->cb_state[slot].bs_errno = error;
}

/*
 * Get block number.  Get number for block at circular buffer's
 * pointer 'ptr'.
 */
int
CircularIoGetBlock(
    CircularBuffer_t *buffer,
	char *ptr)
{
	int slot;
	int blockNumber;

	slot =  (int)(ptr - buffer->cb_first) / buffer->cb_blockSize;
	ASSERT(slot < buffer->cb_numBuffers);
	blockNumber = buffer->cb_state[slot].bs_blkno;

	return (blockNumber);
}

/*
 * Get block error.  Get errno for block at circular buffer's
 * pointer 'ptr'.
 */
int
CircularIoGetError(
    CircularBuffer_t *buffer,
	char *ptr)
{
	int slot;

	slot =  (int)(ptr - buffer->cb_first) / buffer->cb_blockSize;
	ASSERT(slot < buffer->cb_numBuffers);
	return (buffer->cb_state[slot].bs_errno);
}

/*
 * Search for block number. The caller will set/get the block number
 * for a block of data.  Is used to check if a block is already available
 * in the circular buffer and avoid media positioning.
 */
char *
CircularIoStartBlockSearch(
	CircularBuffer_t *buffer,
	int blockNumber,
	int *len)
{
	int out;
	int slot;
	char *ptr;

	out = 0;
	for (slot = 0; slot < buffer->cb_numBuffers; slot++) {
		if (buffer->cb_state[slot].bs_blkno == blockNumber) {
			break;
		}
		out += buffer->cb_blockSize;
	}

	if (slot < buffer->cb_numBuffers) {
		/*
		 * Found block. Set circular buffer's 'out' pointer.
		 * Return the 'out' pointer.
		 */
		buffer->cb_out = out;
		ptr = buffer->cb_first + out;
		*len = buffer->cb_blockSize;

		/*
		 * Set circular buffer's 'in' pointer.
		 */
		buffer->cb_in = out + buffer->cb_blockSize;
		if (buffer->cb_in >= buffer->cb_bufSize) {
			buffer->cb_in -= buffer->cb_bufSize;
		}

		/*
		 * Block is available.  Notify consumer thread that
		 * the buffer is not empty.
		 */
		buffer->cb_notEmpty = B_TRUE;

	} else {
		/*
		 * Block not found.  Reset circular buffer's pointers.
		 * Return NULL.
		 */
		CircularIoReset(buffer);
		ptr = NULL;
		*len = 0;
	}

	return (ptr);
}


/*
 * Get buffer slot number.  Get slot for block at circular buffer's
 * pointer 'ptr'.  Used for debugging.
 */
int
CircularIoSlot(
    CircularBuffer_t *buffer,
	char *ptr)
{
	int index;

	index =  (int)(ptr - buffer->cb_first) / buffer->cb_blockSize;

	return (index);
}
