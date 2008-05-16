/*
 * archiverd.h - Archiverd program definitions.
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

#if !defined(ARCHIVERD_H)
#define	ARCHIVERD_H

#pragma ident "$Revision: 1.54 $"

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#include "sam/defaults.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "sam/fs/ino.h"

/* Local headers. */
#include "common.h"
#include "archset.h"
#include "archreq.h"
#include "threads.h"
#include "utility.h"

#define	MAX_START_PROCESS_ARGS 3

/* Archiver execution control codes. */
typedef enum { EC_none,
	EC_run,			/* Start archiving */
	EC_idle,		/* Idle archiving */
	EC_stop,		/* Stop archiving */
	EC_amld_start,		/* Start removable media from sam-amld */
	EC_amld_stop,		/* Stop removable media from sam-amld */
	EC_max
} ExecControl_t;

/* Queue entry. */
/* The queue entry is used in compose.c and scheduler.c. */
struct QueueEntry {
	struct QueueEntry *QeFwd;	/* Forward link */
	struct QueueEntry *QeBak;	/* Backward link */
	struct QueueEntry *QeNext;	/* Forward link for all queues */
	struct ArchReq *QeAr;		/* Queued ArchReq */
	char	QeArname[ARCHREQ_NAME_SIZE];
};

struct Queue {
	uname_t	QuName;			/* Name of queue */
	struct QueueEntry QuHead;	/* Head of queue */
};

/* Public data. */

/* File system table */
DCL struct FileSys {
	int	count;
	struct FileSysEntry {
		uname_t	FsName;		/* Name of filesystem */
		upath_t	FsMntPoint;
		int	FsFlags;
		struct ArfindState *FsAfState;
	} entry[1];
} *FileSysTable IVAL(NULL);

#define	FS_mounted	0x01	/* File system mounted */
#define	FS_noarchive	0x02	/* noarchive (nosam) or noarscan */
#define	FS_noarfind	0x04	/* Don't run sam-arfind (noarscan) */
#define	FS_norestart	0x08	/* Do not restart arfind */
#define	FS_share_client	0x10	/* Sharefs client */
#define	FS_share_reader	0x20	/* Shared reader */
#define	FS_umount	0x40	/* Unmount requested */

/* Public functions. */
/* archiverd.c */
boolean_t ChangeState(ExecState_t newState);
void ChildTrace(void);
void FileSysMount(struct FileSysEntry *fs);
void FileSysStatus(struct FileSysEntry *fs);
void FileSysUmount(struct FileSysEntry *fs);
int StartProcess(char *argv[], void (*func)(char *argv1, int stat));

/* compose.c */
void *Compose(void *arg);
void ComposeDrives(struct ArchReq *ar, struct ArchSet *as, int drives);
void ComposeEnqueue(struct QueueEntry *qe);
void ComposeForCpi(struct ArchReq *arDrive, struct ArchSet *as, int cpi);
void ComposeInit(struct Threads *th);
void ComposeMakeTarballs(struct ArchReq *ar, int cpi, fsize_t archmax,
		fsize_t ovflmin, fsize_t volSpace);
int ComposeNonStageFiles(struct ArchReq *ar);
void ComposeSelectFit(struct ArchReq *ar, int cpi, fsize_t availSpace,
		fsize_t minSize);
void ComposeTrace(void);

/* control.c */
char *Control(char *ident, char *value);

/* messages.c */
void *Message(void *arg);
void MessageInit(struct Threads *th);
void MessageReady(void);
void MessageReturnQueueEntry(struct QueueEntry *qe);

/* queue.c */
void QueueAdd(struct QueueEntry *qe, struct Queue *qu);
void QueueFree(struct QueueEntry *qe);
struct QueueEntry *QueueGetFree(void);
boolean_t QueueHasArchReq(char *arname);
void QueueInit(struct Queue *qu);
void QueueRemove(struct QueueEntry *qe);
void QueueTrace(const char *srcFile, const int srcLine, struct Queue *qu);

/* reserve.c */
void ReadReservedVsns(void);

/* schedule.c */
void Schedule(void);
boolean_t ScheduleDequeue(char *dequeueArname);
void ScheduleEnqueue(struct QueueEntry *qe);
boolean_t ScheduleGetVolStatus(mtype_t mtype, vsn_t vsn);
void ScheduleInit(struct Threads *th);
void ScheduleRun(char *msg);
int ScheduleRequestVolume(char *ariname, fsize_t fileSize);
void ScheduleSetDkState(ExecControl_t ctrl);
void ScheduleSetFsState(struct FileSysEntry *fs, ExecControl_t ctrl);
void ScheduleSetRmState(ExecControl_t ctrl);
void ScheduleTrace(void);
#if defined(AR_DEBUG)
void SchedulerTest(void);
#endif /* defined(AR_DEBUG) */

#endif /* ARCHIVERD_H */
