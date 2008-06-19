/*
 * dkarchive.c - functions specific to staging from disk archive files
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

#pragma ident "$Revision: 1.32 $"

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
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "pub/rminfo.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/diskvols.h"
#include "sam/resource.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "aml/stager.h"
#include "aml/id_to_path.h"
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

/* Public data */
extern CopyInstanceInfo_t *Instance;
extern IoThreadInfo_t *IoThread;
extern StreamInfo_t *Stream;
extern StagerStateInfo_t *State;

/* Private data */
static upath_t fullpath;
static upath_t lastdv;
static boolean_t lastdvavail = B_FALSE;

static int initRemoteStage();

static DiskVolumeInfo_t *diskVolume = NULL;
static DiskVolumeSeqnum_t seqnum;
static boolean_t diskArchiveOpen;

/*
 * Init stage from disk file.
 */
void
DkInit(void)
{
	DiskVolsDictionary_t *diskvols;
	struct DiskVolumeInfo *dv;
	size_t size;

	diskvols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT,
	    DISKVOLS_RDONLY);
	if (diskvols != NULL) {
		(void) diskvols->Get(diskvols, Stream->vsn, &dv);
		if (dv != NULL) {
			size = STRUCT_RND(sizeof (struct DiskVolumeInfo) +
			    dv->DvPathLen);
			SamMalloc(diskVolume,  size);
			memset(diskVolume, 0,  size);
			memcpy(diskVolume, dv, size);
		} else {
			FatalSyscallError(EXIT_FATAL, HERE, "DiskVols", "get");
		}
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	} else {
		FatalSyscallError(EXIT_NORESTART, HERE, "DiskVols", "open");
	}
	ASSERT(diskVolume != NULL);
}

/*
 * Open disk archive file.
 */
int
DkLoadVolume(void)
{
	Trace(TR_MISC, "Load dk volume: '%s'", Stream->vsn);

	diskArchiveOpen = B_FALSE;
	IoThread->io_rftHandle = NULL;

	return (initRemoteStage());
}

/*
 * Open next disk archive file.
 */
int
DkNextArchiveFile(void)
{
	static upath_t tarFileName;
	FileInfo_t *file;
	int copy;
	int retry;
	int error = 0;
	boolean_t oprmsg = B_TRUE;

	file = IoThread->io_file;
	copy = file->copy;

	if (diskArchiveOpen == B_FALSE ||
	    seqnum != file->ar[copy].section.position) {

		if (diskArchiveOpen) {
			(void) SamrftClose(IoThread->io_rftHandle);
			diskArchiveOpen = B_FALSE;
		}

		/*
		 * Generate tar file name.
		 */
		seqnum = file->ar[copy].section.position;
		(void) DiskVolsGenFileName(seqnum, tarFileName,
		    sizeof (tarFileName));

		snprintf(fullpath, sizeof (fullpath), "%s/%s",
		    diskVolume->DvPath, tarFileName);

		Trace(TR_MISC, "Open file '%s' (0x%llx)", fullpath, seqnum);

		if (lastdvavail == B_FALSE &&
		    strncmp(fullpath, lastdv, sizeof (fullpath)) == 0) {
			retry = 1;		/* No retry */
			oprmsg = B_FALSE;
		} else {
			retry = file->retry;
		}
		error = -1;

		while (error == -1 && retry-- > 0) {
			int rc;

			rc = SamrftOpen(IoThread->io_rftHandle, fullpath,
			    O_RDONLY | O_LARGEFILE, NULL);
			if (rc == 0) {
				error = 0;
			} else {
				Trace(TR_ERR, "Unable to open file: %s "
				    "errno: %d", fullpath, errno);
				if (retry > 0) {
					sleep(5);
				}
			}
		}

		/*
		 * A new disk archive file.  Invalidate buffers and set
		 * current position to beginning of file.
		 */
		if (error == 0) {
			ResetBuffers();
			diskArchiveOpen = B_TRUE;
			IoThread->io_position = 0;
			lastdvavail = B_TRUE;
		} else {
			char errbuf[132];

			if (diskVolume->DvHost != NULL &&
			    diskVolume->DvHost[0] != '\0') {
				snprintf(errbuf, sizeof (errbuf), "%s:%s",
				    diskVolume->DvHost, fullpath);
			} else {
				strncpy(errbuf, fullpath, sizeof (errbuf));
			}
			lastdvavail = B_FALSE;
		}
		strncpy(lastdv, fullpath, sizeof (fullpath));
	}

	if (error != 0) {
		char errbuf[132];

		if (oprmsg == B_TRUE) {
			PostOprMsg(State->errmsg, 19208, fullpath);
			sleep(5);
			ClearOprMsg(State->errmsg);
		}

		if (diskVolume->DvHost != NULL &&
		    diskVolume->DvHost[0] != '\0') {
			snprintf(errbuf, sizeof (errbuf), "%s:%s",
			    diskVolume->DvHost, fullpath);
		} else {
			strncpy(errbuf, fullpath, sizeof (errbuf));
		}
		WarnSyscallError(HERE, "open", errbuf);
	}
	return (error);
}

