/*
 * archive_read.c - read archive data from the removable media file
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

#pragma ident "$Revision: 1.8 $"

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
#include "pax_hdr/pax_hdr.h"
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
static CircularBuffer_t *reader;
static FileInfo_t *file;	/* file being staged */
static int filePos;		/* position of file on media to be staged */
static int fileOff;		/* block offset for file being staged */
static int noTarhdr;		/* non-zero if no tar header */
static int paxTarhdr;		/* non-zero if pax tar header */
static longlong_t dataToRead;	/* total bytes to read from media */
static int cancel;		/* non-zero if canceled by file system */
static int readError;		/* -1 if read or positioning error */
static boolean_t firstRead;	/* set if first read on file being staged */

static void findFirstBlock();
static int readBlock(char *buf, int nbytes);

static int validateTarHeader(char *in, int nbytes, int *residual);
static size_t getTarHeaderSize(char *buffer);
static boolean_t isTarHeader(char *ptr);

static boolean_t isStarHeader(char *ptr, longlong_t reqSize);
static boolean_t validateStarFileSize(struct header *tarHeader,
	longlong_t reqSize);
static boolean_t isPaxTarHeader(char *buffer, longlong_t reqSize);
static boolean_t ifVerifyFileSizeInTarhdr();

static void notifyWorkers();

static int simReadError(int ngot);

/* Test for more data to read from removable media */
#define	DATA_TO_READ() (dataToRead > 0 && readError == 0 && cancel == 0)

/*
 * The archive read thread is responsible for reading data from the
 * removable media file.
 *
 * Archive read thread is a producer.
 * A producer must wait until the buffer is not full, deposit its
 * data, and then notify the consumer that the buffer is not empty.
 *
 * All read requests must be based on the media (tape) block size.
 * Tape media i/o is fixed to a specific block size.  Each media read
 * request must be an integer multiple of the tape's block size.
 *
 * First block before tar header validation.
 *
 * +=============================+  Beginning of block
 * +                             +
 * +                             +  filePos = block number
 * +                             +
 * +-----------------------------+  <---- fileOff
 * +                             +
 * +          Tar Header         +
 * +                             +
 * +-----------------------------+
 * +                             +
 * +                             +
 * +                             +
 * +          File Data          +
 * +                             +
 * +                             +
 * +                             +
 * +=============================+  End of block
 *
 * After tar header validation, fileOff will point to beginning of
 * the file's data.  The tar header is skipped in the reader thread
 * and not sent to the double buffer thread.
 *
 */
