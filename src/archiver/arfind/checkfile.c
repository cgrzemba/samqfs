/*
 * checkfile.c - Check status of file.
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

#pragma ident "$Revision: 1.74 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* FILE_TRACE	If defined, turn on DEBUG traces for files processed */

/* ANSI C headers. */
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "pub/lib.h"
#include "sam/lib.h"
#include "sam/syscall.h"
#include "aml/tar.h"

/* Local headers. */
#include "arfind.h"
#include "dir_inode.h"
#if defined(lint)
#undef snprintf
#endif /* defined(lint) */

/* Private functions. */
static void checkAnInode(struct PathBuffer *pb, struct sam_perm_inode *pinode,
		struct FilePropsEntry *fp, struct ScanListEntry *se);
static void checkArchiveStatus(struct PathBuffer *pb,
		struct sam_perm_inode *pinode, struct FilePropsEntry *fp,
		struct ScanListEntry *se);
static void changeFileAttributes(struct PathBuffer *pb,
		struct sam_perm_inode *pinode, struct FilePropsEntry *fp);
static void checkRearchive(struct PathBuffer *pb,
		struct sam_perm_inode *pinode);
static void checkSegments(struct PathBuffer *pb, struct sam_perm_inode *pinode,
		struct FilePropsEntry *fp, struct ScanListEntry *se);
static char *fixPath(struct PathBuffer *pb);
static void setArchflags(char *filePath, struct sam_disk_inode *dinode,
		int copiesReq);
static int unarchiveFile(struct PathBuffer *pb, struct sam_perm_inode *pinode,
		int asn, int copy);



/*
 * Check file.
 * Determine if the file needs to be archived now.
 */
void
CheckFile(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode,	/* Permanent inode */
	struct ScanListEntry *se)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	struct FilePropsEntry *fp;

#if defined(CHECK_PATH)
	sam_id_t id;

	if (PathToId(pb->PbPath, &id) != 0) {
		Trace(TR_DEBUGERR, "%d stat(%d.%d, %s)", EXAM_MODE(dinode),
		    dinode->id.ino, dinode->id.gen, pb->PbPath);
		return (&FileProps->FpEntry[0]);
	}
	if (id.ino != dinode->id.ino) {
		Trace(TR_DEBUGERR, "%d %d.%d != %d.%d %s", EXAM_MODE(dinode),
		    dinode->id.ino, dinode->id.gen, id.ino, id.gen, pb->PbPath);
		return (&FileProps->FpEntry[0]);
	}
#endif /* define(CHECK_PATH) */

#if defined(FILE_TRACE)
	char	c;

	if (S_ISLNK(dinode->mode)) {
		c = 'l';
	} else if (S_ISREQ(dinode->mode)) {
		c = 'R';
	} else if (S_ISSEGI(dinode)) {
		c = 'I';
	} else if (S_ISDIR(dinode->mode)) {
		c = 'd';
	} else if (S_ISSEGS(dinode)) {
		c = 'S';
	} else if (S_ISREG(dinode->mode)) {
		c = 'f';
	} else if (S_ISBLK(dinode->mode)) {
		c = 'b';
	} else {
		c = '?';
	}
	Trace(TR_DEBUG, "Checking %d.%d %s %c",
	    dinode->id.ino, dinode->id.gen, pb->PbPath, c);
#endif /* defined(FILE_TRACE) */

	if (Exec != ES_run && !(se->SeFlags & SE_request)) {
		if (State->AfExamine >= EM_noscan) {
			/*
			 * Not running, schedule a scan.
			 */
			se->SeTime = TIME_NOW(dinode);
		}
		return;
	}

	/*
	 * Classify files.
	 */
	fp = ClassifyFile(pb->PbPath, pinode, &se->SeTime);
	if (fp == NULL) {
		checkRearchive(pb, pinode);
		return;
	}

	/*
	 * Change attributes for the file if they are not as required.
	 */
	if ((dinode->status.bits & fp->FpMask) != fp->FpStatus) {
		changeFileAttributes(pb, pinode, fp);
	}
	if (fp->FpFlags & FP_noarch) {
		/* A "no_archive" Archive Set */
		checkRearchive(pb, pinode);
		return;
	}
	if (!S_ISSEGI(dinode)) {
		if (S_ISSEGS(dinode)) {
			int	n;

			/*
			 * Add data segment number to end of path.
			 */
			n = snprintf(pb->PbEnd, PATHBUF_SIZE - PATHBUF_END,
			    "/%d", dinode->rm.info.dk.seg.ord + 1);
			pb->PbEnd += n;
			checkAnInode(pb, pinode, fp, se);
			pb->PbEnd -= n;
		} else {
			checkAnInode(pb, pinode, fp, se);
		}
	} else {
		/*
		 * Segmented file.
		 * The segments may by archived independently.
		 * checkSegments() calls checkAnInode() for each segment.
		 */
		checkSegments(pb, pinode, fp, se);
	}
}


