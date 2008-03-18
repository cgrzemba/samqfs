/*
 * archiver.c - Archiver communication functions.
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

#pragma ident "$Revision: 1.38 $"

/* Feature test switches. */

/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/mman.h>

/* SAM-FS headers. */
#define	ARCHIVER_PRIVATE
#define	ARFIND_PRIVATE
#include "aml/archiver.h"
#undef	ARCHIVER_PRIVATE
#include "aml/archset.h"
#include "aml/archreq.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/udscom.h"

/* Private data. */
static char ClientName[16] = "libsamut";
static struct UdsClient clnt = { SERVER_NAME, ClientName, SERVER_MAGIC,
	NULL };

/* Private functions. */
static char *sendControl(const char *SrcFile, const int SrcLine, char *fsname,
	char *ident, char *value, char *msg, int msgSize);
static int sendToArchiver(const char *SrcFile, const int SrcLine, int req,
	void *arg, int argSize);
static int sendToArfind(const char *SrcFile, const int SrcLine, int req,
	char *fsname, void *arg, int argSize);



/*
 * Catalog change.
 */
int
_ArchiverCatalogChange(
	const char *SrcFile,
	const int SrcLine)
{
	return (sendToArchiver(SrcFile, SrcLine, ASR_catalog_change, NULL, 0));
}


/*
 * Archiver control.
 * Set archiver control 'ident' to 'value'.
 * 'ident' is a '.' separated sequence of names of archiver data.
 * 'msg' is a buffer to which messages are returned.
 * If 'msg' is NULL, a static buffer is used.
 */
char *
_ArchiverControl(
	const char *SrcFile,
	const int SrcLine,
	char *ident,
	char *value,
	char *msg,
	int msgSize)
{
	return (sendControl(SrcFile, SrcLine, NULL, ident, value, msg,
	    msgSize));
}


/*
 * Dequeue an ArchReq.
 */
int
_ArchiverDequeueArchReq(
	const char *SrcFile,
	const int SrcLine,
	char *fsname,			/* Name of filesystem */
	char *arname)			/* Name of ArchReq */
{
	struct AsrArchReq arg;

	strncpy(arg.ArFsname, fsname, sizeof (arg.ArFsname)-1);
	strncpy(arg.ArArname, arname, sizeof (arg.ArArname)-1);
	return (sendToArchiver(SrcFile, SrcLine, ASR_dequeueArchreq,
	    &arg, sizeof (arg)));
}


/*
 * Filesystem mounted.
 */
int
_ArchiverFsMount(
	const char *SrcFile,
	const int SrcLine,
	char *fsname)			/* Filesystem name */
{
	struct AsrFsname arg;

	strncpy(arg.ArFsname, fsname, sizeof (arg.ArFsname)-1);
	return (sendToArchiver(SrcFile, SrcLine, ASR_fs_mount, &arg,
	    sizeof (arg)));
}


/*
 * Filesystem unmount requested.
 */
int
_ArchiverFsUmount(
	const char *SrcFile,
	const int SrcLine,
	char *fsname)			/* Filesystem name */
{
	struct AsrFsname arg;

	strncpy(arg.ArFsname, fsname, sizeof (arg.ArFsname)-1);
	return (sendToArchiver(SrcFile, SrcLine, ASR_fs_umount, &arg,
	    sizeof (arg)));
}


/*
 * Queue an ArchReq.
 */
int
_ArchiverQueueArchReq(
	const char *SrcFile,
	const int SrcLine,
	char *fsname,			/* Name of filesystem */
	char *arname)			/* Name of ArchReq */
{
	struct AsrArchReq arg;

	strncpy(arg.ArFsname, fsname, sizeof (arg.ArFsname)-1);
	strncpy(arg.ArArname, arname, sizeof (arg.ArArname)-1);
	return (sendToArchiver(SrcFile, SrcLine, ASR_queueArchreq,
	    &arg, sizeof (arg)));
}


/*
 * Request a volume for an arcopy.
 */
int
_ArchiverRequestVolume(
	const char *SrcFile,
	const int SrcLine,
	char *ariname,			/* ArchReq instance name */
	fsize_t fileSize)
{
	struct AsrRequestVolume arg;

	strncpy(arg.AsAriname, ariname, sizeof (arg.AsAriname)-1);
	arg.AsFileSize = fileSize;
	return (sendToArchiver(SrcFile, SrcLine, ASR_request_volume,
	    &arg, sizeof (arg)));
}


/*
 * Stop removable media activity.
 */
int
_ArchiverRmState(
	const char *SrcFile,
	const int SrcLine,
	int state)
{
	struct AsrRmState arg;

	arg.state = state;
	return (sendToArchiver(SrcFile, SrcLine, ASR_rm_state, &arg,
	    sizeof (arg)));
}


/*
 * Request volume status.
 */
int
_ArchiverVolStatus(
	const char *SrcFile,
	const int SrcLine,
	char *mtype,
	char *vsn)
{
	struct AsrVolStatus arg;

	strncpy(arg.mtype, mtype, sizeof (arg.mtype));
	strncpy(arg.vsn, vsn, sizeof (arg.vsn));
	return (sendToArchiver(SrcFile, SrcLine, ASR_vol_status, &arg,
	    sizeof (arg)));
}


/*
 * Attach an archive queue file.
 * Returns NULL if fails.
 */
