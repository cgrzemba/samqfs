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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.27 $"

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

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stage_reqs.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"
#include "copy.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Public data. */
extern CopyInstance_t *Context;

/*
 * Control information for process.
 */
extern CopyControl_t *Control;

/*
 * Active stream for process.
 */
extern StreamInfo_t *Stream;

/* Private data. */
static CopyInstance_t *errContext = NULL;

static void switchContext(CopyInstance_t *from, boolean_t save);

/*
 * Init stage.
 */
void
InitStage(
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
	if (strcmp(Context->vsn, new_vsn) != 0) {
		Context->vsn[0] = '\0';
	}

	/*
	 * If switching to another archive copy for error recovery we
	 * may be switching to another type of media.  Find a
	 * context for the new media type.
	 */
	if (Context->media != Stream->media) {

		errContext = FindCopyProc(Stream->lib, Stream->media);
		switchContext(errContext, B_TRUE);

		if (is_disk(errContext->media)) {
			if (is_stk5800(errContext->media)) {
				Control->flags |= CC_honeycomb;
			} else {
				Control->flags |= CC_disk;
			}
		}

		if (Control->numBuffers != 0) {
			SamFree(Control->buffers[0].data);
			Control->numBuffers = 0;
		}
	}

	if (Control->flags & CC_disk) {
		InitDkStage();
	} else if (Control->flags & CC_honeycomb) {
		HcStageInit();
	} else {
		InitRmStage();
	}
}

/*
 * End stage.
 */
void
EndStage(void)
{
	if (errContext != NULL) {
		switchContext(NULL, B_FALSE);
		errContext = NULL;

		if (Control->numBuffers != 0) {
			SamFree(Control->buffers[0].data);
			Control->numBuffers = 0;
		}
	}
}

/*
 * Open archive file.
 */
int
LoadVolume(void)
{
	int error = 0;

	if (Control->flags & CC_disk) {
		error = LoadDkVolume();
	} else if (Control->flags & CC_honeycomb) {
		error = HcStageLoadVolume();
	} else {
		error = LoadRmVolume();
	}

	return (error);
}

/*
 * Open next archive file.
 */
int
NextArchiveFile(void)
{
	int error = 0;

	if (Control->flags & CC_disk) {
		error = NextDkArchiveFile();
	} else if (Control->flags & CC_honeycomb) {
		error = HcStageNextArchiveFile();
	}
	return (error);
}

/*
 * End of archive file.
 */
void
EndArchiveFile(void)
{
	if (Control->flags & CC_disk) {
		EndDkArchiveFile();
	} else if (Control->flags & CC_honeycomb) {
		HcStageEndArchiveFile();
	}
}

/*
 * Get block size for mounted media.
 */
int
GetBlockSize(void)
{
	int block_size = 0;

	if (Control->flags & CC_diskArchiving) {
		/*
		 * Media allocation unit(mau) for disk archive media.
		 */
		block_size = OD_SS_DEFAULT;
	} else {
		block_size = GetRmBlockSize();
	}
	return (block_size);
}

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

/*
 * Get position of archive file.
 */
u_longlong_t
GetPosition(void)
{
	u_longlong_t position;

	if (Control->flags & CC_disk) {
		position = GetDkPosition();
	} else if (Control->flags & CC_honeycomb) {
		position = HcStageGetPosition();
	} else {
		position = GetRmPosition();
	}
	return (position);
}

/*
 * Position archive file.
 */
int
SeekVolume(
	int to_pos)
{
	int position;

	if (Control->flags & CC_disk) {
		position = SeekDkVolume(to_pos);
	} else if (Control->flags & CC_honeycomb) {
		position = HcStageSeekVolume(to_pos);
	} else {
		position = SeekRmVolume(to_pos);
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

	if (Control->flags & CC_diskArchiving) {
		eq = 0;
	} else {
		eq = GetRmDriveNumber();
	}
	return (eq);
}

/*
 * Unload/close archiver file.
 */
void
UnloadVolume(void)
{
	if (Control->flags & CC_disk) {
		UnloadDkVolume();
	} else if (Control->flags & CC_honeycomb) {
		HcStageUnloadVolume();
	} else {
		UnloadRmVolume();
	}
}

/*
 * Switching to another type of media for error recovery.
 * Save or restore context.
 */
static void
switchContext(
	CopyInstance_t *from,
	boolean_t save)
{
	static CopyInstance_t saveContext;

	if (save) {
		memcpy(&saveContext, Context, sizeof (saveContext));

		Trace(TR_MISC, "Switch context (0x%x) "
		    "curr: '%s' 0x%x to: '%s' 0x%x",
		    (int)Context, sam_mediatoa(Context->media), Context->flags,
		    sam_mediatoa(from->media), from->flags);

		Context->media = from->media;
		Context->num_buffers = from->num_buffers;
		Context->flags = from->flags;
		Context->eq = from->eq;
	} else {

		/*
		 * Restore from saved context.
		 */

		Trace(TR_MISC, "Restore context (0x%x) "
		    "curr: '%s' 0x%x to: '%s' 0x%x",
		    (int)Context, sam_mediatoa(Context->media), Context->flags,
		    sam_mediatoa(saveContext.media), saveContext.flags);

		Context->media = saveContext.media;
		Context->num_buffers = saveContext.num_buffers;
		Context->flags = saveContext.flags;
		Context->eq = saveContext.eq;
	}

	Control->flags = 0;
	if (is_disk(Context->media)) {
		if (is_stk5800(Context->media)) {
			Control->flags |= CC_honeycomb;
		} else {
			Control->flags |= CC_disk;
		}
	}
}
