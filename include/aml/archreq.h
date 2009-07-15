/*
 * archreq.h - Archive Requests definitions.
 * Archive Requests contain the information about a batch of files to be
 * archived together.
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

#ifndef _AML_ARCHREQ_H
#define	_AML_ARCHREQ_H

#pragma	ident	"$Revision: 1.47 $"

/* Macros. */
#define	ARCHREQ_MAGIC 0122031022	/* ArchReq file magic number */
#define	ARCHREQ_VERSION 70216		/* ArchReq file version (YMMDD) */

#define	ARCHREQ_MAX_SIZE 100000000	/* Growth limit - bytes */


/* ArchReq - Archive request definitions. */

/* State of ArchReq. */
enum ArState {
	ARS_create = 1,			/* Being created - by sam-arfind */
	ARS_schedule,			/* Being scheduled - by sam-archiverd */
	ARS_archive,			/* Being archived - by sam-arcopy */
	ARS_done,			/* Archiving complete */

	/* Used internally by archive scheduler */
	ARS_cpiMore,			/* Assign more work to copy instance */
	ARS_volRequest,			/* Volume requested by sam-arcopy */
	ARS_MAX
};

/* Definition of drive division methods. */
enum DivideMethods {
	DM_none,
	DM_offline,			/* Offline files */
	DM_ownerDir,			/* Files for -owner dir */
	DM_ownerUidGid,			/* Files for -owner user | group */
	DM_segment,			/* Segmented files */
	DM_max
};

/* Archive request. */
struct ArchReq {
	MappedFile_t	Ar;
	int		ArVersion;	/* Version */
	int		ArAtIndex;	/* Index in arfind ArchReq table */
	enum ArState	ArState;	/* State */
	uint_t		ArSeqnum;	/* ArchReq sequence number */
	set_name_t	ArAsname;	/* Archive set name */
	uname_t		ArFsname;	/* File system name */
	ushort_t	ArCopy;		/* Archive copy to be made */
	ushort_t	ArStatus;	/* Status - set by sam-arfind */
	int		ArAsn;		/* Archive set number */
	int		ArCount;	/* Number of entries in ArchReq */
	int		ArDrives;	/* Maximum number of drives to use */
	int		ArDrivesUsed;	/* Actual number of drives used */
	int		ArFileIndex;	/* Offset to the start of file */
					/* index array */
	size_t		ArSize;		/* Size of ArchReq */
	fsize_t		ArSpace;	/* Space required to archive */
					/* all files */
	time_t		ArTime;		/* Time ArchReq created */
	time_t		ArTimeQueued;	/* Time ArchReq queued */

	/* Scheduler data. */
	ushort_t	ArDivides;	/* Drive division method */
	ushort_t	ArFlags;
	int		ArFiles;	/* Number of files to be archived */
					/* I.e. number of entries in file */
					/* index array */
	fsize_t		ArMinSpace;	/* Space required to archive */
					/* smallest file */
	uint_t		ArMsgSent;	/* Message sent flags */
	int		ArStageVols;	/* Number of volumes used for */
					/* staging files */
	Priority_t	ArPriority;	/* ArchReq priority */
	Priority_t	ArSchedPriority; /* Scheduling priority */
	int		ArSelFiles;	/* Number files selected to archive */
	fsize_t		ArSelSpace;	/* Space required to archive */
					/* selected files */
	/* Archiving data. */
	struct ArcopyInstance {
		fsize_t	CiBytesWritten;	/* Number of bytes written to archive */
		fsize_t	CiMinSpace;	/* Space required to archive */
					/* smallest file */
		fsize_t	CiSpace;	/* Space needed to archive all files */
		pid_t	CiPid;		/* sam-arcopy process id */
		int	CiArchives;	/* Archive files (tarballs) written */
		int	CiArcopyNum;	/* arcopy number ('arcopyxx') */
		int	CiFiles;	/* Number of files */
		int	CiFilesWritten;	/* Number of files written to archive */
		ushort_t CiFlags;
		OwnerName_t CiOwner;	/* Reserve VSN owner name */
		char	CiOprmsg[OPRMSG_SIZE];
		int	CiAln;		/* Archive Library number */
		int	CiLibDev;	/* Library device entry offset */
		int	CiRmFn;		/* Removable media file number */
		int	CiSlot;		/* Slot number in library */
		mtype_t	CiMtype;	/* Media type of volume being written */
		vsn_t	CiVsn;		/* VSN of volume being written */
		fsize_t CiVolSpace;	/* Space available on volume */
		DiskVolumeSeqnum_t CiSeqNum; /* Arcopy's sequence number */
	} ArCpi[1];			/* Arcopy instance ([n] = ArDrives) */

