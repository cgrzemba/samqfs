/*
 * utility.h - Archiver utility function definitions.
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

#ifndef UTILITY_H
#define	UTILITY_H

#pragma ident "$Revision: 1.24 $"

/* Macros. */
#define	ISO_STR_FROM_TIME_BUF_SIZE 32

/* Functions. */
void ClearOprMsg(void);
char *CountToA(uint64_t v);
char *ExecStateToStr(ExecState_t state);
void LogLock(FILE *st);
void LogUnlock(FILE *st);
void NotifyRequest(int priority, int msgNum, char *msg);
void OpenOprMsg(char *OprMsgLoc);
void PostOprMsg(int MsgNum, ...);
char *ScanPathToMsg(char *path);
void *ShmatSamfs(int mode);
char *StrSignal(int sig);
char *TimeToIsoStr(time_t tv, char *buf);

#endif /* UTILITY_H */