/*
 * End of disk archive file.
 */
void
DkEndArchiveFile(void)
{
}

/*
 * Seek to position on disk archive file.
 */
int
DkSeekVolume(
	int to_pos)
{
	int rval;
	off64_t setPosition;
	off64_t offset;

	setPosition = to_pos * IoThread->io_blockSize;
	Trace(TR_DEBUG, "Seek to: %d (%lld)", to_pos, setPosition);

	rval = SamrftSeek(IoThread->io_rftHandle, setPosition,
	    SEEK_SET, &offset);

	if (rval < 0) {
		FatalSyscallError(EXIT_NORESTART, HERE,
		    "SamrftSeek", "SEEK_SET");
	}

	return (to_pos);
}

/*
 * Get position of disk archive file.
 */
u_longlong_t
DkGetPosition(void)
{
	int rval;
	u_longlong_t getPosition;
	off64_t offset;

	rval = SamrftSeek(IoThread->io_rftHandle, 0, SEEK_SET, &offset);
	if (rval < 0) {
		FatalSyscallError(EXIT_NORESTART, HERE,
		    "SamrftSeek", "SEEK_SET");
	}
	getPosition = offset/IoThread->io_blockSize;

	return (getPosition);
}


/*
 * Close disk archive file.
 */
void
DkUnloadVolume(void)
{
	Trace(TR_MISC, "Unload dk volume: '%s'", Stream->vsn);

	if (diskArchiveOpen) {
		(void) SamrftClose(IoThread->io_rftHandle);
		diskArchiveOpen = B_FALSE;
	}

	SamrftDisconnect(IoThread->io_rftHandle);
	if (diskVolume != NULL) {
		SamFree(diskVolume);
		diskVolume = NULL;
	}
}

/*
 * Establish connection to remote host.  If connection fails,
 * the copy process will exit.
 */
static int
initRemoteStage(void)
{
	char *hostname;
	extern StagerStateInfo_t *State;

	/*
	 * Establish connection to remote host.
	 */
	hostname = DiskVolsGetHostname(diskVolume);
	IoThread->io_rftHandle = (void *) SamrftConnect(hostname);

	if (IoThread->io_rftHandle == NULL) {
		if (hostname == NULL) {
			hostname = "";
		}
		if (State != NULL) {
			PostOprMsg(State->errmsg, 19208, hostname);
			sleep(5);
			ClearOprMsg(State->errmsg);
		}
		LibFatal(SamrftConnect, hostname);
	}

	if (DiskVolsIsAvail(NULL, diskVolume, B_FALSE, DVA_stager) != B_TRUE) {
		return (ENODEV);
	}

	return (0);
}