/*
 * Set 'archdone' for a file.
 */
void
SetArchdone(
	char *filePath,
	struct sam_disk_inode *dinode)
{
	if (((State->AfFlags & ASF_setarchdone) ||
	    EXAM_MODE(dinode) == AE_archive) && !dinode->status.b.archdone) {
		dinode->status.b.archdone = TRUE;
		setArchflags(filePath, dinode, 0);
		Trace(TR_DEBUG, "Set archdone inode: %d.%d (%s)",
		    dinode->id.ino, dinode->id.gen, filePath);
	}
}


/* Private functions. */


/*
 * Check inode.
 */
static void
checkAnInode(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode,
	struct FilePropsEntry *fp,
	struct ScanListEntry *se)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	boolean_t saveArchdone;
	boolean_t saveArchnodrop;
	int	copy;
	int	copyBit;
	int	copiesReq;

	/*
	 * If the file is allowed to be archived, and not damaged,
	 * determine what archive action is required.
	 */
	if (dinode->status.b.noarch || dinode->status.b.damaged) {
		return;
	}

	saveArchdone = dinode->status.b.archdone;
	saveArchnodrop = dinode->status.b.archnodrop;
	checkArchiveStatus(pb, pinode, fp, se);

	if ((State->AfFlags & ASF_setarchdone) ||
	    EXAM_MODE(dinode) == AE_archive) {
		/*
		 * If the "archdone" or "archnodrop" have changed in our copy
		 * of the inode, or the copies required is not correct, change
		 * them in the inode.
		 */
		copiesReq = 0;
		for (copy = 0, copyBit = 1; copy < MAX_ARCHIVE;
		    copy++, copyBit <<= 1) {
			copiesReq |=
			    (dinode->ar_flags[copy] & AR_required) ?
			    copyBit : 0;
		}
		if (dinode->status.b.archdone != saveArchdone ||
		    dinode->status.b.archnodrop != saveArchnodrop ||
		    copiesReq != fp->FpCopiesReq) {
			setArchflags(pb->PbPath, dinode, fp->FpCopiesReq);
		}
	}
}


/*
 * Check archive status.
 * Determine if the file needs to be archived.
 */
static void
checkArchiveStatus(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode,
	struct FilePropsEntry *fp,
	struct ScanListEntry *se)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	time_t	later;
	time_t	archRef;
	time_t	accessRef;
	time_t	modifyRef;
	boolean_t unarch;
	int	copy;
	/* Following contain one bit per copy. */
	int	copyBit;
	int	copiesDamaged;
	int	copiesLater;
	int	copiesReq;
	int	copiesToholdRelease;
	int	copiesTomake;

	/*
	 * Set access and modification reference times.
	 */
	modifyRef = dinode->modify_time.tv_sec;
#if defined(FILE_TRACE)
	Trace(TR_DEBUG, "Modtime %d.%d %s %s", dinode->id.ino, dinode->id.gen,
	    pb->PbPath, TimeToIsoStr(modifyRef, NULL));