struct ArchReq *
ArchReqAttach(
	char *fsname,	/* File system name */
	char *arname,	/* ArchReq name */
	int mode)	/* O_RDONLY = read only, read/write otherwise */
{
	static char name[sizeof (ARCHIVER_DIR) + sizeof (upath_t) +
	    sizeof (ARCHREQ_DIR) + sizeof (set_name_t) + 10];
	struct ArchReq *ar;

	snprintf(name, sizeof (name), ARCHIVER_DIR"/%s/"ARCHREQ_DIR"/%s",
	    fsname, arname);
	ar = MapFileAttach(name, ARCHREQ_MAGIC, mode);
	if (ar != NULL && ar->ArVersion != ARCHREQ_VERSION) {
		(void) MapFileDetach(ar);
		errno = EINVAL;
		ar = NULL;
	}
	return (ar);
}


/*
 * Detach an archive queue file.
 * Returns -1 if fails.
 */
int
ArchReqDetach(
	struct ArchReq *ar)
{
	int rval;

	rval = 0;
	if (ar != NULL) {
		rval = MapFileDetach(ar);
	}
	return (rval);
}


/*
 * Attach an arfind state file.
 * Returns NULL if fails.
 */
struct ArfindState *
ArfindAttach(
	char *fsname,	/* Filesystem name. */
	int mode)	/* O_RDONLY = read only, read/write otherwise */
{
	static char name[sizeof (ARCHIVER_DIR"/"ARFIND_STATE) +
	    sizeof (uname_t) ];
	struct ArfindState *af;

	snprintf(name, sizeof (name), ARCHIVER_DIR"/%s/"ARFIND_STATE, fsname);
	af = MapFileAttach(name, AF_MAGIC, mode);
	return (af);
}


/*
 * ArchReq done.
 */
int
_ArfindArchreqDone(
	const char *SrcFile,
	const int SrcLine,
	char *fsname,			/* File system name */
	char *arname)			/* Archreq name */
{
	struct AfrPath arg;

	strncpy(arg.AfPath, arname, sizeof (arg.AfPath)-1);
	return (sendToArfind(SrcFile, SrcLine, AFR_archreqDone, fsname,
	    &arg, sizeof (arg)));
}


/*
 * Change state.
 */
int
_ArfindChangeState(
	const char *SrcFile,
	const int SrcLine,
	char *fsname,		/* File system name */
	int state)		/* New state */
{
	struct AfrState arg;

	arg.AfState = state;
	return (sendToArfind(SrcFile, SrcLine, AFR_changeState, fsname,
	    &arg, sizeof (arg)));
}


/*
 * Arfind control.
 * Set arfind control 'ident' to 'value'.
 * 'ident' is a '.' separated sequence of names of archiver data.
 * 'msg' is a buffer to which messages are returned.
 * If 'msg' is NULL, a static buffer is used.
 */
char *
_ArfindControl(
	const char *SrcFile,
	const int SrcLine,
	char *fsname,
	char *ident,
	char *value,
	char *msg,
	int msgSize)
{
	return (sendControl(SrcFile, SrcLine, fsname, ident, value, msg,
	    msgSize));
}


/* Private functions. */


/*
 * Send control.
 */
static char *
sendControl(
	const char *SrcFile,
	const int SrcLine,
	char *fsname,
	char *ident,
	char *value,
	char *msg,
	int msgSize)
{
	upath_t srvrName;
	struct UdsClient clnt = { SERVER_NAME, ClientName, SERVER_MAGIC, NULL };
	static struct AsrControlRsp rsp;
	struct AsrControl arg;
	int status;

	if (ident == NULL || *ident == '\0') {
		strncpy(rsp.CtMsg, "NULL ident", sizeof (rsp.CtMsg)-1);
		goto out;
	}
	if (value != NULL) {
		strncpy(arg.CtValue, value, sizeof (arg.CtValue)-1);
	} else {
		/*
		 * Make a NULL value empty.
		 */
		*arg.CtValue = '\0';
	}

	/*
	 * Select server.
	 */
	if (fsname == NULL) {
		clnt.UcServerName = SERVER_NAME;
		clnt.UcMagic = SERVER_MAGIC;
	} else {
		snprintf(srvrName, sizeof (srvrName), AFSERVER_NAME".%s",
		    fsname);
		clnt.UcServerName = srvrName;
		clnt.UcMagic = AFSERVER_MAGIC;
	}

	strncpy(ClientName, program_name, sizeof (ClientName)-1);
	strncpy(arg.CtIdent, ident, sizeof (arg.CtIdent)-1);
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, ASR_control,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		(void) StrFromErrno(errno, rsp.CtMsg, sizeof (rsp.CtMsg));
	}

out:
	if (msg != NULL) {
		if (msgSize > 0) {
			strncpy(msg, rsp.CtMsg, msgSize - 1);
		} else {
			*msg = '\0';
		}
	} else {
		msg = rsp.CtMsg;
	}
	return (msg);
}


/*
 * Send a request to archiver.
 */
static int
sendToArchiver(
	const char *SrcFile,
	const int SrcLine,
	int req,		/* Request */
	void *arg,		/* Request argument */
	int argSize)
{
	struct AsrGeneralRsp rsp;
	int		status;

	strncpy(ClientName, program_name, sizeof (ClientName)-1);
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, req, arg, argSize,
	    &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		errno = rsp.GrErrno;
		return (-1);
	}
	return (rsp.GrStatus);
}


/*
 * Send a request to arfind.
 */
static int
sendToArfind(
	const char *SrcFile,
	const int SrcLine,
	int req,		/* Request */
	char *fsname,		/* File system name */
	void *arg,		/* Request argument */
	int argSize)
{
	static struct UdsClient clnt = { AFSERVER_NAME, ClientName,
					AFSERVER_MAGIC, NULL };
	upath_t srvrName;
	struct AfrGeneralRsp rsp;
	int status;

	snprintf(srvrName, sizeof (srvrName), AFSERVER_NAME".%s", fsname);
	clnt.UcServerName = srvrName;
	strncpy(ClientName, program_name, sizeof (ClientName)-1);
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, req, arg, argSize,
	    &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		errno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}