void *
ArchiveRead(
	/* LINTED argument unused in function */
	void *arg)
{
#if 0
	int position;		/* current block position on physical media */
#endif
	longlong_t blockOff;	/* media allocation unit offset */
	char *in;		/* input buffer pointer */

	int nbytes;		/* number of bytes to read from media */
	int copy;
	int retry;		/* retry indicator */

	for (;;) {

	/* Wait for next file to stage. */
	ThreadStateWait(&IoThread->io_readReady);

	file = IoThread->io_file;
	copy = file->copy;
	SetErrno = 0;

	/*
	 * If this is a multivolume stage request and stage -n set,
	 * find a vsn based on the offset, and adjust the offset
	 * and data to read in section.
	 *
	 * If this is a multivolume stage request and not stage -n,
	 * the stage length may be greater than data available on
	 * this volume. Set data to read based on section length.
	 */

	if ((file->ar[copy].n_vsns > 1) &&
	    GET_FLAG(file->flags, FI_STAGE_NEVER)) {
		if (file->offset >= file->ar[copy].section.length) {
			/* Skip this section */
			file->offset -= file->ar[copy].section.length;
			dataToRead = 0;
		} else {
			if ((file->offset + file->residlen) >
			    file->ar[copy].section.length) {
				dataToRead = file->ar[copy].section.length -
				    file->offset;
			} else {
				dataToRead = (longlong_t)file->residlen;
			}
			file->residlen -= (u_longlong_t)dataToRead;
		}
	} else {
		if (file->len > file->ar[copy].section.length) {
			dataToRead = file->ar[copy].section.length;
		} else {
			dataToRead = (longlong_t)file->len;
		}
	}

	Trace(TR_FILES,
	    "Archive read inode: %d.%d\n\tpos: %llx.%llx len: %lld "
	    "offset: %lld residlen: %lld",
	    file->id.ino, file->id.gen, file->ar[copy].section.position,
	    file->ar[copy].section.offset, dataToRead, file->offset,
	    file->residlen);

	/*
	 * If zero length file there is no data to read.
	 * Notify disk cache write thread that there is
	 * no data to write.
	 * Go back to while loop and wait for next file
	 * to stage.
	 */
	if (dataToRead == 0) {
		fileOff = 0;
		filePos = -1;
		notifyWorkers();

		ThreadStatePost(&IoThread->io_readDone);
		continue;
	}

	/*
	 * Check if attempting to complete a stage from
	 * another copy following a failure from one copy.
	 */
	retry = GET_FLAG(file->flags, FI_RETRY) && (file->write_off > 0);
	noTarhdr = GET_FLAG(file->flags, FI_NO_TARHDR);
	paxTarhdr = GET_FLAG(file->flags, FI_PAX_TARHDR);

	if (retry != 0) {
		file->offset += file->write_off;
		/*
		 * If multivolume set data to read based on
		 * section length.
		 */
		if (file->len > file->ar[copy].section.length) {
			dataToRead = file->len - file->write_off;
			if (dataToRead > file->ar[copy].section.length) {
				dataToRead = file->ar[copy].section.length;
			}
		} else {
			dataToRead = dataToRead - file->write_off;
		}
		noTarhdr = 1;	/* no tar header on retry */
	}

	/*
	 * Calculate new offset and position based on media
	 * allocation unit size (block size).  A media allocation units is
	 * the smallest addressable part on media, ie. block on tape
	 * sector on optical.
	 */

	/*
	 * Calculate block offset, number of ma units to start of file's
	 * data. Value section.offset is offset in TAR_RECORDSIZE units
	 * to file's data.
	 */
	blockOff = file->ar[copy].section.offset;
	/* If star header to validate. */
	if (noTarhdr == 0 && paxTarhdr == 0) {
		blockOff = blockOff - 1; /* adjust for tar header */
	}

	blockOff = (blockOff * TAR_RECORDSIZE) + file->offset;
	blockOff = blockOff / IoThread->io_blockSize;

	/*
	 * Calculate file position, starting file position (block) in
	 * ma units based on copy's file position and offset.
	 */
	if (Stream->diskarch) {
		/*
		 * For a disk archive the copy's position indicates a file
		 * name, e.g. d1/f11.  The copy's offset is the seek
		 * position in the archive file.
		 */
		filePos = blockOff;
	} else {
		filePos = (int)file->ar[copy].section.position + blockOff;
	}

	/*
	 * Calculate file offset, byte offset to start of file's
	 * data in the file's mau.  If necessary, add offset to
	 * start of tar header for file being staged.
	 */
	fileOff = ((file->ar[copy].section.offset * TAR_RECORDSIZE) +
	    file->offset) - (blockOff * IoThread->io_blockSize);

	/* If star header, adjust offset for tar hdr. */
	if (noTarhdr == 0 && paxTarhdr == 0) {
		fileOff -= TAR_RECORDSIZE;
	}

	Trace(TR_DEBUG, "File position: %d", filePos);
	Trace(TR_DEBUG, "File offset: %d", fileOff);

	/* First read for file.  Used to notify double buffer thread. */
	firstRead = B_TRUE;

	readError = 0;
	cancel = GET_FLAG(IoThread->io_flags, IO_cancel);

/*	(void) time(&Control->cc_startTime); */
	reader = IoThread->io_reader;

	/* Set up for read from media. */
	findFirstBlock();

	while (DATA_TO_READ()) {
		/*
		 * Wait until the buffer is not full.
		 * Get next available 'in' position.
		 */
		in = CircularIoWait(reader, &nbytes);

		readError = readBlock(in, nbytes);

		if (readError == -1) {
			/* Media read failed. */
			CircularIoSetError(reader, in, errno);
			Trace(TR_DEBUG, "Read error, slot: %d errno: %d",
			    CircularIoSlot(reader, in), errno);

			SendCustMsg(HERE, 19031,
			    IoThread->io_drive, errno);
		}

		/*
		 * If first read on file being staged, validate and
		 * skip tar header.
		 *
		 * Adjust bytes read by tar offset to ignore the tar
		 * header size. Save len and block offset of file being
		 * staged for the double buffer thread.
		 */
		if (firstRead == B_TRUE) {
			if (readError == 0) {
				int residual;

				readError =
				    validateTarHeader(in, nbytes, &residual);

				if (readError == -1) {
					/* Tar header validation failed. */
					CircularIoSetError(reader, in, EIO);
					Trace(TR_DEBUG, "Tar header error, "
					    "slot: %d",
					    CircularIoSlot(reader, in));
				}

				/* Number of data bytes left in buffer. */
				nbytes = residual;

				/*
				 * After skipping the tar header there may
				 * not be any any file data in the buffer,
				 * nbytes = 0.  Start thread pipeline but
				 * there will be no data to move or write.
				 */
			}
			/*
			 * Start thread pipeline to other threads.
			 */
			notifyWorkers();
		}

		/*
		 * Read complete. Advance read buffer's 'in'
		 * pointer and notify double buffer thread that the buffer
		 * is not empty. Set block position for the buffer.
		 */
		CircularIoSetBlock(reader, in, filePos);
		CircularIoAdvanceIn(reader);

		/*
		 * Read a complete block. For completeness, if bytes left
		 * to read is less than the block reset number of bytes
		 * read.
		 */
		if (nbytes > dataToRead) {
			nbytes = dataToRead;
		}

		dataToRead -= nbytes;
		filePos++;

		/*
		 * If stage request is canceled, the swrite sycall
		 * in the write thread will fail and cancel flag
		 *  will be set in io thread control structure.
		 */
		cancel = GET_FLAG(IoThread->io_flags, IO_cancel);

		Trace(TR_DEBUG, "Read %d bytes left: %lld (%d/%d)",
		    nbytes, dataToRead, readError, cancel);
	}

	/*
	 * If multivolume, no read error and stage -n set,
	 * read on next volume starts at offset 0.
	 * If read error, don't clear the offset for retry
	 * from the next available copy.
	 */
	if ((file->residlen > 0) && (readError == 0) &&
	    GET_FLAG(file->flags, FI_STAGE_NEVER)) {
		file->offset = 0;
	}

	Trace(TR_FILES, "Archive read complete inode: %d.%d",
	    file->id.ino, file->id.gen);

	/* Update current block position on media. */
	IoThread->io_position = filePos;

	/* Wait for double buffering to complete */
	ThreadStateWait(&IoThread->io_moveDone);

	/* Notify waiter that staging is complete */
	ThreadStatePost(&IoThread->io_readDone);
	}

#if defined(lint)
	/* LINTED statement not reached */
	return (NULL);
#endif
}