#endif /* defined(FILE_TRACE) */
	archRef = modifyRef;
	accessRef = dinode->access_time.tv_sec;

	/*
	 * Determine which archive copies need to be made and which copies
	 * should be unarchived.
	 */
	copiesDamaged = 0;
	copiesLater = 0;
	copiesToholdRelease = fp->FpCopiesNorel;
	copiesTomake = 0;
	later = TIME_MAX;
	copiesReq = fp->FpCopiesReq;
	unarch = FALSE;
	for (copy = 0, copyBit = 1; copy < MAX_ARCHIVE; copy++, copyBit <<= 1) {
		time_t	actionTime;

		if (!(fp->FpCopiesReq & copyBit)) {

			/*
			 * This copy is not defined for the archive set.
			 * Check if a copy was previously made, and must
			 * be rearchived.
			 */
			if ((dinode->arch_status & copyBit) &&
			    (dinode->ar_flags[copy] & AR_rearch)) {
				/*
				 * Copy exists and rearchive set -
				 * unarchive copy if it has been archived
				 * and another copy is archived.
				 */
				if ((dinode->arch_status & copyBit) &&
				    (dinode->arch_status & ~copyBit)) {
					(void) unarchiveFile(pb, pinode,
					    fp->FpAsn[copy], copy);
				}
			}
			continue;
		}

		if ((fp->FpCopiesUnarch & copyBit) &&
		    (fp->FpCopiesReq & ~copyBit) != 0) {
			time_t	unarchRef;

			/*
			 * Copy is to be unarchived; and more than one copy
			 * is required.  Select the unarchive reference time
			 * based on modify or access time.
			 */
			if (ArchSetTable[fp->FpAsn[copy]].AsEflags &
			    AE_unarchage) {
				unarchRef = modifyRef;
			} else {
				unarchRef = accessRef;
			}
			actionTime = unarchRef + fp->FpUnarchAge[copy];
			unarch = TRUE;
			/*
			 * Prevent setting archdone
			 */
			copiesReq |= 1 << MAX_ARCHIVE;
#if defined(FILE_TRACE)
			Trace(TR_DEBUG, "Unarchive %d.%d %s %d at %s",
			    dinode->id.ino, dinode->id.gen, pb->PbPath,
			    copy, TimeToIsoStr(actionTime, NULL));
#endif /* defined(FILE_TRACE) */
			if (actionTime <= TIME_NOW(dinode)) {
				/*
				 * unarchive-age has passed -
				 * unarchive copy if it has been archived
				 * and another copy is archived.
				 */
				if (dinode->arch_status & copyBit) {
					if (dinode->arch_status & ~copyBit) {
						if (unarchiveFile(pb, pinode,
						    fp->FpAsn[copy],
						    copy) == -1) {
							later = min(later,
							    actionTime);
							se->SeAsn =
							    fp->FpBaseAsn;
							copiesLater |= copyBit;
						}
					}
				}
				copiesToholdRelease &= ~copyBit;
				continue;
			} else {
				later = min(later, actionTime);
				se->SeAsn = fp->FpBaseAsn;
				copiesLater |= copyBit;
			}
		}

		if (dinode->arch_status & copyBit) {
			/*
			 * Archive copy already made.
			 */
			if (dinode->ar_flags[copy] & AR_damaged) {
				/*
				 * A damaged copy cannot be staged to make
				 * an archive copy.  Note damaged copy.
				 */
				copiesDamaged |= copyBit;
			}
			if (!(dinode->ar_flags[copy] & AR_rearch)) {
				/*
				 * File is not to be rearchived.
				 */
				continue;
			}
		}

		if (dinode->ar_flags[copy] & (AR_arch_i | AR_rearch)) {
			/*
			 * File is to be archived immediately or rearchived.
			 */
			copiesTomake |= copyBit;
			actionTime = TIME_NOW(dinode);
		} else {
			/*
			 * Archive copy needs to be made.
			 */
			actionTime = archRef + fp->FpArchAge[copy];
		}
		if (actionTime <= TIME_NOW(dinode)) {
			/*
			 * It is past the time to archive.
			 */
			copiesTomake |= copyBit;
		} else {
			later = min(later, actionTime);
			se->SeAsn = fp->FpBaseAsn;
			copiesLater |= copyBit;
		}
	}

	/*
	 * Check if all required copies before allowing the release
	 * of the file are made. Set 'archnodrop' if not.
	 */
	if ((dinode->arch_status & copiesToholdRelease) !=
	    copiesToholdRelease) {
		dinode->status.b.archnodrop = TRUE;
	} else {
		dinode->status.b.archnodrop = FALSE;
	}

	/*
	 * Check for a file that cannot be staged.
	 * Avoid the corner case when all copies are marked stale, but none of
	 * them is damaged. (i.e. file is offline, but arch_status == 0).
	 * This happens during modification of an offline file.
	 * This is only a temporary status.
	 * After the modification, file is online with all copies staled.
	 */
	if (dinode->status.b.offline && copiesDamaged != 0 &&
	    copiesDamaged == dinode->arch_status) {
		/*
		 * Offline and all archive copies are damaged.
		 */
		if (!dinode->status.b.archdone) {
			/*
			 * Cannot archive file: offline with all copies
			 * damaged.  %s
			 */
			SendCustMsg(HERE, 4028, pb->PbPath);
		}
		dinode->status.b.archdone = TRUE;
		return;
	}

	/*
	 * Check all copies to be archived.
	 */
	dinode->status.b.archdone = TRUE;
	for (copy = 0, copyBit = 1; copy < MAX_ARCHIVE; copy++, copyBit <<= 1) {
		if (copiesTomake & copyBit) {
			dinode->status.b.archdone = FALSE;

			/*
			 * Copy needs to be made now.
			 */
			if (Exec != ES_run) {
				/*
				 * Not running, schedule a scan.
				 */
				se->SeCopies |= copyBit;
				se->SeTime = TIME_NOW(dinode);
				continue;
			}
			copiesTomake &= ~copyBit;
#if defined(FILE_TRACE)
			Trace(TR_DEBUG, "Addfile %d.%d %s %d %x %x",
			    dinode->id.ino, dinode->id.gen, pb->PbPath,
			    copy, copiesTomake, copiesReq);
#endif /* defined(FILE_TRACE) */
			if (ArchiveAddFile(pb, pinode, fp, copy,
			    TIME_NOW(dinode) - archRef, copiesReq,
			    unarch) > 0) {
				/*
				 * Rescan file later.
				 */
				Trace(TR_ARDEBUG, "AddFile failed %s %d",
				    pb->PbPath, copy + 1);
				later = min(later,
				    TIME_NOW(dinode) + State->AfInterval);
				copiesLater |= copyBit;
			}
		}
	}
	if (later != TIME_MAX) {
		/*
		 * Archive action later.
		 */
		dinode->status.b.archdone = FALSE;
#if defined(FILE_TRACE)
		Trace(TR_DEBUG, "Later %d.%d %s %d",
		    dinode->id.ino, dinode->id.gen, pb->PbPath,
		    (int)(later - TIME_NOW(dinode)));
#endif /* defined(FILE_TRACE) */
		if (State->AfExamine >= EM_noscan && later < se->SeTime) {
#if !defined(SCANMODE)
			Trace(TR_DEBUG, "Exam later inode: %d.%d "
			    "copies: 0x%x (%s) time: %s",
			    dinode->id.ino, dinode->id.gen, copiesLater,
			    pb->PbPath, TimeToIsoStr(later, NULL));

			ExamInodesAddEntry(dinode->id, EXAM_INODE, later,
			    "later");
#else /* !defined(SCANMODE) */
			se->SeCopies |= copiesLater;
			se->SeTime = later;
#endif /* !defined(SCANMODE) */
		}
	}
}


