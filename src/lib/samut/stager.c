/*
 * stager.c - Stager communication functions.
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

#pragma ident "$Revision: 1.20 $"

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
#include "aml/stager.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/udscom.h"

/* Private data. */
static char ClientName[16] = "libsamut";
static struct UdsClient clnt = { STAGER_SERVER_NAME, ClientName,
	STAGER_SERVER_MAGIC, NULL };

/*
 * Send filesystem mount message to stager.
 */
int
_StagerFsMount(
	const char *SrcFile,
	const int SrcLine,
	char *fsname)
{
	struct StagerFsname arg;
	struct StagerGeneralRsp rsp;
	int status;

	strncpy(ClientName, program_name, sizeof (ClientName)-1);
	strncpy(arg.fsname, fsname, sizeof (arg.fsname)-1);

	status = UdsSendMsg(SrcFile, SrcLine, &clnt, SSR_fs_mount,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));

	if (status == -1) {
		return (-1);
	}
	if (rsp.status == -1) {
		errno = rsp.error;
		return (-1);
	}

	return (0);
}

/*
 * Send filesystem umount message to stager.
 */
int
_StagerFsUmount(
	const char *SrcFile,
	const int SrcLine,
	char *fsname)
{
	struct StagerFsname arg;
	struct StagerGeneralRsp rsp;
	int status;

	strncpy(ClientName, program_name, sizeof (ClientName)-1);
	strncpy(arg.fsname, fsname, sizeof (arg.fsname)-1);

	status = UdsSendMsg(SrcFile, SrcLine, &clnt, SSR_fs_umount,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));

	if (status == -1) {
		return (-1);
	}
	if (rsp.status == -1) {
		errno = rsp.error;
		return (-1);
	}

	return (0);
}

/*
 * Stop removable media activity.  Sent from sam-amld when
 * a 'samd stop' is issued.
 */
int
_StagerStopRm(
	const char *SrcFile,
	const int SrcLine)
{
	struct StagerGeneralRsp rsp;
	int status;

	strncpy(ClientName, program_name, sizeof (ClientName)-1);

	status = UdsSendMsg(SrcFile, SrcLine, &clnt, SSR_stop_rm,
	    NULL, 0, &rsp, sizeof (rsp));

	if (status == -1) {
		return (-1);
	}
	if (rsp.status == -1) {
		errno = rsp.error;
		return (-1);
	}

	return (0);
}

/*
 * Send staging event for log file message to stager.
 */
int
_StagerLogEvent(
	const char *SrcFile,
	const int SrcLine,
	int type,
	int id)
{
	struct StagerLogEvent arg;
	struct StagerGeneralRsp rsp;
	int status;

	strncpy(ClientName, program_name, sizeof (ClientName)-1);
	arg.type = type;
	arg.id = id;

	status = UdsSendMsg(SrcFile, SrcLine, &clnt, SSR_log_event,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));

	if (status == -1) {
		return (-1);
	}
	if (rsp.status == -1) {
		errno = rsp.error;
		return (-1);
	}

	return (0);
}

/*
 * Stager control.
 * Set stager control 'ident' to 'value'.
 * 'ident' is a '.' separated sequence of names of stager data.
 * 'msg' is a buffer to which messages are returned.
 * If 'msg' is NULL, a static buffer is used.
 */
char *
_StagerControl(
	const char *SrcFile,
	const int SrcLine,
	char *ident,
	char *value,
	char *msg,
	int msgsize)
{
	struct StagerControlRsp rsp;
	struct StagerControl arg;
	int status;

	if (ident == NULL || *ident == '\0') {
		strncpy(rsp.msg, "NULL ident", sizeof (rsp.msg)-1);
	} else {
		strncpy(ClientName, program_name, sizeof (ClientName) - 1);
		strncpy(arg.ident, ident, sizeof (arg.ident) - 1);
		if (value != NULL) {
			strncpy(arg.value, value, sizeof (arg.value) - 1);
		} else {
			*arg.value = '\0';
		}
		status = UdsSendMsg(SrcFile, SrcLine, &clnt, SSR_control,
		    &arg, sizeof (arg), &rsp, sizeof (rsp));
		if (status == -1) {
			(void) StrFromErrno(errno, rsp.msg, sizeof (rsp.msg));
		}
	}
	if (msg != NULL) {
		if (msgsize > 0) {
			strncpy(msg, rsp.msg, msgsize - 1);
		} else {
			*msg = '\0';
		}
	} else {
		msg = rsp.msg;
	}
	return (msg);
}
