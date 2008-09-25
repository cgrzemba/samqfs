/*
 * rft_defs.h - rft client library definitions
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

#ifndef RFT_DEFS_H
#define	RFT_DEFS_H

#pragma ident "$Revision: 1.14 $"

#include <assert.h>

#include "sam/sam_trace.h"
#undef ASSERT

#if defined(DEBUG)
#define	ASSERT(f) assert(f);
#define	ASSERT_NOT_REACHED(f) assert(0);
#else
#define	ASSERT(f) {}
#define	ASSERT_NOT_REACHED(f) {}
#endif

/*
 *	SamrftImpl information flags.
 */

/*
 * Define prototypes in command.c
 */
void SendCommand(SamrftImpl_t *rftd, const char *fmt, ...);
int GetReply(SamrftImpl_t *rftd);
int GetConnectReply(SamrftImpl_t *rftd, int *error);
int GetDPortReply(SamrftImpl_t *rftd, int *error);
int GetOpenReply(SamrftImpl_t *rftd, int *error);
int GetRecvReply(SamrftImpl_t *rftd, int *error);
int GetSendReply(SamrftImpl_t *rftd, int *error);
int GetSeekReply(SamrftImpl_t *rftd, off64_t *offset, int *error);
int GetStorReply(SamrftImpl_t *rftd, int *error);
int GetConfigReply(SamrftImpl_t *rftd, int *stripwidth, int *stripsize,
	int *tcpwindowsize);
int GetIsMountedReply(SamrftImpl_t *rftd);
int GetStatReply(SamrftImpl_t *rftd, SamrftStatInfo_t *buf, int *error);
int GetStatvfsReply(SamrftImpl_t *rftd, struct statvfs64 *buf, int *error);
int GetSpaceUsedReply(SamrftImpl_t *rftd, fsize_t *used, int *error);
int GetOpendirReply(SamrftImpl_t *rftd, int *dirp, int *error);
int GetReaddirReply(SamrftImpl_t *rftd, SamrftReaddirInfo_t *dir_info,
	int *error);
int GetMkdirReply(SamrftImpl_t *rftd, int *error);
int GetArchiveOpReply(SamrftImpl_t *rftd, int *error);

int GetLoadVolReply(SamrftImpl_t *rftd, int *error);
int GetGetRminfo(SamrftImpl_t *rftd, struct sam_rminfo *getrm, int *error);
int GetSeekVolReply(SamrftImpl_t *rftd, int *error);
int GetUnloadVolReply(SamrftImpl_t *rftd, uint64_t *position, int *error);
int GetVolInfoReply(SamrftImpl_t *rftd, struct sam_rminfo *getrm, int *eq,
	int *error);

/*
 * Define prototypes in crew.c
 */
int CreateCrew(SamrftImpl_t *rftd, int stripwidth, int stripsize,
				int tcpwindowsize);
size_t SendData(SamrftImpl_t *rftd, char *buf, size_t nbytes);
size_t ReceiveData(SamrftImpl_t *rftd, char *buf, size_t nbytes);
void CleanupCrew(SamrftCrew_t *crew);

/*
 * Define prototypes in worker.c
 */
void* Worker(void *arg);
void InitWorker(SamrftWorker_t *worker);
void CleanupWorker(SamrftWorker_t *worker);

#endif /* RFT_DEFS_H */
