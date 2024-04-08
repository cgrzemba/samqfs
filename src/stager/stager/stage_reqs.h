/*
 * stage_reqs.h - storage and access to active stage requests
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
 * or https://illumos.org/license/CDDL.
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

#ifndef STAGE_REQS_H
#define	STAGE_REQS_H

#pragma ident "$Revision: 1.28 $"

/* SAM-FS headers. */
#include "aml/stager_defs.h"

/* Local headers. */
#include "file_defs.h"

/* Structures. */

#define	STAGER_REQ_FILE_MAGIC	05041536
#define	STAGER_REQ_FILE_VERSION	60522	/* StageReq file version (YMMDD) */

typedef struct StageReqFileVal {
	uint32_t	magic;
	uint32_t	version;
	time_t		create;
	size_t		alloc;
	size_t		size;
} StageReqFileVal_t;

/*
 * This structure contains all stage requests in progress.
 * The request list is implemented as an index file that is mapped to
 * the process's address space.
 */
typedef struct StageReqs {
	size_t		entries;	/* number of entries in use */
	size_t		alloc;		/* size of allocated space */
	int		*free;		/* free list pointer */
	int		sp;		/* stack pointer for free list */
	int		requeue;	/* requeue link pointer */

	pthread_mutex_t	free_list_mutex; /* protect access to free list */
	pthread_mutex_t	entries_mutex;	/* protect access to num of entries */
	pthread_cond_t	entries_cond;

	FileInfo_t		*data;
	StageReqFileVal_t	*val;
} StageReqs_t;

/* Functions */
int AddFile(sam_stage_request_t *req, int *status);
void CancelRequest(sam_stage_request_t *req);

FileInfo_t *CreateFile(sam_stage_request_t *req);
FileInfo_t *GetFile(int id);
void InitStageDoneList();
void SetStageDone(FileInfo_t *file);
void StageError(FileInfo_t *file, int error);
void DeleteRequest(int id);
int GetArcopy(FileInfo_t *file, int start);
int InitRequestList();

int CheckRequests(int *id);
void ErrorRequest(sam_stage_request_t *req, int error);
void SeparateMultiVolReq();
void DoneOrphanReq(pid_t pid);
void RemoveStageReqsMapFile();
void RemoveStageDoneMapFile();
void TraceStageReqs(int flag, char *srcFile, int srcLine);

/* Error codes */
#define	REQUEST_READY		0	/* not an error, request ready */
#define	REQUEST_LIST_FULL	1	/* request list is full */
#define	REQUEST_ERROR		2	/* duplicate or error in request */
#define	REQUEST_EXTENDED	3	/* multivolume extended request */

/* CheckRequests error codes */
#define	CHECK_REQUEST_SUCCESS	0
#define	CHECK_REQUEST_SCHED		1

#endif /* STAGE_REQS_H */
