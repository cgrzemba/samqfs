/*
 * setarch.c - Set archive status for archived files.
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

#pragma ident "$Revision: 1.31 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <pthread.h>
#include <unistd.h>

/* Solaris headers. */
#include <sys/types.h>

/* SAM-FS headers. */
#include "pub/lib.h"
#include "pub/rminfo.h"

#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/sam_malloc.h"
#include "sam/lib.h"
#include "sam/uioctl.h"
#include "sam/fs/ino.h"
#include "aml/diskvols.h"
#include "aml/tar.h"

/* Local headers. */
#include "arcopy.h"

/* Private data. */
static pthread_cond_t setArchWait = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t setArchWaitMutex = PTHREAD_MUTEX_INITIALIZER;

static struct ArchiveFile **afsort;
static int filesCopied = 0;

/* Private functions. */
static void enterArchiveStatus(int firstFile, int nfiles);
static void logArchives(int firstFile, int end);
static void releaseFile(struct FileInfo *fi);
static void wakeup(void);
static int cmp_inode(const void *p1, const void *p2);


/*
 * Thread - Set archive status for files.
 */
void *
SetArchive(
	void *arg)
{
	int	nextFile;

	SamMalloc(afsort, FilesNumof * sizeof (struct ArchiveFile *));
	nextFile = 0;
	FilesArchived = 0;
	ThreadsInitWait(wakeup, wakeup);

	while (Exec < ES_term && nextFile < FilesNumof) {
		int	nfiles;

		PthreadMutexLock(&setArchWaitMutex);
		while (Exec < ES_term && (nfiles = filesCopied - nextFile) <=
		    0) {
			PthreadCondWait(&setArchWait, &setArchWaitMutex);
		}
		PthreadMutexUnlock(&setArchWaitMutex);

		while (nfiles > 0) {
			int	n;

			n = min(nfiles, LISTSETARCH_MAX);

			/*
			 * Give operator confidence.
			 */

			/* Entering archive information for %d files, %d left */
			PostOprMsg(4309, filesCopied, nfiles);

			enterArchiveStatus(nextFile, n);
			nfiles -= n;
			nextFile += n;
		}
	}

	ThreadsExit();
	/*NOTREACHED*/
	return (arg);
}


/*
 * Run setarchive.
 */
void
SetArchiveRun(
	int fn)
{
	filesCopied = fn;
	wakeup();
}


/*
 * Enter archive status for files.
 */
