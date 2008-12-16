/*
 * schedule.h
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

#ifndef SCHEDULE_H
#define	SCHEDULE_H

#pragma ident "$Revision: 1.26 $"

/*
 * Schedule request.
 * Structure representing a singly-linked list of stage requests
 * for the scheduler thread.
 */
typedef struct SchedRequest {
	int			id;	/* stage file to process */
	struct SchedRequest	*next;	/* next request in list */
} SchedRequest_t;

/*
 * Scheduler communication.
 * Structure representing communication to scheduling thread.
 */
typedef struct SchedComm {
	pthread_mutex_t		mutex;		/* protect access to data */
	pthread_cond_t		avail;		/* data available */
	int			data_ready;	/* data present */
	SchedRequest_t		*first;
	SchedRequest_t		*last;
	pthread_t		tid;		/* thread id */
} SchedComm_t;

/*
 * Load exported request.
 * Structure representing arguments for thread which loads
 * export media.
 */
typedef struct LoadExported {
	char 		*mountPoint;
	StreamInfo_t	*stream;
} LoadExported_t;

/* Functions */
int SendToScheduler(int id);
void CheckCopyProcs();
void *Scheduler(void *arg);
void AddWork(StreamInfo_t *stream);
void CancelWork(int id);
void ShutdownCopy(int stopSignal);
void SendSig2Copy(int signum);
void RemoveCopyProcMapFile();
void CopyProcExit(int sig);
void ClearScheduler(media_t media, char *volume);
void CancelWork(int id);
void ShutdownWork();
void TraceWorkQueue(int flag, char *srcFile, int srcLine);
void KillCopyProc(pid_t pid, CopyInstanceInfo_t *cc);

#endif /* SCHEDULE_H */