/*
 * Position media.
 */
int
SetPosition(
	int from,
	int to)
{
	FileInfo_t *file;
	int cancel;
	int position;

	if (from == -1) {
		Trace(TR_FILES, "Set position to: %d(0x%x)", to, to);
	} else {
		Trace(TR_FILES, "Set position from: %d(0x%x) to: %d(0x%x)",
		    from, from, to, to);
	}

	position = from;
	cancel = GET_FLAG(IoThread->io_flags, IO_cancel);
	if (from != to && cancel == 0) {
		/*
		 * Positioning flag used only in status displays.
		 */
		file = IoThread->io_file;
		SET_FLAG(file->flags, FI_POSITIONING);

		position = SeekVolume(to);

		CLEAR_FLAG(file->flags, FI_POSITIONING);
	}
	return (position);
}

/*
 * Set up for read from media.
 *
 * Find first block to transfer.  Blocks may already available
 * in buffer. If one or more blocks are found, notify other threads
 * that data is available.
 *
 * After using all blocks in buffers, and if more data to read,
 * position media for read.
 */
static void
findFirstBlock()
{
	char *ptr;
	int nbytes;
	int residual;
	int toPos;

	ptr = CircularIoStartBlockSearch(reader, filePos, &nbytes);

	if (ptr != NULL) {

		Trace(TR_DEBUG, "Reuse block: %d buf: %d [0x%x] len: %d "
		    "error: %d",
		    filePos, CircularIoSlot(reader, ptr),
		    (int)ptr, nbytes, readError);

		readError = validateTarHeader(ptr, nbytes, &residual);

		if (readError == -1) {
			/* Tar header validation failed. */
			CircularIoSetError(reader, ptr, EIO);
			Trace(TR_DEBUG, "Tar header error, slot: %d",
			    CircularIoSlot(reader, ptr));
		}

		/*
		 * After skipping the tar header there may not be any
		 * any file data in the buffer, nbytes = 0.  Start
		 * thread pipeline but there will be no data to move
		 * or write.
		 *
		 * Start thread pipeline to other threads.
		 */
		notifyWorkers();

		/* Number of bytes read for staged file from block. */
		dataToRead -= residual;
		filePos++;

		/* FIXME */
#if 0
		while (in != NULL && DATA_TO_READ()) {

			filePos++;
			in = CircularIoNextBlockSearch(reader, filePos);

			/*
			 * If stage request is canceled, the swrite sycall
			 * in the write thread will fail and cancel flag
			 *  will be set in io thread control structure.
			 */
			cancel = GET_FLAG(IoThread->io_flags, IO_cancel);
		}
#endif
	}

	/*
	 * Drained blocks in buffers. If more data to read,
	 * position media.
	 */
	if (DATA_TO_READ()) {

		toPos = SetPosition(IoThread->io_position, filePos);

		if (toPos >= 0) {
			/* Media has been positioned for read. */
			filePos = toPos;
		} else {
			char *ptr;
			int nbytes;

			/* Positioning failed. Set read error. */
			Trace(TR_DEBUG, "Positioning err: %d", errno);
			if (errno == 0) {
				SetErrno = EIO;
			}

			/*
			 * Notify move thread that the buffer
			 * is not empty so error is detected.
			 */
			ptr = CircularIoWait(reader, &nbytes);
			CircularIoSetBlock(reader, ptr, filePos);
			CircularIoSetError(reader, ptr, errno);
			Trace(TR_DEBUG, "Positioning error to 0x%x, slot: %d "
			    " errno: %d",
			    filePos, CircularIoSlot(reader, ptr), errno);
			CircularIoAdvanceIn(reader);
			readError = -1;
			SendCustMsg(HERE, 19030, filePos,
			    IoThread->io_drive, errno);
			if (firstRead == B_TRUE) {
				notifyWorkers();
			}
		}
	}
}

