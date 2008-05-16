/*
 *	arcopy.h - Arcopy program definitions.
 *
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

#ifndef ARCOPY_H
#define	ARCOPY_H

#pragma ident "$Revision: 1.48 $"

/* Local includes. */
#include "common.h"
#include "archset.h"
#include "archreq.h"
#include "threads.h"
#include "utility.h"
#include "volume.h"

#define	ALARM_INTERVAL 30	/* Interval between SIGALRMs */
#define	LISTSETARCH_MAX 3000	/* Maximum number of files per setarch list */

/* Public data. */

/* Volume information. */
struct Vols {
	int	count;
	struct VolsEntry {
		struct VolInfo Vi;
		int	VlEq;		/* Drive equipment number */
		fsize_t VlLength;	/* Section length of file on */
					/* this volume */
		uint64_t VlPosition;	/* Position of archive file for */
					/* this section */
	} entry[1];
} *VolsTable;				/* Volumes to archive to */

DCL int VolCur;				/* Current volume */
DCL upath_t ArVolName;			/* Name of current archive volume */
DCL media_t ArchiveMedia;		/* Archive media type */

/* Archived file information. */
struct ArchiveFile {
	time_t		AfCreateTime;	/* Time file created */
	uint64_t	AfPosition;	/* Position of start of archive file */
	int		AfVol;		/* Volume in VolsTable (first or */
					/* only) */
	fsize_t		AfOffset;	/* Offset of start of file from the */
					/* beginning of the archive file */
	fsize_t		AfFileSize;	/* Size of file */
	sam_timestruc_t	AfAccessTime;	/* File access time */
	sam_timestruc_t	AfModifyTime;	/* File modification time */
	ushort_t	AfFlags;	/* Archival status flags */
	csum_t		AfCsum;		/* Checksum value */
	int		AfSegNum;	/* Segment number */
	struct FileInfo *f;
} *FilesTable;

DCL struct ArchiveFile *File;		/* File being archived */

/* Flags definitions. */
#define	AF_copied	0x0001		/* File was successfully copied */
#define	AF_csummed	0x0002		/* Checksum was calculated for file */
#define	AF_error	0x0004		/* An error occurred during archival */
#define	AF_first	0x0008		/* First file in archive file */
#define	AF_notarhdr	0x0010		/* No tar header built for file */
#define	AF_offline	0x0020		/* File was offline */
#define	AF_overflow	0x0040		/* File overflowed volume */
#define	AF_release	0x0080		/* Release file after archival */
#define	AF_sparse	0x0100		/* Sparse tar file written */

#define	AF_tarheader(flags)	((flags & AF_notarhdr) == 0)

DCL int FilesNumof IVAL(0);		/* Number of entries */

DCL ExecState_t Exec IVAL(ES_init);
DCL struct ArchReq *ArchReq;
DCL struct ArchSet *ArchiveSet;
DCL struct ArcopyInstance *Instance;
DCL fsize_t	VolBytesWritten;	/* Bytes written to a volume */
DCL boolean_t DirectIo;			/* Use direct I/O for staging files */
DCL boolean_t SparseFile;		/* Sparse file being archived */
DCL boolean_t TapeNonStop;		/* Write tape non-stop */
DCL upath_t ArName;			/* ArchReq name */
DCL char *LogName;			/* Archiver log file name */
DCL int AfFd IVAL(-1);			/* Archive file file descriptor */
DCL int ArchiveCopy;			/* Copy being made */
DCL int BlockSize IVAL(0);		/* Number of bytes in media block */
DCL int FilesArchived IVAL(0);		/* Files archived */
DCL int OldArchive;			/* Old/New style archive records? */
DCL int ReadCount;			/* Number bytes for read() from file */
DCL int WriteCount;			/* Number bytes for write() to file */
DCL int WriteTimeout;			/* Media dependent write timeout */

/* Remote media archiving information. */
struct {
	boolean_t enabled;	/* set TRUE if archiving to remote host */
	char	  *host;	/* remote host on which archival media reside */
	void	  *rft;		/* remote file transfer descriptor */
} RemoteArchive;

/* Public functions. */

/* arcopy.c */
void ClearTimeout(enum Timeouts to);
void SetTimeout(enum Timeouts to);

/* checksum.c */
void ChecksumInit(uchar_t algo);
void ChecksumData(char *data, int numBytes);
void ChecksumWait(void);

/* copyfile.c */
void AdvanceIn(int count);
void CopyFile(void);
void CopyFileReconfig(void);
void EndArchiveFile(int firstFile);
void RoundBuffer(int nbytes);
char *WaitRoom(int count);
void *WriteBuffer(void *);
void WriteData(void *buf, int count);
void WriteInit(struct Threads *th);
void _IoFatal(const char *SrcFile, const int SrcLine, const char *FunctionName,
	const char *functionArg);

/* dk.c */
void DkArchiveDone(void);
void DkBeginArchiveFile(void);
void DkBeginCopyFile(void);
void DkEndArchiveFile(void);
DiskVolumeSeqnum_t DkGetVolumeSeqnum(char *path, char *fname);
void DkInit(void);
void DkSetDiskVolsFlag(struct DiskVolumeInfo *dv, int flag);
void DkUpdateVolumeSeqnum(char *path, char *fname);
int DkWrite(void *rft, void *buf, size_t nbytes);
int DkWriteError(void);

/* hc.c */
void HcArchiveDone(void);
void HcBeginArchiveFile(void);
void HcBeginCopyFile(void);
long HcCreateSeqnumMeta(void *stream, char *buf, long nbytes);
void HcEndArchiveFile(void);
void HcInit(void);
long HcReadFromFile(void *stream, char *buf, long nbytes);
int HcWrite(void *rft, void *buf, size_t nbytes);
int HcWriteError(void);
int strerror_r(int errnum, char *strerrbuf, size_t buflen);

/* header.c */
void BuildHeader(char *name, int name_l, struct sam_stat *st);

/* rm.c */
void RmBeginArchiveFile(void);
void RmEndArchiveFile(int firstFile);
void RmInit(void);
int RmWrite(int fd, void *buf, size_t nbytes);
int RmWriteError(void);

/* setarch.c */
void *SetArchive(void *arg);
void SetArchiveInit(struct Threads *th);
void SetArchiveRun(int fn);

/* sparse.c */
void GetFileStoreInfo(int);
offset_t GetFileSize(void);
int IsSparse(void);
int ReadSparse(int fd, void *buf, size_t nbyte, boolean_t *dataRead);

/* Function macros. */
#define	IoFatal(f, a) _IoFatal(_SrcFile, __LINE__, #f, a)

#endif /* ARCOPY_H */
