/*
 * double_buffer.c - double buffer data from reader to writer threads.
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

#pragma ident "$Revision: 1.4 $"

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
#include "aml/shm.h"
#include "aml/tar.h"
#include "aml/tar_hdr.h"
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "pub/rminfo.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/opticals.h"
#include "sam/resource.h"
#include "sam/syscall.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#include "aml/id_to_path.h"
#include "aml/sam_rft.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stager_threads.h"
#include "stager_shared.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"
#include "file_defs.h"

#include "copy.h"
#include "circular_io.h"

/* Public data. */
extern CopyInstanceInfo_t *Instance;
extern IoThreadInfo_t *IoThread;
extern StreamInfo_t *Stream;

/* Private data. */
static boolean_t infiniteLoop = B_TRUE;
static longlong_t dataToMove;	/* size of file being staged */
static int cancel;		/* non-zero if canceled by file system */
static int readErrno;		/* non-zero if read or positioning error */

/* Private functions. */

/* Test for more data to move between buffers */
#define	DATA_TO_MOVE() (dataToMove > 0 && readErrno == 0 && cancel == 0)

/*
 * Double buffer thread is a consumer for reader thread.
 * A consumer must wait until the buffer is not empty, retrieve its
 * data, and then notify the producer that the buffer is not full.
 *
 * Double buffer thread is a producer for writer thread.
 * A producer must wait until the buffer is not full, deposit its
 * its data, and then notify the consumer that the buffer is not empty.
 *
 * First block for file being staged.
 *
 * +=============================+  Beginning of block
 * +                             +
 * +                             +  io_position = block number
 * +                             +
 * +-----------------------------+
 * +                             +
 * +          Tar Header         +
 * +                             +
 * +-----------------------------+  <---- io_offset
 * +                             +
 * +                             +
 * +                             +
 * +          File Data          +
 * +                             +
 * +                             +
 * +                             +
 * +=============================+  End of block
 *
 */