/*
 * Check rearchive.
 */
static void
checkRearchive(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode)
{
	static struct FilePropsEntry emptyFp;
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	int	copy;

	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		/*
		 * Check if a copy was previously made, and must be rearchived.
		 */
		if ((dinode->arch_status & (1 << copy)) &&
		    (dinode->ar_flags[copy] & AR_rearch)) {
			/*
			 * Copy exists and rearchive set - add to
			 * allsets ArchReq.
			 */
			(void) ArchiveAddFile(pb, pinode, &emptyFp, copy,
			    0, 0, 0);
		}
	}
	SetArchdone(pb->PbPath, dinode);
}


/*
 * Check Segments.
 * Determine if any of the segments need to be archived.
 */
static void
checkSegments(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode,
	struct FilePropsEntry *fp,
	struct ScanListEntry *se)
{
	struct sam_ioctl_idseginfo ss;
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	struct sam_perm_inode dsPinode;
	boolean_t indexArchdone;
	int	sn;

#if defined(FILE_TRACE)
	Trace(TR_DEBUG, "checkSegments(%d.%d %s)",
	    dinode->id.ino, dinode->id.gen,  pb->PbPath);
#endif /* defined(FILE_TRACE) */

	/*
	 * Check each segment of the file.
	 * Segments may be archived at different times.
	 */
	memset(&ss, 0, sizeof (ss));
	ss.size = 0;
	indexArchdone = TRUE;
	while ((sn =
	    GetSegmentInode(pb->PbPath, pinode, &dsPinode, &ss)) != -1) {
		struct sam_disk_inode *dsDinode =
		    (struct sam_disk_inode *)&dsPinode;
		int	n;

#if defined(FILE_TRACE)
		Trace(TR_DEBUG, "  Segment %d", sn);
#endif /* defined(FILE_TRACE) */
		TIME_NOW(dsDinode) = TIME_NOW(dinode);

		/*
		 * Add segment number to end of path.
		 */
		n = snprintf(pb->PbEnd, PATHBUF_SIZE - PATHBUF_END,
		    "/%d", sn + 1);
		pb->PbEnd += n;
		checkAnInode(pb, &dsPinode, fp, se);
		pb->PbEnd -= n;
		indexArchdone &= dsDinode->status.b.archdone;
	}

