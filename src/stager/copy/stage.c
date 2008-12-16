/*
 * stage.c - support functions for staging from archive files.
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

#pragma ident "$Revision: 1.31 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <values.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "aml/tar.h"
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "pub/rminfo.h"
#include "aml/opticals.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stager_threads.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"

#include "copy.h"
#include "circular_io.h"

/* Public data. */
extern CopyInstanceInfo_t *Instance;
extern IoThreadInfo_t *IoThread;
extern StreamInfo_t *Stream;

/* Private data. */
static CopyInstanceInfo_t *errInstance = NULL;

static void switchInstance(CopyInstanceInfo_t *from, boolean_t save);

/*
 * Init stage.
 */
void
StageInit(
	char *new_vsn)
{
	/*
	 * Clear vsn field in context if not loading the same VSN as the
	 * last time this proc was active.  Its important to clear the vsn
	 * from the context so the scheduler does not think this proc is
	 * busy trying to stage from the last VSN.  Yet we want to maintain
	 * this information so its possible to reuse data in the buffers if
	 * the same VSN is being loaded again.  The checkBuffers function will
	 * deal with buffer reuse after the VSN has been loaded and the tape
	 * block size is obtained for the media.
	 */
	if (strcmp(Instance->ci_vsn, new_vsn) != 0) {
		Instance->ci_vsn[0] = '\0';
	}

	/*
	 * If switching to another archive copy for error recovery we
	 * may be switching to another type of media.  Find a
	 * context for the new media type.
	 */
	if (Instance->ci_media != Stream->media) {

		errInstance = FindCopyInstanceInfo(Stream->lib, Stream->media);
		switchInstance(errInstance, B_TRUE);

		if (is_disk(errInstance->ci_media)) {
			if (is_stk5800(errInstance->ci_media)) {
				IoThread->io_flags |= IO_stk5800;
			} else {
				IoThread->io_flags |= IO_disk;
			}
		} else {
			/* staging for sam remote */
			if (errInstance->ci_flags & CI_samremote) {
				IoThread->io_flags |= IO_samremote;
			}

		}
	}

	if (IoThread->io_flags & IO_disk) {
		DkInit();
	} else if (IoThread->io_flags & IO_stk5800) {
		HcInit();
	} else {
		RmInit();
	}
}

/*
 * End stage.
 */
void
StageEnd(void)
{
	if (errInstance != NULL) {
		switchInstance(NULL, B_FALSE);
		errInstance = NULL;

		CircularIoDestructor(IoThread->io_reader);
		CircularIoDestructor(IoThread->io_writer);

		IoThread->io_numBuffers = 0;
		IoThread->io_blockSize = 0;
	}
}

/*
 * Open archive file.
 */
int
LoadVolume(void)
{
	int rval;

	if (IoThread->io_flags & IO_disk) {
		rval = DkLoadVolume();
	} else if (IoThread->io_flags & IO_stk5800) {
		rval = HcLoadVolume();
	} else {
		rval = RmLoadVolume();
	}

	return (rval);
}

/*
 * Open next archive file.
 */
int
NextArchiveFile(void)
{
	int rval;

	rval = 0;
	if (IoThread->io_flags & IO_disk) {
		rval = DkNextArchiveFile();
	} else if (IoThread->io_flags & IO_stk5800) {
		rval = HcNextArchiveFile();
	}
	return (rval);
}

/*
 * End of archive file.
 */
void
EndArchiveFile(void)
{
	if (IoThread->io_flags & IO_disk) {
		DkEndArchiveFile();
	} else if (IoThread->io_flags & IO_stk5800) {
		HcEndArchiveFile();
	}
}

/*
 * Get block size for mounted media.
 */
int
GetBlockSize(void)
{
	int blockSize = 0;

	if (IoThread->io_flags & IO_diskArchiving) {
		/*
		 * Media allocation unit(mau) for disk archive media.
		 */
		blockSize = OD_BS_DEFAULT;
	} else {
		blockSize = RmGetBlockSize();
	}
	return (blockSize);
}

#if 0
/*
 * Get buffer size for mounted media.
 */
offset_t
GetBufferSize(
	int block_size)
{
	offset_t buffer_size = 0;

	if (Control->flags & CC_diskArchiving) {
		buffer_size = OD_BS_DEFAULT;
	} else {
		buffer_size = (offset_t)GetRmBufferSize(block_size);
	}
	return (buffer_size);
}
#endif

/*
 * Get position of archive media.
 */
u_longlong_t
GetPosition(void)
{
	u_longlong_t position;

	if (IoThread->io_flags & IO_disk) {
		position = DkGetPosition();
	} else if (IoThread->io_flags & IO_stk5800) {
		position = HcGetPosition();
	} else {
		position = RmGetPosition();
	}
	return (position);
}

/*
 * Position archive file.
 */
int
SeekVolume(
	int to)
{
	int position;

	if (IoThread->io_flags & IO_disk) {
		position = DkSeekVolume(to);
	} else if (IoThread->io_flags & IO_stk5800) {
		position = HcSeekVolume(to);
	} else {
		position = RmSeekVolume(to);
	}
	return (position);
}

/*
 * Get drive number for the mounted VSN.
 * Zero if disk archive.
 */
equ_t
GetDriveNumber(void)
{
	equ_t eq;

	if (IoThread->io_flags & IO_diskArchiving) {
		eq = 0;
	} else {
		eq = RmGetDriveNumber();
	}
	return (eq);
}

/*
 * Unload/close archiver file.
 */
void
UnloadVolume(void)
{
	if (IoThread->io_flags & IO_disk) {
		DkUnloadVolume();
	} else if (IoThread->io_flags & IO_stk5800) {
		HcUnloadVolume();
	} else {
		RmUnloadVolume();
	}
}

/*
 * Switching to another type of media for error recovery.
 * Save or restore context.
 */
static void
switchInstance(
	CopyInstanceInfo_t *from,
	boolean_t save)
{
	static CopyInstanceInfo_t saveInstance;

	if (save) {
		memcpy(&saveInstance, Instance, sizeof (saveInstance));

		Trace(TR_MISC, "Switch instance (0x%x) "
		    "curr: '%s' 0x%x to: '%s' 0x%x",
		    (int)Instance,
		    sam_mediatoa(Instance->ci_media), Instance->ci_flags,
		    sam_mediatoa(from->ci_media), from->ci_flags);

		Instance->ci_media = from->ci_media;
		Instance->ci_numBuffers = from->ci_numBuffers;
		Instance->ci_flags = from->ci_flags;
		Instance->ci_eq = from->ci_eq;
	} else {

		/*
		 * Restore from saved context.
		 */

		Trace(TR_MISC, "Restore instance (0x%x) "
		    "curr: '%s' 0x%x to: '%s' 0x%x",
		    (int)Instance,
		    sam_mediatoa(Instance->ci_media), Instance->ci_flags,
		    sam_mediatoa(saveInstance.ci_media), saveInstance.ci_flags);

		Instance->ci_media = saveInstance.ci_media;
		Instance->ci_numBuffers = saveInstance.ci_numBuffers;
		Instance->ci_flags = saveInstance.ci_flags;
		Instance->ci_eq = saveInstance.ci_eq;
	}

	/* Update io thread to reflect changed context. */
	IoThread->io_flags = 0;
	if (is_disk(Instance->ci_media)) {
		if (is_stk5800(Instance->ci_media)) {
			IoThread->io_flags |= IO_stk5800;
		} else {
			IoThread->io_flags |= IO_disk;
		}
	}
}