/*
 * Read data block from media.  Upon successful completion return a zero.
 * Otherwise, this function returns a -1 and errno is set.
 */
static int
readBlock(
	char *buf,
	int nbytes)
{
	boolean_t retval;
	int ngot;
	sam_id_t id;

	id = file->id;

	/* FIXME SetTimeout(TO_read); */
	Trace(TR_DEBUG, "Read block: %d buf: %d [0x%x] len: %d",
	    filePos, CircularIoSlot(reader, buf),
	    (int)buf, nbytes);

retry:

	if (IoThread->io_flags & IO_disk) {
		ngot = SamrftRead(IoThread->io_rftHandle, buf, nbytes);

	} else if (IoThread->io_flags & IO_stk5800) {
		ngot = HcReadArchiveFile(IoThread->io_rftHandle, buf,
		    nbytes);

	} else if (IoThread->io_flags & IO_samremote) {
		ngot = SamrftRead(IoThread->io_rftHandle, buf, nbytes);

	} else {
		ngot = read(IoThread->io_rmFildes, buf, nbytes);
	}

/* FIXME */
	ngot = simReadError(ngot);
	/*
	 * Check if read was successful.  Either read the entire block
	 * or a short read at end of tarball is okay.
	 */
	if ((ngot == nbytes) ||
	    (ngot > 0 && ngot >= dataToRead && errno == 0)) {

		file->read += ngot;
		retval = 0;	/* successful read */

	} else {

		/* FIXME - Check for empty sector condition on optical. */
		/* FIXME - too many error messages */

		/* Read failed but errno was not set. */
		if (errno == 0) {
			SetErrno = EIO;
		}

		SysError(HERE,
		    "Stager read failed inode: %d.%d expected: %d got: %d",
		    id.ino, id.gen, nbytes, ngot);

		/* Check if read error should be retried. */
		if (IfRetry(file, errno) == B_TRUE) {

			/* Clear errno before retry. */
			SetErrno = 0;

			/* Reposition media and retry read. */
			(void) SetPosition(-1, filePos);
			goto retry;
		}

		retval = -1;	/* read block failed */
	}

	return (retval);
}

