/*
 * arfind.h - Arfind program definitions.
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

#ifndef ARFIND_H
#define	ARFIND_H

#pragma ident "$Revision: 1.66 $"

/* Solaris headers. */
#include <sys/param.h>

/* SAM-FS headers. */
#include "sam/fs/arfind.h"

/* Local headers. */
#include "common.h"
#include "archset.h"
#include "archreq.h"
#include "examlist.h"
#include "fileprops.h"
#include "scanlist.h"
#include "threads.h"
#include "utility.h"

/* Macros. */
#define	DIRBUF_SIZE 131072	/* Size of buffer for directory reads */
#define	EPSILON_TIME 30		/* Action within this time is close enough */
#define	PATHBUF_SIZE ((2*MAXPATHLEN)+4)	/* Size of path buffer */
#define	PATHBUF_END (PATHBUF_SIZE - 32) /* Allowance for segment number */

#define	ES_force	0x400		/* To force setting of state */

#if defined(lint)
#define	EXAM_TRACE
#define	FILE_TRACE
#define	I2P_TRACE
#define	SCAN_TRACE
#endif /* defined(lint) */

/*
 * Fields of the struct sam_disk_inode used to pass file examination arguments.
 * The fields used by these macros contain no useful metadata for the file
 * examination process.  The are used internally by arfind.
 */

/*
 * Where to put the segmented file size in a data segment inode.
 */
#define	SEGFILE_SIZE(d) (offset_t *)(void *)&((d)->extent[0])

/*
 * Which examination method or file system upcall is being used to examine
 * the inode.
 */
#define	EXAM_MODE(d) ((d)->unit)
#define	EXAM_DIR AE_none
#define	EXAM_INODE AE_MAX

/*
 * Buffer for contructiong a path name.
 * The path may be constructed from either end.
 */
struct PathBuffer {
	char	*PbPath;		/* First character of path */
	char	*PbEnd;			/* Last character + 1 of path */
	char	PbBuf[PATHBUF_SIZE];
};

/* Public data. */

DCL struct ArfindState *State IVAL(NULL);	/* arfind state */
DCL struct FileProps *FileProps;
DCL ExecState_t Exec;
DCL boolean_t Recover;

DCL FILE *LogStream IVAL(NULL);		/* Log file stream */
DCL time_t StopScan;			/* Time at which to stop scan */

DCL char *FsName;			/* Filesystem name */

DCL int *ArchSetMap IVAL(NULL);	/* Table of Archive Set number equivalences */


/* Public functions. */
void ChangeState(ExecState_t newState);
void TraceTimes();

/* archive.c */
void *Archive(void *arg);
int ArchiveAddFile(struct PathBuffer *pb, struct sam_perm_inode *pinode,
	struct FilePropsEntry *fp, int copy, int age, int copiesReq,
	boolean_t unarch);
void ArchiveInit(struct Threads *th);
void ArchiveReconfig(void);
int ArchiveRmArchReq(char *arname);
void ArchiveRmInode(sam_id_t id);
void ArchiveRun(void);
void ArchiveStartDir(char *dirPath);
void ArchiveStartRequests(void);
void ArchiveTrace(void);

/* checkfile.c */
void CheckFile(struct PathBuffer *pb, struct sam_perm_inode *pinode,
	struct ScanListEntry *se);
void SetArchdone(char *filePath, struct sam_disk_inode *dinode);

/* classify.c */
struct FilePropsEntry *ClassifyFile(char *path, struct sam_perm_inode *pinode,
	sam_time_t *accessTime);
void ClassifyInit(void);

/* control.c */
char *Control(char *ident, char *value);

/* examinodes.c */
void *ExamInodes(void *arg);
void ExamInodesAddEntry(sam_id_t id, int mode, sam_time_t xeTime, char *caller);
void ExamInodesRmInode(sam_id_t id);
void ExamInodesTrace(void);

/* fsact.c */
void FsAct(void);
void FsActInit(struct Threads *th);
void *FsExamine(void *arg);
void FsExamineInit(struct Threads *th);

/* fsstats.c */
void FsstatsCountFile(struct sam_perm_inode *pinode, char *filePath);
void FsstatsEnd(void);
void FsstatsStart(void);
void FsActTrace(void);

/* id2path.c */
void IdToPath(struct sam_disk_inode *inode, struct PathBuffer *pb);
void IdToPathId(sam_id_t id, struct PathBuffer *pb);
void IdToPathInit(void);
void IdToPathRmInode(sam_id_t id);
void IdToPathTrace(void);
int PathToId(char *path, sam_id_t *id);

/* message.c */
void *Message(void *arg);
void MessageInit(struct Threads *th);

/* scanfs.c */
void *Scanfs(void *arg);
void ScanfsAddEntry(struct ScanListEntry *se);
void ScanfsFullScan(void);
void ScanfsInit(struct Threads *th);
void ScanfsList(FILE *st);
void ScanfsReconfig(void);
void ScanfsRmInode(sam_id_t id);
void ScanfsStartScan(void);
void ScanfsTrace(void);

/* scandirs.c */
void ScanDirs(struct ScanListEntry *se);
void ScanDirsInit(void);

/* scaninodes.c */
void ScanInodes(struct ScanListEntry *se);
void CheckInode(struct PathBuffer *pb, struct sam_perm_inode *pinode,
	struct ScanListEntry *se);
void ScanInodesPauseScan(boolean_t pause);

#endif /* ARFIND_H */
