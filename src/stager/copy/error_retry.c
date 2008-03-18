/*
 * error_retry.c - stager's error retry processing
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.36 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "sam/resource.h"
#include "sam/fs/amld.h"
#include "sam/syscall.h"
#include "aml/tar.h"
#include "sam/lib.h"
#include "sam/custmsg.h"

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "stage_reqs.h"
#include "file_defs.h"
#include "copy_defs.h"
#include "copy.h"
#include "error_retry.h"

/* Public data. */
extern CopyInstance_t *Context;
extern CopyControl_t *Control;

/* Private functions. */
static boolean_t isRetryError(int error);

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/*
 * Check if stage error should be retried.
 */
int
IsRetryError(
	FileInfo_t *file)
{
	int retry = FALSE;
	ASSERT(file != 0);

	Trace(TR_ERR, "Error retry started inode: %d copy %d errno %d",
	    file->id.ino, file->copy + 1, errno);

	if (Control->flags & CC_honeycomb) {
		/* No retry if honeycomb */
		return (FALSE);
	}

	/*
	 *  Check if error can be retried.
	 */
	if (isRetryError(errno)) {
		/*
		 * Decrement maximum retry attempts for this copy.
		 */
		file->retry--;

		/*
		 * Check if max retry count for this copy exhausted.
		 */
		if (file->retry >= 0) {
			/*
			 * Max retry count not exhausted.
			 */
			retry = TRUE;
			SetErrno = 0;
		}
	}

	return (retry);
}

/*
 * Max retry count on this copy exhausted.  Check if another
 * copy is available or stage error response should be sent
 * to filesystem.
 */
void
SendErrorResponse(
	FileInfo_t *file)
{
	int copy = -1;
	int i, j;

	/*
	 *	Don't allow retry if multivolume and not section 0.
	 */
	if (GET_FLAG(file->flags, FI_WRITE_ERROR) || file->error == ECOMM ||
	    file->ar[file->copy].ext_ord != 0 || file->se_ord != 0) {

		CLEAR_FLAG(file->flags, FI_DCACHE);

		ErrorFile(file);
		return;
	}

	i = file->copy;
	j = 0;
	while (j < MAX_ARCHIVE) {
		if (i >= MAX_ARCHIVE) {
			i = 0;
		}
		if (file->copy != i) {
			if (file->ar[i].flags & STAGE_COPY_ARCHIVED) {
				if ((file->ar[i].flags & (STAGE_COPY_STALE |
				    STAGE_COPY_DAMAGED)) == 0) {
					copy = i;
					break;
				}
			}
		}
		j++;
		i++;
	}
	if (copy == -1) {
		/*
		 * Nothing left to try.  Give error response to
		 * filesystem and close the file.
		 */
		CLEAR_FLAG(file->flags, FI_DCACHE);
		ErrorFile(file);
	}
}

/*
 * Read retry scenios on optical.
 *	Empty sector.
 *	Bus reset.
 *
 * An empty sector is encountered at the end of each archive set.
 * Since we are reading large chunks of data the driver errors on
 * an empty sector, returning an EIO and -1 for number of bytes read.
 * The following code attempts to read only the amount of data
 * left in this file.  This should be fixed in the driver.  (FIXME)
 *
 * A bus reset occurred, decrease the stage buffer and try the
 * read again.
 */
int
RetryRead(
	int in,
	longlong_t total_read,
	int file_position)
{
	int numbytes_read;
	int mau_in_buffer;
	int count;
	boolean_t error;

	extern CopyInstance_t *Context;
	extern CopyControl_t *Control;

	if ((Context->media & DT_CLASS_MASK) != DT_OPTICAL) {
		return (-1);
	}

	if (errno != EIO) {
		return (-1);
	}

	if (total_read < Control->bufSize) {
		/*
		 * Retry empty sector on optical.
		 */
		Trace(TR_ERR, "Empty sector retry total_read: %lld",
		    total_read);

		SetErrno = 0;
		(void) SetPosition(0, file_position);

		mau_in_buffer = total_read / Control->mau;
		if (total_read % Control->mau) {
			mau_in_buffer++;
		}
		count = mau_in_buffer * Control->mau;

		Trace(TR_MISC, "Start empty sector read in: %d count: %d",
		    in, count);

		if (IS_REMOTE_STAGE(Context)) {
			numbytes_read = SamrftRead(Control->rft,
			    Control->buffers[in].data, count);
		} else {
			numbytes_read = read(Control->rmfd,
			    Control->buffers[in].data, count);
		}

		Trace(TR_MISC, "End empty sector read %d", numbytes_read);

		/*
		 * Return if empty sector read was okay.
		 */
		if (numbytes_read == count) {
			return (numbytes_read);
		}
	}

	/*
	 * Retry bus reset on optical.
	 */
	Trace(TR_ERR, "Optical read error retry");

	count = Control->bufSize;
	error = B_TRUE;

	while (error && count > (8 * 1024)) {

		sleep(4);
		SetErrno = 0;
		(void) SetPosition(0, file_position);

		count = count >> 1;

		Trace(TR_MISC, "Start optical read error retry "
		    "in: %d count: %d", in, count);

		if (IS_REMOTE_STAGE(Context)) {
			numbytes_read = SamrftRead(Control->rft,
			    Control->buffers[in].data, count);
		} else {
			numbytes_read = read(Control->rmfd,
			    Control->buffers[in].data, count);
		}
		Trace(TR_MISC, "End optical read error retry %d",
		    numbytes_read);

		if (numbytes_read == count) {
			error = B_FALSE;
		}
	}
	return (numbytes_read);
}


/*
 * Return TRUE if error can be retried.
 */
static boolean_t
isRetryError(
	int error)
{
	boolean_t retry;

	switch (error) {
		case ECANCELED:
		case EINTR:
		case EREMCHG:
		case ENOMEM:
		case ENOSPC:
		case EPERM:
		case ENOTSUP:
		case ENODEV:
			retry = B_FALSE;
			break;
		default:
			retry = B_TRUE;
	}
	return (retry);
}

/*
 * Error could not be recovered from.  Give error response to filesystem
 * and close the file.
 */
void
ErrorFile(
	FileInfo_t *file)
{
	extern int NumOpenFiles;
	int syserr;
	sam_fsstage_arg_t arg;

	Trace(TR_MISC, "Error response inode: %d error: %d fd: %d",
	    file->id.ino, file->error, file->dcache);

	memset(&arg.handle, 0, sizeof (sam_handle_t));

	arg.handle.id = file->id;
	arg.handle.fseq = file->fseq;

	arg.handle.stage_off = file->fs.stage_off;
	arg.handle.stage_len = file->len;
	arg.handle.flags.b.stage_wait = file->fs.wait;

	if (file->error == EDVVCMP) {
		arg.ret_err = EIO;		/* checksum error */
	} else {
		arg.ret_err = file->error;
	}

	if (file->dcache == -1) {
		syserr = sam_syscall(SC_fsstage, &arg, sizeof (arg));
		if (syserr == -1) {
			WarnSyscallError(HERE, "fsstage", "");
		}

	} else {
		syserr = sam_syscall(SC_fsstage_err, &arg, sizeof (arg));
		if (syserr == -1) {
			WarnSyscallError(HERE, "fsstage_err", "");
		}

		if (close(file->dcache) == -1) {
			WarnSyscallError(HERE, "close", "");
		}
		NumOpenFiles--;
	}
}