static void
enterArchiveStatus(
	int firstFile,	/* File to start with */
	int nfiles)
{
	static struct sam_ioctl_listsetarch lsa;
	static struct sam_ioctl_setarch *saBase = NULL;
	static struct sam_vsn_section *vpBase;
	static size_t saSize = 0;
	static size_t vpSize = 0;
	struct sam_ioctl_setarch *sa;
	struct sam_vsn_section *vp;
	size_t	size;
	int	fn;

	/*
	 * Enter files to be archived in sort table.
	 */
	for (fn = 0; fn < nfiles; fn++) {
		afsort[fn] = &FilesTable[firstFile + fn];
	}
	qsort(afsort, nfiles, sizeof (struct ArchiveFile *), cmp_inode);

	/*
	 * Allocate lists.
	 */
	size = nfiles * sizeof (struct sam_ioctl_setarch);
	if (size > saSize) {
		saSize = size;
		SamRealloc(saBase, saSize);
		memset(saBase, 0, saSize);
	}
	size = VolsTable->count * sizeof (struct sam_vsn_section);
	if (size > vpSize) {
		vpSize = size;
		SamRealloc(vpBase, vpSize);
		memset(vpBase, 0, vpSize);
	}

	/*
	 * Enter archive information for files in sort table.
	 */
	sa = saBase;
	vp = vpBase;
	lsa.lsa_count = 0;
	for (fn = 0; fn < nfiles; fn++) {
		struct FileInfo *fi;
		struct VolsEntry *ve;
		struct ArchiveFile *af;

		af = afsort[fn];
		fi = af->f;
		if (!(af->AfFlags & AF_copied)) {
			if (af->AfFlags & AF_release) {
				releaseFile(fi);
			}
			fi->FiFlags |= (af->AfFlags & AF_error) ? FI_error : 0;
			continue;
		}

		sa->copy = ArchiveCopy;
		sa->vp.ptr = vp;
		/*
		 * Enter the archive status.
		 */
		sa->id = fi->FiId;
		if (af->AfFlags & AF_release) {
			sa->sa_copies_norel = 0;
			sa->sa_copies_rel = 1 << ArchiveCopy;
		} else {
			sa->sa_copies_norel = fi->FiCopiesNorel;
			sa->sa_copies_rel = fi->FiCopiesRel;
		}
		sa->sa_copies_req =  fi->FiCopiesReq;
		if (af->AfFlags & AF_csummed) {
			sa->flags = SA_csummed;
			sa->csum = af->AfCsum;
		}
		sa->access_time = af->AfAccessTime;
		sa->modify_time = af->AfModifyTime;

		ve = &VolsTable->entry[af->AfVol];
		/*
		 * ZeroOffset indicates that the offset points to the
		 * beginning of the tar header block instead of the
		 * beginning of the file data.  This is configured implicitly
		 * from defaults.conf:  Any format other than 'legacy'
		 * (currently limited to 'pax') should set this bit.
		 */
		sa->ar.arch_flags = ((ZeroOffset) ? SAR_pax_hdr : 0);
		sa->ar.version = 0;
		sa->ar.creation_time	= af->AfCreateTime;

		sa->ar.position_u = 0;
		sa->ar.position = (uint32_t)af->AfPosition;

		/*
		 * Set offset of file in this volume.
		 */
/*LINTED warning: constant promoted according to the 1999 ISO C standard */
		if (af->AfOffset >= SAM_MAXOFF_T) {
			sa->ar.file_offset = af->AfOffset / TAR_RECORDSIZE;
			sa->ar.arch_flags |= SAR_size_block;
		} else {
			sa->ar.file_offset = af->AfOffset;
		}
		/*
		 * Set disk archive flag for this copy.
		 */
		if (Instance->CiFlags & CI_diskInstance) {
			sa->ar.arch_flags |= SAR_diskarch;
		}
		sa->media = sam_atomedia(ve->Vi.VfMtype);
		memmove(sa->ar.vsn, ve->Vi.VfVsn, sizeof (sa->ar.vsn));

		/*
		 * Check file for overflow in this volume.
		 */
		if (!(af->AfFlags & AF_overflow)) {
			/*
			 * File has not overflowed.
			 */
			sa->ar.n_vsns	= 1;
			if (sa->ar.position == 0 &&
			    (ArchiveMedia & DT_CLASS_MASK) != DT_DISK) {
				snprintf(ScrPath, sizeof (ScrPath), "%s.%s",
				    ve->Vi.VfMtype, ve->Vi.VfVsn);
				LibFatal(Zero_position, ScrPath);
			}
		} else {
			/*
			 * File has overflowed a volume.
			 * Identify how many volumes have been written for this
			 * file by comparing the file's size with succeeding
			 * volume's length.  Enter section data during the
			 * process.
			 */
			fsize_t	accum;

			sa->ar.n_vsns = 0;
			sa->vp.ptr = vp;
			for (accum = 0;
			    accum < af->AfFileSize; /* advance inside */) {
				memmove(vp->vsn, ve->Vi.VfVsn,
				    sizeof (vp->vsn));
				vp->position = ve->VlPosition;

				/*
				 * Set section offset and length.
				 * Offset for first section is the file offset.
				 * Succeeding sections have a 0 offset.
				 */
				vp->offset =
				    (sa->ar.n_vsns == 0) ? af->AfOffset : 0;
				vp->length = ve->VlLength - vp->offset;
				if ((accum + vp->length) > af->AfFileSize) {
					vp->length = af->AfFileSize - accum;
				}
				if (vp->position == 0) {
					snprintf(ScrPath, sizeof (ScrPath),
					    "%s.%s", ve->Vi.VfMtype,
					    ve->Vi.VfVsn);
					LibFatal(Zero_section_position,
					    ScrPath);
				}
				accum += vp->length;

				/*
				 * Advance search.
				 */
				ve++;
				vp++;
				sa->ar.n_vsns++;
			}
		}
		sa++;
		lsa.lsa_count++;
	}
	if (lsa.lsa_count <= 0) {
		return;
	}
	lsa.lsa_list.ptr = saBase;
	if (ioctl(FsFd, F_LISTSETARCH, &lsa) < 0) {
		LibFatal(F_LISTSETARCH, "");
	}

	/*
	 * Set archive status.
	 */
	sa = saBase;
	for (fn = 0; fn < nfiles; fn++) {
		struct FileInfo *fi;
		struct ArchiveFile *af;

		af = afsort[fn];
		fi = af->f;
		if (!(af->AfFlags & AF_copied)) {
			continue;
		}
		if (sa->flags & SA_error) {
			fi->FiFlags |= FI_error;
			fi->FiErrno = errno = sa->error;
			if (errno != ENOENT) {
				Trace(TR_DEBUGERR, "archive(%d.%d %s)",
				    sa->id.ino, sa->id.gen, fi->FiName);
			}
		} else {
			fi->FiFlags |= FI_archived;
			FilesArchived++;
		}
		if (*TraceFlags & TR_files && !(sa->flags & SA_error)) {
			if (!(af->AfFlags & AF_overflow)) {
				Trace(TR_FILES,
				    "Archive %d.%d %s copies:%x-%x"
				    " flags:%x %llx.%x",
				    sa->id.ino, sa->id.gen, fi->FiName,
				    sa->sa_copies_norel, sa->sa_copies_req,
				    sa->ar.arch_flags,
				    ((fsize_t)sa->ar.position_u << 32) |
				    (uint32_t)sa->ar.position,
				    sa->ar.file_offset);
			} else {
				int	i;

				Trace(TR_FILES, "Archive %d.%d %s  copies:%x-%x"
				    " flags:%x %lld",
				    sa->id.ino, sa->id.gen, fi->FiName,
				    sa->sa_copies_norel, sa->sa_copies_req,
				    sa->ar.arch_flags, af->AfFileSize);
				for (i = 0, vp = sa->vp.ptr; i < sa->ar.n_vsns;
				    i++, vp++) {
					Trace(TR_FILES,
					    "  section %d: %llx.%llx %lld %s",
					    i, vp->position,
					    vp->offset / TAR_RECORDSIZE,
					    vp->length, vp->vsn);
				}
			}
		}
		sa++;
	}
	logArchives(firstFile, nfiles);
}


