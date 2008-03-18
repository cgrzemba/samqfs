/*
 * custmsg.h - Customer message processor definitions.
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

#if !defined(CUSTMSG_H)
#define	CUSTMSG_H

#ifdef sun
#pragma ident "$Revision: 1.21 $"
#endif

#include "sam/types.h"

/* Generate customer message arguments. */
#define	CustMsg(n) n
#define	HERE _SrcFile, __LINE__
#define	LibFatal(f, a) _LibFatal(_SrcFile, __LINE__, #f, a)

/* Users can be notified of an problems via Email, SNMP or fault log */
#define	NOTIFY_AS_FAULT		0x00000001
#define	NOTIFY_AS_TRAP		0x00000002
#define	NOTIFY_AS_EMAIL		0x00000004

/* Classification of the components as required by sysevent */
#define	FSD_CLASS	"Fsd"   /* sam-fsd class name used by sysevent */
#define	FS_CLASS	"FS"	/* Filesystem Class name used by sysevent */
#define	MISC_CLASS	"Misc"	/* Misc Class name used by sysevent */
#define	DUMP_CLASS	"Dump"	/* for events related to samfsdump */
#define	ACSLS_CLASS	"Acsls"	/* for events related to acsls */
#define	ACSLS_ERROR_SUBCLASS	"Err"			/* for errors */
#define	ACSLS_INFO_SUBCLASS	"Warn"			/* for info */
#define	DUMP_WARN_SUBCLASS	"Warn"			/* for warnings */
#define	DUMP_INTERRUPTED_SUBCLASS	"Interrupted"	/* for errors */
#define	MAX_MSGBUF_SIZE	1024

/* Public functions. */
void _LibFatal(const char *SrcFile, const int SrcLine, const char *FunctionName,
	const char *arg);
void CustmsgInit(int log_mode,
	void(*Notify)(int priority, int MsgNum, char *msg));
void CustmsgTerm(void);
char *GetCustMsg(int MsgNum);
void LibError(void(*MsgFunc)(int code, char *msg), int code, int MsgNum, ...);
void Nomem(const char *FunctionName, char *ObjectName, size_t size);
void SendCustMsg(const char *SrcFile, const int SrcLine, int MsgNum, ...);
void SysError(const char *SrcFile, const int SrcLine, const char *fmt, ...);
/*
 * PostEvent
 * Post the system event to syseventd. In sysevent.conf, a handler exists for
 * ach posted event. The action to be taken (send email, send trap or persist
 * as fault) is given as input in action. The class, subclass, msgnum,
 * errortype and message describe the event
 * This has replaced the PostSysevent() api
 */
int PostEvent(char *class, char *subclass, int msgnum, int errortype,
	char *msg, uint32_t action);

/* Public data. */
extern int ErrorExitStatus;

#endif	/* CUSTMSG_H */
