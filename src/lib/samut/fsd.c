/*
 * notify.c - send notify message.
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

#pragma ident "$Revision: 1.15 $"

/* ANSI C headers. */
#include <errno.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#define	FSD_PRIVATE
#include "aml/fsd.h"
#include "sam/lib.h"
#include "sam/udscom.h"

/* Private data. */
static char ClientName[16] = "libsamut";
static struct UdsClient clnt = { SERVER_NAME, ClientName, SERVER_MAGIC,
	NULL };


/*
 * Execute notification file.
 */
int
_FsdNotify(
	const char *SrcFile,
	const int SrcLine,
	char *fname,			/* name of file to execute */
	int priority,			/* syslog priority */
	int msg_num,			/* from message catalog */
	char *msg)			/* actual message */
{
	struct FsdNotify arg;
	struct FsdGeneralRsp rsp;
	int status;

	if (program_name != NULL && *program_name != '\0') {
		strncpy(ClientName, program_name, sizeof (ClientName)-1);
	}
	strncpy(arg.FnFname, fname, sizeof (arg.FnFname)-1);
	arg.FnPriority = priority;
	arg.FnMsgNnum = msg_num;
	strncpy(arg.FnMsg, msg, sizeof (arg.FnMsg)-1);
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, FSD_notify,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}

#if defined(TEST)

#include "sam/custmsg.h"
static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */


int
main(
	int argc,
	char *argv[])
{
	int ret_val;

	ret_val = FsdNotify("/opt/SUNWsamfs/sbin/test.sh", 3, 0,
	    "test message");
	if (ret_val == -1) {
		LibFatal(FsdNotify, "");
	}
	return (0);
}

#endif /* defined(TEST) */