/*
 * Log archives.
 */
static void
logArchives(
	int first,
	int nfiles)
{
	FILE	*log_st;	/* Log file stream */
	int	fn;

	if (*LogName == '\0') {
		return;
	}
	if ((log_st = fopen64(LogName, "a")) == NULL) {
		Trace(TR_ERR, "Logfile open(%s)", LogName);
		return;
	}
	LogLock(log_st);
	for (fn = first; fn < first + nfiles; fn++) {
		struct FileInfo *fi;
		struct VolsEntry *ve;
		struct ArchiveFile *af;
		struct tm *tm;
		char	timestr[64];
		char	type;
		char	*vsn;

		af = &FilesTable[fn];
		fi = af->f;
		if (!(fi->FiFlags & FI_archived)) {
			/*
			 * Skip unarchived files.
			 */
			continue;
		}

		if (fi->FiStatus & FIF_rearch) {
			type = 'R';
		} else {
			type = 'A';
		}

		tm = localtime(&af->AfCreateTime);
		strftime(timestr, sizeof (timestr)-1, "%Y/%m/%d %H:%M:%S", tm);

		ve = &VolsTable->entry[af->AfVol];
		if ((Instance->CiFlags & CI_diskInstance) == 0) {
			vsn = ve->Vi.VfVsn;
		} else {
			if (Instance->CiFlags & CI_disk) {
				static char	fileName[sizeof (vsn_t) +
				    sizeof (upath_t) + 2];
				DiskVolumeSeqnum_t seqnum;
				int	l;

				/*
				 * Generate file name for the archive file.
				 * VfVsn has the disk volume, and the sequence
				 * number is used to generate the path of the
				 * tarball.
				 */
				snprintf(fileName, sizeof (fileName),
				    "%s/", ve->Vi.VfVsn);
				l = strlen(fileName);
				seqnum = af->AfPosition;
				(void) DiskVolsGenFileName(seqnum, fileName + l,
				    sizeof (fileName) - l);
				vsn = fileName;
			} else {
				static char *archiveId = NULL;
				/*
				 * Generate archive id metadata string for the
				 * archive file.  The disk volume name and
				 * sequence number is used to generate the
				 * metadata string.
				 */
				archiveId =
				    DiskVolsGenMetadataArchiveId(ve->Vi.VfVsn,
				    af->AfPosition, archiveId);
				vsn = archiveId;
			}
		}
		if (!(af->AfFlags & AF_overflow)) {
			/*
			 * File has not overflowed.
			 */
			fprintf(log_st,
			    "%c %s %s %s %s %x.%llx %s %d.%d %lld"
			    " %s %c %d %d\n",
			    type,
			    timestr, ve->Vi.VfMtype, vsn,
			    ArchReq->ArAsname,
			    (uint32_t)af->AfPosition,
			    af->AfOffset / TAR_RECORDSIZE,
			    ArchReq->ArFsname,
			    fi->FiId.ino,
			    fi->FiId.gen,
			    af->AfFileSize,
			    fi->FiName,
			    fi->FiType,
			    0,
			    ve->VlEq);
		} else {
			/*
			 * File has overflowed a volume.
			 * Identify how many volumes have been written for this
			 * file by comparing the file's size with succeeding
			 * volume's length. Log each section during the process.
			 */
			fsize_t accum;
			fsize_t	length;
			int	section;

			section = 0;
			for (accum = 0; accum < af->AfFileSize;
			    accum += length) {
				fsize_t	offset;

				offset = (section == 0) ? af->AfOffset : 0;
				length = ve->VlLength - offset;
				if ((accum + length) > af->AfFileSize) {
					length = af->AfFileSize - accum;
				}
				fprintf(log_st,
				    "%c %s %s %s %s %x.%llx %s %d.%d %lld"
				    " %s %c %d %d\n",
				    type,
				    timestr, ve->Vi.VfMtype, ve->Vi.VfVsn,
				    ArchReq->ArAsname,
				    (uint32_t)ve->VlPosition,
				    offset / TAR_RECORDSIZE,
				    ArchReq->ArFsname,
				    fi->FiId.ino,
				    fi->FiId.gen,
				    length,
				    fi->FiName,
				    fi->FiType,
				    section,
				    ve->VlEq);
				section++;
				ve++;
			}
		}
	}
	(void) fflush(log_st);
	LogUnlock(log_st);
	(void) fclose(log_st);
}