	/* Dynamic array of FileInfo entries */
	/* struct FileInfo ArFileInfo[1]; */
};

/* ArStatus - set by sam-arfind */
#define	ARF_changed	0x0001		/* ArchReq has been changed */
#define	ARF_full	0x0002		/* ArchReq has reached maximum size */
#define	ARF_merge	0x0004		/* ArchReq is to be merged */
#define	ARF_rearch	0x0008		/* Rearchive files in the ArchReq */
#define	ARF_recover	0x0010		/* ArchReq recovered */
#define	ARF_rmreq	0x0020		/* Remove ArchReq */
#define	ARF_start	0x0040		/* Request has start parameters set */

/* ArFlags - set by sam-archiverd. */
#define	AR_acnors	0x0001		/* sam-arcopy norestart error */
#define	AR_disk		0x0002		/* Archive to disk */
#define	AR_first	0x0004		/* First time request is composed */
#define	AR_honeycomb	0x0008		/* Archive to honeycomb */
#define	AR_join		0x0010		/* Request has joined files */
#define	AR_nonstage	0x0020		/* Request has non-stagable files */
#define	AR_offline	0x0040		/* Request has off line files */
#define	AR_schederr	0x0080		/* Scheduler error detected */
#define	AR_segment	0x0100		/* Request has segmented files */
#define	AR_unqueue	0x0200		/* Unqueue request */

#define	AR_diskArchReq	(AR_disk | AR_honeycomb)
#define	AR_restartScan	(AR_acnors | AR_nonstage | AR_schederr)

/* Instance flags. */
#define	CI_disk		0x0001		/* Archive to disk */
#define	CI_honeycomb	0x0002		/* Archive to honeycomb */
#define	CI_idled	0x0004		/* sam-arcopy idled */
#define	CI_more		0x0008		/* More files available - */
					/* result of drivemax */
#define	CI_sim		0x0010		/* Simulate archiving */
#define	CI_volreq	0x0020		/* Volume requested */

#define	CI_diskInstance	(CI_disk | CI_honeycomb)

/* Shared fields. */

/* Used in sam-arfind - start archiving triggers. */
#define	ArStartCount ArCpi[0].CiFiles	/* Number of files in ArchReq */
#define	ArStartSize ArCpi[0].CiSpace	/* Size of all files in ArchReq  */
#define	ArStartTime ArCpi[0].CiBytesWritten	/* Time to start */


/*
 * File information.
 * All the information about a file that is to be archived.
 *
 * Each file has a FileInfo entry in the ArchReq.  The entries are entered
 * as found by sam-arfind.  When sam-arfind sends the ArchReq to sam-archiverd,
 * a file index array is constructed.  This array contains the offset of each
 * FileInfo entry in the ArchReq.  Before scheduling, the array entries may
 * be rearranged to satisfy user, join, sort, and offline file requirements.
 *
 * The FiCpi field is used to identify which sam-arcopy will archive the
 * file.  A value of -1 is used to 'save' the file for a later copy operation.
 * This can result from file organization requirements, or space restrictions
 * on a volume.
 *
 */
struct FileInfo {
	sam_id_t	FiId;		/* File inode id */
	fsize_t		FiFileSize;	/* File size */
	fsize_t		FiSpace;	/* Space required in archive file */
	time_t		FiModtime;	/* Time file was last modified */
	Priority_t	FiPriority;	/* Archive priority */
	union {				/* Owner information */
		ushort_t path_l;	/* Length of directory path part */
					/* of file name */
		uid_t	uid;
		gid_t	gid;
	} FiOwner;
	uint_t		FiSegOrd;	/* Segment ordinal */
	ushort_t	FiName_l;	/* Length of file name */
	uchar_t		FiStatus;	/* Status of file - set by sam-arfind */
	char		FiType;		/* Type of file - sfind character */
	/* The following three elements are in arch_status format */
	/* One bit for each copy */
	uchar_t		FiCopiesNorel;	/* Copies before automatic release */
					/* is allowed */
	uchar_t		FiCopiesRel;	/* Release file after copy is made */
	uchar_t		FiCopiesReq;	/* Copies required to be made */

	/* Following three are set by sam-archiverd and sam-arcopy. */
	short		FiCpi;		/* sam-arcopy instance */
	uchar_t		FiFlags;
	uchar_t		FiStageCopy;	/* Copy to stage from */

	char		FiName[1];	/* File name */
	/* Followed by a 'struct OfflineInfo' if the file is offline. */
};

/* Flags definitions. */
/* FiStatus - Set by sam-arfind */
#define	FIF_exam	0x01		/* File to be examined after archival */
#define	FIF_offline	0x02		/* Entry has off line information */
#define	FIF_rearch	0x04		/* Copy is being rearchived */
#define	FIF_remove	0x08		/* Entry removed */
#define	FIF_segment	0x10		/* Entry is a segment of the file */
#define	FIF_simread	0x20		/* Simulate archival read of the file */