	/*
	 * If the "archdone" for the index inode does not reflect the data
	 * inodes, change it in the filesystem.
	 */
	*pb->PbEnd = '\0';
	if (dinode->status.b.archdone != indexArchdone) {
		dinode->status.b.archdone = indexArchdone;
		setArchflags(pb->PbPath, dinode, fp->FpCopiesReq);
	}
	SamFree(ss.buf.ptr);
}


/*
 * Change the attributes of a file.
 */
static void
changeFileAttributes(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode,
	struct FilePropsEntry *fp)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	ino_st_t mask;
	ino_st_t status;
	boolean_t restat;
	char	attr[32];
	char	*path;
	char	*p;

	restat = FALSE;
	path = fixPath(pb);
#if defined(FILE_TRACE)
	Trace(TR_DEBUG,
	    "Change attributes: %d.%d %s mask:%02x from:%02x to:%02x\n",
	    dinode->id.ino, dinode->id.gen, pb->PbPath,
	    fp->FpMask, dinode->status.bits, fp->FpStatus);
#endif /* defined(FILE_TRACE) */
	mask.bits = fp->FpMask;
	status.bits = fp->FpStatus;
	if (S_ISREG(dinode->mode) &&
	    (mask.b.bof_online | mask.b.release | mask.b.nodrop)) {
		/*
		 * Only regular files may be released.
		 */
		p = attr;
		if (!(status.b.bof_online |
		    status.b.release |
		    status.b.nodrop)) {
			*p++ = 'd';
		}
		if (status.b.release) {
			*p++ = 'a';
		}
		if (status.b.nodrop) {
			*p++ = 'n';
		}

		/*
		 * Avoid trying to set 'p' if csum in use.
		 * Otherwise the syscall will get an error.
		 */
		if (status.b.bof_online &&
		    !(dinode->status.b.cs_gen || dinode->status.b.cs_use)) {
			if (fp->FpPartial != 0) {
				(void) snprintf(p,
				    sizeof (attr) - Ptrdiff(p, attr), "s%d",
				    fp->FpPartial);
				p += strlen(p);
			} else {
				*p++ = 'p';
			}
		}
		*p++ = '\0';
		if (*attr != '\0') {
			if (sam_release(path, attr) != 0) {
				Trace(TR_DEBUGERR,
				    "release(%s, %s)", path, attr);
			} else {
				restat = TRUE;
				Trace(TR_FILES,
				    "Change attributes: %d.%d %s release(%s)",
				    dinode->id.ino, dinode->id.gen,
				    pb->PbPath, attr);
			}
		}
	}
	if (mask.b.stage_all | mask.b.direct) {
		p = attr;
		if (!(status.b.stage_all | status.b.direct)) {
			*p++ = 'd';
		}
		if (status.b.stage_all) {
			*p++ = 'a';
		}
		if (status.b.direct) {
			*p++ = 'n';
		}
		*p++ = '\0';
		if (sam_stage(path, attr) != 0) {
			Trace(TR_DEBUGERR, "stage(%s, %s)", path, attr);
		} else {
			restat = TRUE;
			Trace(TR_FILES, "Change attributes: %d.%d %s stage(%s)",
			    dinode->id.ino, dinode->id.gen, pb->PbPath, attr);
		}
	}
	if (restat) {
		/*
		 * We changed the attributes above.
		 * Get a fresh copy of the inode.
		 */
		if (GetPinode(dinode->id, pinode) != 0) {
			Trace(TR_DEBUGERR, "Re-stat(%d.%d, %s)",
			    dinode->id.ino, dinode->id.gen, path);
		}
		Trace(TR_FILES, "Re-stat: %d.%d %s",
		    dinode->id.ino, dinode->id.gen, pb->PbPath);
	}
}


