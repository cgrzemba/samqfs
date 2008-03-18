/*
 * dkarchive.c - functions specific to staging from disk archive files
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

#pragma ident "$Revision: 1.30 $"

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

/* Local headers. */
#include "stager_config.h"
#include "stager_lib.h"
#include "stage_reqs.h"
#include "rmedia.h"
#include "stream.h"
#include "copy_defs.h"
#include "filesys.h"
#include "copy.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Public data */
extern CopyInstance_t *Context;
extern CopyControl_t *Control;
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
InitDkStage(void)
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
LoadDkVolume(void)
{
	Trace(TR_MISC, "Load dk volume: '%s'", Stream->vsn);

	diskArchiveOpen = B_FALSE;
	Control->rft = NULL;

	return (initRemoteStage());
}

/*
 * Open next disk archive file.
 */
int
NextDkArchiveFile(void)
{
	static upath_t tarFileName;
	FileInfo_t *file;
	int copy;
	int retry;
	int error = 0;
	boolean_t oprmsg = B_TRUE;

	file = Control->file;
	copy = file->copy;

	if (diskArchiveOpen == B_FALSE ||
	    seqnum != file->ar[copy].section.position) {

		if (diskArchiveOpen) {
			(void) SamrftClose(Control->rft);
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

		Trace(TR_DEBUG, "Open file '%s' (0x%llx)", fullpath, seqnum);

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

			rc = SamrftOpen(Control->rft, fullpath,
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
			InvalidateBuffers();
			diskArchiveOpen = B_TRUE;
			Control->currentPos = 0;
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
EndDkArchiveFile(void)
{
}

/*
 * Seek to position on disk archive file.
 */
int
SeekDkVolume(
	int to_pos)
{
	off64_t set_position;
	off64_t offset;

	set_position = to_pos * Control->mau;
	Trace(TR_DEBUG, "Seek to %lld", set_position);

	if (SamrftSeek(Control->rft, set_position, SEEK_SET, &offset) < 0) {
		FatalSyscallError(EXIT_NORESTART, HERE,
		    "SamrftSeek", "SEEK_SET");
	}
	return (to_pos);
}

/*
 * Get position of disk archive file.
 */
u_longlong_t
GetDkPosition(void)
{
	u_longlong_t position;
	off64_t offset;

	if (SamrftSeek(Control->rft, 0, SEEK_SET, &offset) < 0) {
		FatalSyscallError(EXIT_NORESTART, HERE,
		    "SamrftSeek", "SEEK_SET");
	}
	position = offset/Control->mau;

	return (position);
}


/*
 * Close disk archive file.
 */
void
UnloadDkVolume(void)
{
	Trace(TR_MISC, "Unload dk volume: '%s'", Stream->vsn);
	if (diskArchiveOpen) {
		(void) SamrftClose(Control->rft);
		diskArchiveOpen = B_FALSE;
	}
	SamrftDisconnect(Control->rft);
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
	char *host_name;
	extern StagerStateInfo_t *State;
	char *vsn = NULL;

	/*
	 * Establish connection to remote host.
	 */
	host_name = DiskVolsGetHostname(diskVolume);
	Control->rft = (void *) SamrftConnect(host_name);

	if (Control->rft == NULL) {
		if (host_name == NULL) {
			host_name = "";
		}
		if (State != NULL) {
			PostOprMsg(State->errmsg, 19208, host_name);
			sleep(5);
			ClearOprMsg(State->errmsg);
		}
		LibFatal(SamrftConnect, host_name);
	}
	if (DiskVolsIsAvail(vsn, diskVolume, B_FALSE, DVA_stager) != B_TRUE) {
		return (ENODEV);
	}
	return (0);
}