/*
 * Validate and skip tar header. Upon successful completion return a 0.
 * Otherwise, this function returns a -1.
 *
 * After skipping the tar header, return the number of bytes left
 * in the buffer.  This is the number of bytes available to the file
 * being staged and will be pipelined to the double buffer thread.
 */
static int
validateTarHeader(
	char *in,
	int nbytes,
	int *residual)
{
	static char *hdrBuffer = NULL;
	static int hdrBufLen = 0;

	int retval;
	char *buffer;
	int bufLen;
	int tarHeaderSize;
	boolean_t validHeader;

	/* No tar header to validate. */
	if (noTarhdr) {
		*residual = nbytes - fileOff;
		return (0);
	}

	/* Bytes in buffer. */
	bufLen = nbytes;

	/* Adjust buffer pointer to tar header. */
	buffer = in + fileOff;
	bufLen -= fileOff;

	Trace(TR_DEBUG, "Validate tar from: [0x%x] %d", (int)buffer, bufLen);

	retval = 0;
	tarHeaderSize = getTarHeaderSize(buffer);

	/*
	 * Check if block contains the complete tar header.
	 * If not, save partial header and get the next block.
	 */
	if (tarHeaderSize > bufLen) {
		if (hdrBuffer == NULL) {
			SamMalloc(hdrBuffer, tarHeaderSize);
		} else if (tarHeaderSize > hdrBufLen) {
			SamRealloc(hdrBuffer, tarHeaderSize);
		}

		hdrBufLen = tarHeaderSize;
		memcpy(hdrBuffer, buffer, bufLen);
		tarHeaderSize -= bufLen;

		buffer = hdrBuffer;

		/* Read the next block. */
		filePos++;
		retval = readBlock(in, nbytes);
		if (retval == 0) {
			memcpy(buffer + bufLen, in, tarHeaderSize);
		}

		/*
		 * Set buffer pointer and length for the next block.
		 * After tar header validation, fileOff will point to
		 * beginning of the file's data.  bufLen is returned as
		 * count of residual data in the block.
		 */
		fileOff = 0;
		bufLen = nbytes;
	}

	if (retval == 0) {
		validHeader = isTarHeader(buffer);
		if (validHeader == B_TRUE) {
			/* Adjusting buffer pointer to start of file's data. */
			fileOff += tarHeaderSize;
			bufLen -= tarHeaderSize;
		} else {
			retval = -1;
		}
	}

	*residual = bufLen;
	return (retval);
}