/*
 * Make a file path into a full path.
 * IdToPath() constructs a path from the end.
 * If this is so, add the mount point to the beginning.
 */
static char *
fixPath(
	struct PathBuffer *pb)
{
	char	*p;

	p = pb->PbPath - 1;
	if (*p != '/') {
		int	l;

		/*
		 * Buffer for the file path does not contain mount point.
		 * Prepend it.
		 */
		*p = '/';
		l = strlen(MntPoint);
		p -= l;
		memmove(p, MntPoint, l);
	} else {
		p = pb->PbBuf;
	}
	return (p);
}


/*
 * Set the archive status flags.
 */
static void
setArchflags(
	char *filePath,
	struct sam_disk_inode *dinode,
	int copiesReq)
{
	struct sam_ioctl_archflags args;

	args.id		= dinode->id;
	args.archdone	= dinode->status.b.archdone;
	args.archnodrop	= dinode->status.b.archnodrop;
	args.copies_req = copiesReq;
	if (ioctl(FsFd, F_ARCHFLAGS, &args) < 0) {
		Trace(TR_ERR, "Set archflags failed inode: %d.%d (%s)",
		    args.id.ino, args.id.gen, filePath);
	}
	Trace(TR_DEBUG,
	    "Set archflags inode: %d.%d done: %d nodrop: %d copies: 0x%0x (%s)",
	    args.id.ino, args.id.gen, args.archdone, args.archnodrop,
	    args.copies_req, filePath);
}


/*
 * Unarchive file.
 */
static int
unarchiveFile(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode,
	int asn,
	int copy)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	struct sam_archive_copy_arg args;
	char	*p;
	char	*path;

#if defined(FILE_TRACE)
	Trace(TR_DEBUG, "unarchive(%d.%d %s, %d)",
	    dinode->id.ino, dinode->id.gen, pb->PbPath, copy);
#endif /* defined(FILE_TRACE) */
	path = fixPath(pb);
	memset(&args, 0, sizeof (args));
	args.operation	= OP_unarchive;
	args.path.ptr	= path;
	args.copies		= 1 << copy;
	if (S_ISSEGS(dinode)) {
		/*
		 * Skip back over the segment number.
		 */
		p = strrchr(path, '/');
		if (p != NULL) {
			*p = '\0';
		}
	} else {
		p = NULL;
	}
	if (sam_syscall(SC_archive_copy, &args, sizeof (args)) == -1) {
		Trace(TR_DEBUGERR, "unarchive(%s)", path);
		if (p != NULL) {
			*p = '/';
		}
		return (-1);
	}

	/*
	 * Clear archived status for this copy.
	 */
	dinode->arch_status &= ~(1 << copy);

	/*
	 * Log the unarchive if required.
	 */
	if (*State->AfLogFile != '\0') {
		if (LogStream == NULL &&
		    (LogStream = fopen64(State->AfLogFile, "a")) == NULL) {
			Trace(TR_ERR, "Logfile open(%s)", State->AfLogFile);
		}
	}
	if (LogStream != NULL) {
		struct tm tm;
		time_t	timeNow;
		uint32_t fileOffset;
		char	timestr[64];

		timeNow = TIME_NOW(dinode);
		strftime(timestr, sizeof (timestr)-1, "%Y/%m/%d %H:%M:%S",
		    localtime_r(&timeNow, &tm));
		if (pinode->ar.image[copy].arch_flags & SAR_size_block) {
			fileOffset = pinode->ar.image[copy].file_offset;
		} else {
			fileOffset =
			    pinode->ar.image[copy].file_offset / TAR_RECORDSIZE;
		}

		LogLock(LogStream);
		fprintf(LogStream, "U %s %s %s %s %x.%x %s %d.%d %lld %s\n",
		    timestr, sam_mediatoa(pinode->di.media[copy]),
		    pinode->ar.image[copy].vsn,
		    ArchSetTable[asn].AsName,
		    pinode->ar.image[copy].position, fileOffset,
		    FsName, dinode->id.ino, dinode->id.gen, dinode->rm.size,
		    pb->PbPath);
		(void) fflush(LogStream);
		LogUnlock(LogStream);
	}
	if (p != NULL) {
		*p = '/';
	}
	return (0);
}