/*
 * Release disk space if allowed.
 * Don't release the file if copy required for release.
 */
static void
releaseFile(
	struct FileInfo *fi)
{
	if (fi->FiCopiesNorel & ~(1 << ArchiveCopy)) {

		/*
		 * This is not the only copy involved in the release criteria.
		 * Check the status of the others.
		 */
		static struct sam_disk_inode dinode;
		static struct sam_ioctl_idstat IdstatArgs = {
			{ 0, 0 }, sizeof (dinode), &dinode };

		IdstatArgs.id = fi->FiId;
		if (ioctl(FsFd, F_IDSTAT, &IdstatArgs) < 0) {
			if (errno != ENOENT) {
				Trace(TR_DEBUGERR, "stat(%d.%d, %s/%s)",
				    IdstatArgs.id.ino, IdstatArgs.id.gen,
				    MntPoint, fi->FiName);
			}
			return;
		}
		if ((dinode.arch_status & fi->FiCopiesNorel) !=
		    fi->FiCopiesNorel) {
			/*
			 * All required copies not made.
			 */
			return;
		}
	}
	if (sam_release(fi->FiName, "i") == -1) {
		if (errno != ENOENT) {
			Trace(TR_DEBUGERR, "release(%s/%s)",
			    MntPoint, fi->FiName);
		}
	} else {
		Trace(TR_FILES, "Released: %d.%d %s",
		    fi->FiId.ino, fi->FiId.gen, fi->FiName);
	}
}


/*
 * Wakeup SetArchive thread.
 */
static void
wakeup(void)
{
	PthreadMutexLock(&setArchWaitMutex);
	PthreadCondSignal(&setArchWait);
	PthreadMutexUnlock(&setArchWaitMutex);
}


/*
 * Compare archive file inodes for ascending sort.
 */
static int
cmp_inode(
	const void *p1,
	const void *p2)
{
	struct ArchiveFile **a1 = (struct ArchiveFile **)p1;
	struct ArchiveFile **a2 = (struct ArchiveFile **)p2;

	if ((*a1)->f->FiId.ino < (*a2)->f->FiId.ino) {
		return (-1);
	}
	if ((*a1)->f->FiId.ino > (*a2)->f->FiId.ino) {
		return (1);
	}
	return (0);
}