void *
DoubleBuffer(
	/* LINTED argument unused in function */
	void *arg)
{
	CircularBuffer_t *reader;
	CircularBuffer_t *writer;

	FileInfo_t *file;
	char *from;		/* read buffer pointer */
	char *to;		/* write buffer pointer */
	int fromResidual;	/* bytes left over in previous read buffer */
	int toResidual;		/* bytes left over in previous write buffer */
	int nbytes;		/* number of bytes for write to disk */
	int offset;		/* block offset for file being staged */
	boolean_t firstMove;	/* set if first move for file being staged */

	while (infiniteLoop) {

	/* Wait for next file to stage. */
	ThreadStateWait(&IoThread->io_moveReady);

	reader = IoThread->io_reader;
	fromResidual = 0;

	writer = IoThread->io_writer;
	toResidual = 0;

	file = IoThread->io_file;
	dataToMove = IoThread->io_size;
	offset = IoThread->io_offset;
	cancel = GET_FLAG(IoThread->io_flags, IO_cancel);
	readErrno = 0;

	/* First move for file. */
	firstMove = B_TRUE;
	CircularIoReset(writer);

	Trace(TR_DEBUG, "Double buffer inode: %d.%d offset: %d len: %lld",
	    file->id.ino, file->id.gen, offset, dataToMove);

	/* Move data from read buffer to write buffer for a file. */
	while (DATA_TO_MOVE()) {
		/*
		 * Check read buffer's residual count.  If no residual,
		 * the slot is empty so wait for another read buffer.
		 */
		if (fromResidual == 0) {
			/*
			 * Wait until the read buffer is not empty.
			 * Retrieve data at 'out' position.
			 *
			 * If archive read thread found an error during a read
			 * from media, positioning failure or tar header
			 * validation the error flag will be set in circular
			 * io control structure.
			 */
			from = CircularIoAvail(reader, &fromResidual,
			    &readErrno);
			if (readErrno != 0) {
				Trace(TR_DEBUG, "Reader found an error, "
				    "slot: %d, readErrno: %d",
				    CircularIoSlot(reader, from), readErrno);
			}
		} else {
			/*
			 * Get 'out' pointer for read buffer,
			 * adjusted for residual bytes.
			 */
			from = CircularIoGetOut(reader, fromResidual);
		}

		/*
		 * If first move for file being staged validate tar
		 * header.  Adjust for file's block offset.  May not be
		 * moving the entire block to the write buffer.
		 */
		if (firstMove == B_TRUE) {
			/*
			 * Adjust for file's block offset.  Following the
			 * adjustment our 'from' buffer is pointed at
			 * beginning of the file's data (not tar header).
			 */
			if (offset > fromResidual) {
				FatalInternalError(HERE, "invalid offset");
			}
			from += offset;
			fromResidual -= offset;
		}

		/*
		 * Check write buffer's residual count.  If no residual,
		 * the slot is full so wait for another write buffer.
		 */
		if (toResidual == 0) {
			/*
			 * Wait until the write buffer is not full.
			 * Get next available 'in' position.
			 */
			to = CircularIoWait(writer, &toResidual);
		} else {
			/*
			 * Get 'in' pointer for write buffer adjusted
			 * for residual bytes.
			 */
			to = CircularIoGetIn(writer, toResidual);
		}

		/*
		 * Initialize move count to number of bytes available
		 * in read buffer.
		 */
		nbytes = fromResidual;

		/*
		 * Write buffer residual is smaller than data available in
		 * read buffer.  Reset number of bytes to move.
		 */
		if (nbytes > toResidual) {
			nbytes = toResidual;
		}

		/*
		 * Bytes left to stage is smaller than data available in
		 * either read or write buffer.  Reset number of bytes to
		 * move.
		 */
		if (nbytes > dataToMove) {
			nbytes = dataToMove;
		}

		Trace(TR_DEBUG, "Move from: [0x%x] %d to: [0x%x] %d nbytes: %d",
		    (int)from, fromResidual, (int)to, toResidual, nbytes);

		memcpy(to, from, nbytes);

		/*
		 * Notify writer thread that the file is ready
		 * to be staged.
		 */
		if (firstMove == B_TRUE) {
			ThreadStatePost(&IoThread->io_writeReady);
			firstMove = B_FALSE;
		}

		fromResidual -= nbytes;
		toResidual -= nbytes;
		dataToMove -= nbytes;

		ASSERT_WAIT_FOR_DBX(dataToMove >= 0);
		ASSERT_WAIT_FOR_DBX(fromResidual >= 0);
		ASSERT_WAIT_FOR_DBX(toResidual >= 0);

		/*
		 * Read buffer empty.  Advance read buffer's 'out' pointer and
		 * notify reader thread that the buffer is not full.
		 */
		if (fromResidual == 0) {
			CircularIoAdvanceOut(reader);
		}

		/*
		 * Advance write buffer's 'in' pointer and notify
		 * writer thread that the buffer is not empty.
		 */
		if (toResidual == 0 || dataToMove == 0 || readErrno != 0) {
			/*
			 * If reader thread set error on this block,
			 * set block error for writer thread.
			 */
			CircularIoSetError(writer, to, readErrno);
			if (readErrno != 0) {
				Trace(TR_DEBUG, "Set error to writer buffer, "
				    "slot: %d readErrno: %d",
				    CircularIoSlot(writer, to), readErrno);
			}
			CircularIoAdvanceIn(writer);
		}

		/*
		 * If stage request is canceled, the swrite syscall
		 * in the write thread will fail and cancel flag
		 * will be set in io thread control structure.
		 */
		cancel = GET_FLAG(IoThread->io_flags, IO_cancel);

		Trace(TR_DEBUG, "Moved %d bytes left: %lld (%d/%d)",
		    nbytes, dataToMove, readErrno, cancel);
	}

	Trace(TR_DEBUG, "Double buffer complete inode: %d.%d",
	    file->id.ino, file->id.gen);

	/* Wait for write to complete */
	ThreadStateWait(&IoThread->io_writeDone);

	/* Done buffering staged file. */
	ThreadStatePost(&IoThread->io_moveDone);

	}

	return (NULL);
}