/* FiFlags */
/* Set by sam-archiverd during compose */
#define	FI_first	0x01		/* First file in archive file */
#define	FI_join		0x02		/* Join file - marks start of */
					/* joined files */
#define	FI_offline	0x04		/* File is offline */
#define	FI_stagesim	0x08		/* Simulate staging */

/* Set by sam-arcopy */
#define	FI_archived	0x10		/* File was archived */
#define	FI_error	0x20		/* An error occurred */

#define	FI_IGNORE (fi->FiFlags & (FI_archived | FI_error))

/* Size of a FileInfo entry. */
/* Used to allocate entries, and find the next entry without the index. */
#define	FI_SIZE(fi) (STRUCT_RND(sizeof (struct FileInfo)) + \
	STRUCT_RND((fi)->FiName_l) + \
(((fi)->FiStatus & FIF_offline) ? STRUCT_RND(sizeof (struct OfflineInfo)) : 0))

/*
 * Offline file information.
 * This structure is placed after the file name of a FileInfo entry.
 */
struct OfflineInfo {
	fsize_t		OiPosition;
	uint32_t	OiOffset;
	media_t		OiMedia;
	vsn_t		OiVsn;
};

#define	LOC_OFFLINE_INFO(fi) ((struct OfflineInfo *)(void *) ((char *)(fi) + \
	STRUCT_RND(sizeof (struct FileInfo)) + STRUCT_RND((fi)->FiName_l)))


/*
 * Join file definitions.
 * A 'join file' entry is used to encapsulate all the FileInfo entries for
 * a join operation.
 * The FiName field contains the path which determines the join.
 * (Note that these 'shared' fields are not used for a 'join file'.
 */
#define	FiJoinCount FiId.gen	/* Count of files joined */
#define	FiJoinStart FiSegOrd	/* Start of the joined files */

/* Archive error return definitions. */
#define	FiErrno	FiOwner.uid	/* Archival error number - when FI_error set */

/* Manipulation of FileInfo entries. */
/* Pointer to first FileInfo entry. */
#define	FIRST_FILE_INFO_ADDR(ar) ((char *)(void *)&(ar)->ArCpi[(ar)->ArDrives])

/* Pointer to file index.  Used in sam-archiverd and sam-arcopy. */
#define	LOC_FILE_INDEX(ar) ((int *)(void *)((char *)(ar) + (ar)->ArFileIndex))

/* Pointer to i-th FileInfo entry.  Used in sam-archiverd and sam-arcopy. */
#define	LOC_FILE_INFO(i) ((struct FileInfo *)(void *)((char *)ar + fii[(i)]))

/* FiCpi values. */
#define	CPI_later -1		/* Process the file later */
#define	CPI_more -2		/* Process when an arcopy instance finishes */

#if defined(NEED_ARCHREQ_NAMES)
static char *StateNames[] = {
	"??", "create", "schedule", "archive", "done",
	"cpiMore", "volRequest"
};

static char *ArFlagNames[] = {
	"acnors", "disk", "first", "honeycomb",
	"join", "nonstage", "offline", "schederr",
	"segment", "??", "??", "??",
	"??", "??", "??", "??"
};
static char *CiFlagNames[] = {
	"disk", "idled", "more", "remote",
	"sim", "volreq", "??", "??"
};
static char *DivideNames[] = {
	"none", "offline", "owner", "owner", "segment", "??"
};
static char *FiFlagNames[] = {
	"first", "join", "offline", "stgsim",
	"archived", "error", "??", "??"
};

#endif /* defined(NEED_ARCHREQ_NAMES) */

#if defined(NEED_ARCHREQ_MSG_SENT)
/* Index in table is the bit number in ArMsgSent */
static int msgSentTable[] = {
	4010,	/* -wN No volumes available for Archive Set %s */
	4013,	/* -wN No space available for Archive Set %s - */
	/* File %s too large for volume overflow for remaining volumes */
	4014,	/* -wN No space available for Archive Set %s - */
	/* File %s too large for any remaining volume */
	4015,	/* -wN No space available for Archive Set %s - */
	/* Joined files too large for any remaining volume */
	4024,	/* -wNS ArchReq %s queue timeout %s */
	4349,	/* -wN Stage volume %s.%s unavailable */
	0
};
#endif /* defined(NEED_ARCHREQ_MSG_SENT) */

/* Functions. */
struct ArchReq *ArchReqAttach(char *fsname, char *arname, int mode);
int ArchReqDetach(struct ArchReq *);

#endif /* _AML_ARCHREQ_H */