/*
 * Returns size of tar header.
 */
static size_t
getTarHeaderSize(
	char *buffer)
{
	size_t hdrSize;

	/* No tar header to validate. */
	if (noTarhdr) {
		return (0);
	}
	if (paxTarhdr != 0) {
		(void) ph_is_pax_hdr(buffer, &hdrSize);
		hdrSize += PAX_HDR_BLK_SIZE;
	} else {
		hdrSize = TAR_RECORDSIZE;
	}
	return (hdrSize);
}

/*
 * Returns true if valid tar header.  Validate magic and request
 * length against tar header's file size.
 */
static boolean_t
isTarHeader(
	char *buffer)
{
	boolean_t valid;

	if (paxTarhdr != 0) {
		valid = isPaxTarHeader(buffer, dataToRead);
	} else {
		valid = isStarHeader(buffer, dataToRead);
	}

	if (valid == B_FALSE) {
		char pathBuffer[PATHBUF_SIZE];

		SetErrno = 0;		/* set for trace */
		Trace(TR_ERR, "Invalid tar header inode: %d.%d",
		    file->id.ino, file->id.gen);

		GetFileName(file, &pathBuffer[0], PATHBUF_SIZE, NULL);

		SendCustMsg(HERE, 19032, pathBuffer, file->id.ino,
		    file->id.gen, file->copy + 1);

		SET_FLAG(file->flags, FI_TAR_ERROR);
	}

	return (valid);
}

/*
 * Returns true if valid star header.
 */
static boolean_t
isStarHeader(
	char *ptr,
	longlong_t reqSize)
{
	boolean_t valid;
	struct header *tarHeader;

	Trace(TR_DEBUG, "Validate star header inode: %d.%d",
	    file->id.ino, file->id.gen);

	valid = B_TRUE;
	tarHeader = (struct header *)ptr;

	if (memcmp(&tarHeader->magic, TMAGIC, sizeof (TMAGIC)) != 0) {
		TraceRawData(TR_MISC, ptr, TAR_RECORDSIZE);
		valid = B_FALSE;
	}

	if (valid == B_TRUE) {
		valid = validateStarFileSize(tarHeader, reqSize);

	}

	return (valid);
}

/*
 * Validate request size against tar header's file size.
 */
static boolean_t
validateStarFileSize(
	struct header *tarHeader,
	longlong_t reqSize)
{
	boolean_t valid;
	u_longlong_t tarFileSize;
	boolean_t verifySize;

	verifySize = ifVerifyFileSizeInTarhdr();

	if (verifySize == B_FALSE) {
		/* Unable to validate size for this type of request. */
		return (B_TRUE);
	}

	valid = B_TRUE;
	tarFileSize = llfrom_str(sizeof (tarHeader->size), tarHeader->size);
	if (tarFileSize != reqSize) {
#define	OLDHDRMAX 07777777777
		if (reqSize <= OLDHDRMAX) {
			Trace(TR_MISC, "Request length (%lld) does "
			    "not match tar header (%lld) file size",
			    reqSize, tarFileSize);
			valid = B_FALSE;
		} else if (tarFileSize != (reqSize & OLDHDRMAX)) {
			Trace(TR_MISC, "Request length adjusted "
			    "to %lld", reqSize & OLDHDRMAX);
			Trace(TR_MISC, "Request length (%lld) does "
			    "not match tar header (%lld) file size",
			    reqSize & OLDHDRMAX, tarFileSize);
			valid = B_FALSE;
		}
	}
	return (valid);
}

/*
 * Returns true if valid pax tar header.
 */
static boolean_t
isPaxTarHeader(
	char *buffer,
	longlong_t reqSize)
{
	boolean_t valid;
	int type;
	pax_hdr_t *hdr;
	boolean_t verifySize;
	size_t hdrSize;
	offset_t tarFileSize;

	Trace(TR_DEBUG, "Validate pax tar header inode: %d.%d",
	    file->id.ino, file->id.gen);

	valid = B_TRUE;
	hdrSize = 0;

	type = ph_is_pax_hdr(buffer, &hdrSize);
	/* SAM only generates extended format. */
	if (type != PX_SUCCESS_EXT_HEADER) {
		valid = B_FALSE;
		SetErrno = 0;		/* set for trace */
		Trace(TR_ERR, "ph_is_pax_hdr failed %d", type);
		TraceRawData(TR_MISC, buffer, PAX_HDR_BLK_SIZE);
	}

	/* Check if file size can be verified for this type of request. */
	verifySize = ifVerifyFileSizeInTarhdr();

	if (valid == B_TRUE && verifySize == B_TRUE) {
		int rval;

		hdrSize += PAX_HDR_BLK_SIZE;
		hdr = ph_create_hdr();

		rval = ph_load_pax_hdr(hdr, buffer, hdrSize);

		if (rval == PX_SUCCESS) {
			rval = ph_get_size(hdr, &tarFileSize);

			if (rval != PX_SUCCESS || tarFileSize != reqSize) {
				valid = B_FALSE;
				SetErrno = 0;		/* set for trace */
				Trace(TR_ERR, "Request length (%lld) does "
				    "not match pax header (%lld) file size",
				    reqSize, tarFileSize);
			}
		} else {
			valid = B_FALSE;
			SetErrno = 0;		/* set for trace */
			Trace(TR_ERR, "ph_load_pax_hdr failed %d", rval);
		}

		if (valid == B_FALSE) {
			/* Invalid header. Raw dump the pax header. */
			TraceRawData(TR_MISC, buffer, hdrSize);
		}

		if (hdr != NULL) {
			ph_destroy_hdr(hdr);
		}
	}

	return (valid);
}

/*
 * Set true if able to verify file size in tar header against
 * stage request size.
 */
static boolean_t
ifVerifyFileSizeInTarhdr()
{
	boolean_t verifySize;

	verifySize = B_TRUE;

	if (GET_FLAG(file->flags, FI_MULTIVOL) ||
	    GET_FLAG(file->flags, FI_STAGE_NEVER) ||
	    GET_FLAG(file->flags, FI_STAGE_PARTIAL)) {

		/* Unable to validate this type of request. */
		verifySize = B_FALSE;
	}
	return (verifySize);
}

/*
 * Start the stage pipeline. Each thread will wait for a data buffer
 * passed from its producer and pass it along the the next or final
 * thread.
 *
 * Notify double buffer thread that the read buffer is not empty.
 * The double buffer thread will notify the write thread when data
 * is available.
 *
 */
static void
notifyWorkers()
{
	/*
	 * Update io thread fields.  Be sure to update
	 * any fields which are file based.
	 *
	 * Save len and block offset of file being staged for the double
	 * buffer thread.
	 */
	IoThread->io_size = dataToRead;
	IoThread->io_offset = fileOff;
	IoThread->io_position = filePos;

	CLEAR_FLAG(IoThread->io_flags, IO_cancel);

	/*
	 * If zero length file, no need to notify
	 * double buffer thread but need to notify
	 * disk cache write thread.
	 * Otherwise, notify double buffer thread that
	 * next file in stream is ready to be staged.
	 */
	if (dataToRead == 0) {
		ThreadStatePost(&IoThread->io_writeReady);
		ThreadStateWait(&IoThread->io_writeDone);
	} else {
		ThreadStatePost(&IoThread->io_moveReady);
	}

	firstRead = B_FALSE;
}

/*
 * Simulate read error from media.  Return a zero if no simulated read error
 * should be generated.  Otherwise, this function returns a -1 and
 * errno is set.
 */
static int
simReadError(
	int ngot)
{
/* FIXME based on file name */
	int retval;

	retval = ngot;

#if 0
	retval = -1;
	SetErrno = EIO;

	Trace(TR_MISC, "SIM read error");
#endif

	return (retval);
}
