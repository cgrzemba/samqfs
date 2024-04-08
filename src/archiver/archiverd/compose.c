/*
 * compose.c - compose archive requests.
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

#pragma ident "$Revision: 1.86 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pthread.h>
#include <pwd.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "aml/tar.h"
#include "sam/fs/ino.h"
#include "sam/uioctl.h"

/* Local headers. */
#define	NEED_ARCHSET_NAMES
#include "archiverd.h"
#include "device.h"
#include "volume.h"

/* Private data. */
static pthread_cond_t composeWait = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t composeWaitMutex = PTHREAD_MUTEX_INITIALIZER;
static struct Queue composeQ = {"compose" };
static struct FileInfo *fiJoin = NULL;	/* File entry for 'join' file */
static struct FileInfo **fiList = NULL;	/* File info list for ordering files */
static struct ArchReq *arFiList = NULL;	/* ArchReq for file info list */
static size_t fiListAlloc = 0;
static char *arname;

/* Private functions. */
static struct ArchReq *addJoinFile(struct ArchReq *ar, struct ArchSet *as,
		int start, int end);
static void checkOffline(struct ArchReq *ar);
static int cmp_fmodtime(const void *p1, const void *p2);
static int cmp_fpath(const void *p1, const void *p2);
static int cmp_fpriority(const void *p1, const void *p2);
static int cmp_fsize(const void *p1, const void *p2);
static int cmp_order(const void *p1, const void *p2);
static int cmp_rmodtime(const void *p1, const void *p2);
static int cmp_rpath(const void *p1, const void *p2);
static int cmp_rpriority(const void *p1, const void *p2);
static int cmp_rsize(const void *p1, const void *p2);
static int cmp_segments(const void *p1, const void *p2);
static int cmp_VSNs(const void *p1, const void *p2);
static boolean_t findStageVolume(struct ArchReq *ar);
static struct ArchReq *joinFiles(struct ArchReq *ar, struct ArchSet *as);
static void makeFileInfoList(struct ArchReq *ar);
static void makeJoinFile(void);
static void prepareArchReq(struct ArchReq *ar);
static void pruneArchReq(struct ArchReq *ar);
static void sortFiles(struct ArchReq *ar, struct ArchSet *as);
static void sortSegments(struct ArchReq *ar);
static void wakeup(void);


/*
 * Thread - Compose ArchReqs in the queue.
 */
void *
Compose(
	void *arg)
{
	QueueInit(&composeQ);
	ThreadsInitWait(wakeup, wakeup);

	while (AdState->AdExec < ES_term) {
		struct QueueEntry *qe;
		struct ArchReq *ar;
		struct ArchSet *as;

		/*
		 * Wait for entries in the queue.
		 */
		ThreadsReconfigSync(RC_allow);
		PthreadMutexLock(&composeWaitMutex);
		while (AdState->AdExec < ES_term &&
		    composeQ.QuHead.QeFwd == &composeQ.QuHead) {
			PthreadCondWait(&composeWait, &composeWaitMutex);
		}
		if (AdState->AdExec >= ES_term) {
			PthreadMutexUnlock(&composeWaitMutex);
			break;
		}
		ThreadsReconfigSync(RC_wait);

		/*
		 * Remove and process next compose queue entry.
		 */
		qe = composeQ.QuHead.QeFwd;
		ar = qe->QeAr;
		arname = qe->QeArname;
		QueueRemove(qe);
		PthreadMutexUnlock(&composeWaitMutex);

		arFiList = NULL;
		if ((ar->ArStatus & (ARF_changed | ARF_merge | ARF_rmreq)) ||
		    (ar->ArFlags & AR_unqueue)) {
			ar->ArFlags &= AR_unqueue;
			MessageReturnQueueEntry(qe);
			continue;
		}
		if (!(ar->ArFlags & AR_first)) {
			/*
			 * Remove processed files.
			 */
			pruneArchReq(ar);
			if (ar->ArFiles == 0) {
				MessageReturnQueueEntry(qe);
				continue;
			}
			ar->ArState = ARS_schedule;
		}

		ar->ArFlags &= AR_offline | AR_segment;
		as = FindArchSet(ar->ArAsname);
		if (as == NULL) {
			Trace(TR_MISC, "Invalid ArchReq %s", arname);
			MessageReturnQueueEntry(qe);
			continue;
		}
		if (as->AsFlags & AS_disk_archive) {
			ar->ArFlags |= AR_disk;
		} else if (as->AsFlags & AS_honeycomb) {
			ar->ArFlags |= AR_honeycomb;
		}
		ar->ArDrivesUsed = 0;
		ar->ArDivides = DM_none;
		ar->ArSelFiles = ar->ArFiles;
		ar->ArSelSpace = ar->ArSpace;

		/*
		 * Process the compositions as required.
		 */
		if (ar->ArFlags & AR_offline) {
			/*
			 * Check offline files for available stage volumes.
			 */
			checkOffline(ar);
			ar->ArDivides = DM_offline;
		}
		if (ar->ArFlags & AR_segment) {
			ar->ArDivides = DM_segment;
			sortSegments(ar);
		}
		if (as->AsJoin != JM_none) {
			ar = joinFiles(ar, as);
			qe->QeAr = ar;
		}
		if (as->AsSort != SM_none) {
			sortFiles(ar, as);
		}
		if (as->AsReserve & RM_owner) {
			ar->ArDivides = (as->AsReserve & RM_dir) ?
			    DM_ownerDir : DM_ownerUidGid;
		}
		prepareArchReq(ar);
		ScheduleEnqueue(qe);
	}

	ThreadsExit();
	/*NOTREACHED*/
	return (arg);
}


/*
 * Divide an ArchReq for multiple drives.
 */
void
ComposeDrives(
	struct ArchReq *ar,
	struct ArchSet *as,
	int drives)
{
	struct FileInfo *fi;
	int	*fii;
	int	i;

	ar->ArDrivesUsed = drives;
	fii = LOC_FILE_INDEX(ar);
	memset(&ar->ArCpi[0], 0, ar->ArDrives * sizeof (struct ArcopyInstance));

	/*
	 * Mark all files for 'more' processing.
	 */
	for (i = 0; i < ar->ArSelFiles; i++) {
		fi = LOC_FILE_INFO(i);
		if (fi->FiCpi == CPI_later) {
			continue;
		}
		fi->FiCpi = CPI_more;
	}

	if (drives > 1) {
		ArchReqMsg(HERE, ar, 4315, drives);
	} else if (ar->ArDivides == DM_segment) {
		ar->ArDivides = DM_none;
	}

	/*
	 * Select files for each copy instance.
	 */
	for (i = 0; i < drives; i++) {
		ComposeForCpi(ar, as, i);
	}
}


/*
 * Add entry to compose queue.
 */
void
ComposeEnqueue(
	struct QueueEntry *qe)
{
	qe->QeAr->ArSchedPriority = qe->QeAr->ArPriority;

	/*
	 * Run the Compose() thread.
	 */
	PthreadMutexLock(&composeWaitMutex);
	QueueAdd(qe, &composeQ);
	PthreadCondSignal(&composeWait);
	PthreadMutexUnlock(&composeWaitMutex);
}


/*
 * Select files in an ArchReq for one copy instance.
 */
void
ComposeForCpi(
	struct ArchReq *ar,
	struct ArchSet *as,
	int cpi_a)
{
	struct ArcopyInstance *ci;
	struct FileInfo *fi;
	fsize_t	driveMax;
	short	cpi = (short)cpi_a;
	int	*fii;
	int	i;
	int	more;

	/*
	 * Set the maximum size for a copy instance.
	 * Set it for at least the minimum size file.
	 */
	driveMax = (as->AsFlags & AS_drivemax) ? as->AsDrivemax : FSIZE_MAX;
	driveMax = min(ar->ArSelSpace / ar->ArDrivesUsed, driveMax);
	driveMax = max(ar->ArMinSpace, driveMax);
	fii = LOC_FILE_INDEX(ar);
	ci = &ar->ArCpi[cpi];
	memset(ci, 0, sizeof (struct ArcopyInstance));
	ci->CiMinSpace = FSIZE_MAX;
	more = 0;

	switch (ar->ArDivides) {

	case DM_none:
	default:
		/*
		 * Select files to balance the load.
		 */
		for (i = 0; i < ar->ArSelFiles; i++) {
			fi = LOC_FILE_INFO(i);
			if (fi->FiCpi != CPI_more) {
				continue;
			}
			if (ci->CiSpace + fi->FiSpace > driveMax) {
				more++;
				continue;
			}
			fi->FiCpi = cpi;
			ci->CiFiles++;
			ci->CiSpace += fi->FiSpace;
			ci->CiMinSpace = min(ci->CiMinSpace, fi->FiSpace);
		}
		break;

	case DM_offline: {
		char	*firstVsn;

		/*
		 * Select files from the same offline volume.
		 */
		firstVsn = "";
		for (i = 0; i < ar->ArSelFiles; i++) {
			char	*vsn;

			fi = LOC_FILE_INFO(i);
			if (fi->FiCpi != CPI_more) {
				continue;
			}
			if (!(fi->FiFlags & FI_offline)) {
				vsn = " ";
			} else {
				struct OfflineInfo *oi;

				oi = LOC_OFFLINE_INFO(fi);
				vsn = oi->OiVsn;
			}
			if (*firstVsn == '\0') {
				firstVsn = vsn;
			}
			if (strcmp(firstVsn, vsn) != 0) {
				more++;
				continue;
			}
			if (ci->CiSpace + fi->FiSpace > driveMax) {
				more++;
				continue;
			}
			fi->FiCpi = cpi;
			ci->CiFiles++;
			ci->CiSpace += fi->FiSpace;
			ci->CiMinSpace = min(ci->CiMinSpace, fi->FiSpace);
		}
		}
		break;

	case DM_ownerDir: {
		int	oLen;

		/*
		 * Select file entries for the same owner directory.
		 */
		oLen = 0;
		for (i = 0; i < ar->ArSelFiles; i++) {
			char	*p, *pe;
			int	l;

			fi = LOC_FILE_INFO(i);
			if (fi->FiCpi != CPI_more) {
				continue;
			}
			if (ci->CiSpace + fi->FiSpace > driveMax) {
				more++;
				continue;
			}

			/*
			 * Isolate first directory component - the
			 * owner directory.
			 */
			p = fi->FiName + fi->FiOwner.path_l;
			pe = strchr(p, '/');
			if (pe == NULL) {
				/*
				 * No directory, use "." for owner.
				 */
				p = ".";
				l = 1;
			} else {
				l = Ptrdiff(pe, p);
			}
			if (*ci->CiOwner == '\0') {
				/*
				 * First name.
				 */
				if (l >= sizeof (ci->CiOwner)) {
					l = sizeof (ci->CiOwner) - 1;
				}
				memmove(ci->CiOwner, p, l);
				oLen = l;
			} else if (oLen != l ||
			    memcmp(p, ci->CiOwner, oLen) != 0) {
				/*
				 * Different owner.
				 */
				more++;
				continue;
			}
			fi->FiCpi = cpi;
			ci->CiFiles++;
			ci->CiSpace += fi->FiSpace;
			ci->CiMinSpace = min(ci->CiMinSpace, fi->FiSpace);
		}
		}
		break;

	case DM_ownerUidGid: {
		int	id;

		/*
		 * Select files for the same uid/gid.
		 */
		for (i = 0; i < ar->ArSelFiles; i++) {
			fi = LOC_FILE_INFO(i);
			if (fi->FiCpi != CPI_more) {
				continue;
			}
			if (ci->CiSpace + fi->FiSpace > driveMax) {
				more++;
				continue;
			}
			if (*ci->CiOwner == '\0') {
				/*
				 * First owner.
				 */
				id = fi->FiOwner.uid;
				if (as->AsReserve & RM_user) {
					struct passwd *p;

					if ((p = getpwuid(id)) != NULL) {
						strncpy(ci->CiOwner,
						    p->pw_name,
						    sizeof (ci->CiOwner)-1);
					} else {
						snprintf(ci->CiOwner,
						    sizeof (ci->CiOwner),
						    "U%d", id);
					}
				} else {
					struct group *g;

					if ((g = getgrgid(id)) != NULL) {
						strncpy(ci->CiOwner,
						    g->gr_name,
						    sizeof (ci->CiOwner)-1);
					} else {
						snprintf(ci->CiOwner,
						    sizeof (ci->CiOwner),
						    "G%d", id);
					}
				}
			} else if (id != fi->FiOwner.uid) {
				/*
				 * Different owner.
				 */
				more++;
				continue;
			}
			fi->FiCpi = cpi;
			ci->CiFiles++;
			ci->CiSpace += fi->FiSpace;
			ci->CiMinSpace = min(ci->CiMinSpace, fi->FiSpace);
		}
		}
		break;

	case DM_segment:
		/*
		 * Select segments such that they are striped
		 * to a copy instance.
		 */
		for (i = 0; i < ar->ArSelFiles; i++) {
			fi = LOC_FILE_INFO(i);
			if (fi->FiCpi != CPI_more ||
			    (fi->FiSegOrd % ar->ArDrivesUsed) != cpi) {
				continue;
			}
			if (ci->CiSpace + fi->FiSpace > driveMax) {
				more++;
				continue;
			}
			fi->FiCpi = cpi;
			ci->CiFiles++;
			ci->CiSpace += fi->FiSpace;
			ci->CiMinSpace = min(ci->CiMinSpace, fi->FiSpace);
		}
		break;
	}
	if (more != 0) {
		/*
		 * Did not examine all files.
		 */
		ci->CiFlags |= CI_more;
	}
}


/*
 * Organize files into tarballs.
 */
void
ComposeMakeTarballs(
	struct ArchReq *ar,
	int cpi,
	fsize_t archmax,
	fsize_t ovflmin,
	fsize_t volSpace)
{
	struct FileInfo *fi;
	fsize_t	space;
	fsize_t	tarballSpace;
	int	*fii;
	int	i;

	fii = LOC_FILE_INDEX(ar);

	/*
	 * Set the archive file boundaries.
	 */
	space = 0;
	tarballSpace = 0;
	if (ovflmin == 0) {
		ovflmin = FSIZE_MAX;
	}
	for (i = 0; i < ar->ArSelFiles; i++) {
		fi = LOC_FILE_INFO(i);
		if (fi->FiCpi != cpi || FI_IGNORE) {
			continue;
		}
		if (space + fi->FiSpace > volSpace) {
			if (fi->FiSpace >= ovflmin) {
				/*
				 * Assure that any file that is allowed to
				 * overflow is the only file in a tarball.
				 */
				fi->FiFlags |= FI_first;
				space = volSpace;
				i++;
			} else if (fi->FiSpace > volSpace) {
					/*
					 * Single file too big.
					 * Skip it for now.
					 */
					fi->FiCpi = CPI_later;
					continue;
			}
			break;
		} else if (tarballSpace + fi->FiSpace >= archmax) {
			fi->FiFlags |= FI_first;
			tarballSpace = 0;
		}
		space += fi->FiSpace;
		tarballSpace += fi->FiSpace;
	}
	ar->ArCpi[cpi].CiSpace = space;

	/*
	 * Save remainder for next time.
	 * This is belt and braces - all the selected files should fit.
	 */
	while (i < ar->ArSelFiles) {
		fi = LOC_FILE_INFO(i);
		i++;
		if (fi->FiCpi == cpi) {
			fi->FiCpi = CPI_later;
		}
	}
}


/*
 * Count non-stagable files.
 */
int	/* Count of non-stagable files */
ComposeNonStageFiles(
	struct ArchReq *ar)
{
	media_t	media;
	char	*vsn;
	int	*fii;
	int	i;
	int	nonStageFiles;
	int	nonStageVol;

	fii = LOC_FILE_INDEX(ar);
	ar->ArStageVols = 0;
	media = 0;
	nonStageFiles = 0;
	nonStageVol = 0;
	vsn = "";
	for (i = 0; i < ar->ArSelFiles; i++) {
		struct FileInfo *fi;
		struct OfflineInfo *oi;
		int	stgSim;

		fi = LOC_FILE_INFO(i);
		if (!(fi->FiFlags & FI_offline)) {
			continue;
		}
		fi->FiFlags &= ~FI_stagesim;

		/*
		 * Check offline file.
		 */
		oi = LOC_OFFLINE_INFO(fi);
		if (oi->OiMedia != media || strcmp(oi->OiVsn, vsn) != 0) {
			/*
			 * Different volume.
			 */
			ar->ArStageVols++;
			media = oi->OiMedia;
			vsn = oi->OiVsn;
			if (!IsVolumeAvailable(media, vsn, &stgSim)) {
				char	*mtype;

				mtype = sam_mediatoa(oi->OiMedia);
				/* Stage volume %s.%s not available */
				ArchReqMsg(HERE, ar, 4349, mtype, oi->OiVsn);
				if (!ArchReqCustMsgSent(ar, 4349)) {
					SendCustMsg(HERE, 4349, mtype,
					    oi->OiVsn);
				}
				nonStageVol = 1;
			} else {
				nonStageVol = 0;
			}
		}
		if (nonStageVol == 0) {
			fi->FiFlags |= stgSim;
		} else {
			fi->FiCpi = CPI_later;
		}
		nonStageFiles += nonStageVol;
	}
	return (nonStageFiles);
}


/*
 * Select files that fit a volume.
 */
void
ComposeSelectFit(
	struct ArchReq *ar,
	int cpi,
	fsize_t availSpace,
	fsize_t minSize)
{
	struct ArcopyInstance *ci;
	struct FileInfo *fi;
	int	*fii;
	int	i;
	int	more;

	fii = LOC_FILE_INDEX(ar);
	ci = &ar->ArCpi[cpi];
	ci->CiFiles = 0;
	ci->CiSpace = 0;
	more = 0;
	for (i = 0; i < ar->ArSelFiles; i++) {
		fi = LOC_FILE_INFO(i);
		if (fi->FiCpi != cpi) {
			continue;
		}
		if (fi->FiFileSize >= minSize &&
		    (ci->CiSpace + fi->FiSpace <= availSpace)) {
			ci->CiFiles++;
			ci->CiSpace += fi->FiSpace;
		} else {
			fi->FiCpi = CPI_more;
			more++;
		}
	}
	if (more != 0 && ar->ArDrivesUsed > 1) {
		ci->CiFlags |= CI_more;
	}
}


/*
 * Trace compose queue.
 */
void
ComposeTrace(void)
{
	QueueTrace(HERE, &composeQ);
#if defined(lint)
	Trace(TR_MISC, "%s %s %s ", OfflineCopies[0].EeName,
	    Reserves[0].RsName, Timeouts[0].EeName);
#endif /* defined(lint) */
}


/* Private functions. */


/*
 * Add a join file entry to the ArchReq.
 */
static struct ArchReq *
addJoinFile(
	struct ArchReq *ar,
	struct ArchSet *as,
	int start,
	int end)
{
	int (*compar) (const void *a1, const void *a2);
	size_t	size;
	int	*fii;
	int	count;

	/*
	 * Sort the files.
	 */
	switch (as->AsSort) {
	case SM_none:
		compar = cmp_order;
		break;
	case SM_age:
		compar = cmp_fmodtime;
		break;
	case SM_path:
		compar = NULL;
		break;
	case SM_priority:
		compar = cmp_fpriority;
		break;
	case SM_size:
		compar = cmp_fsize;
		break;
	case SM_rage:
		compar = cmp_rmodtime;
		break;
	case SM_rpath:
		compar = cmp_rpath;
		break;
	case SM_rpriority:
		compar = cmp_rpriority;
		break;
	case SM_rsize:
		compar = cmp_rsize;
		break;
	default:
		ASSERT(as->AsSort == SM_none);
		compar = NULL;
		break;
	}

	count = end - start;
	if (count > 1 && compar != NULL) {
		/*
		 * Sort file entries.
		 */
		qsort(&fiList[start], count, sizeof (struct FileInfo *),
		    compar);
	}
	if (start != 0) {
		fiList[start]->FiFlags |= FI_first;
	}

	/*
	 * Enter characteristics of first file in join file.
	 */
	fiJoin->FiName_l = strlen(fiJoin->FiName) + 1;
	fiJoin->FiModtime = fiList[start]->FiModtime;
	fiJoin->FiJoinStart = start;
	fiJoin->FiJoinCount = count;

	/*
	 * Assure room for a new entry in the ArchReq.
	 * The join file and its index.
	 */
	size = FI_SIZE(fiJoin) + sizeof (int);
	while (ar->ArSize + size >= ar->Ar.MfLen) {
		int	i;

		/* Convert pointers to indices. */
		for (i = 0; i < ar->ArFiles; i++) {
			fiList[i] =
			    (struct FileInfo *)(void *)Ptrdiff(fiList[i], ar);
		}
		ar = ArchReqGrow(ar);
		/* Convert indices to pointers. */
		for (i = 0; i < ar->ArFiles; i++) {
			fiList[i] =
			    (struct FileInfo *)(void *)((char *)ar +
			    (long)fiList[i]);
		}
	}
	ar->ArSize += size;
	size -= sizeof (int);

	/*
	 * Add join file index and move the newly enlarged file index up.
	 * Add join file entry to the ArchReq where the file index was.
	 */
	fii = LOC_FILE_INDEX(ar);
	ar->ArFileIndex += size;
	fii[ar->ArFiles] = Ptrdiff(fii, ar);
	ar->ArFiles++;
	memmove((char *)fii + size, fii, ar->ArFiles * sizeof (int));
	memmove(fii, fiJoin, size);

	/*
	 * Add the join file list entry.
	 */
	size = ar->ArFiles * sizeof (struct FileInfo *);
	if (size > fiListAlloc) {
		fiListAlloc = size;
		SamRealloc(fiList, fiListAlloc);
	}
	fiList[ar->ArFiles - 1] = (struct FileInfo *)(void *)fii;

	/*
	 * Reset join file.
	 */
	makeJoinFile();
	return (ar);
}


/*
 * Check offline files.
 * Offline files are examined to identify files that cannot be staged
 * because the volume may not be available.
 */
static void
checkOffline(
	struct ArchReq *ar)
{
	ArchReqMsg(HERE, ar, 4317);
	(void) makeFileInfoList(ar);
	qsort(fiList, ar->ArSelFiles, sizeof (struct FileInfo *), cmp_VSNs);
	while (ComposeNonStageFiles(ar) == ar->ArSelFiles) {
		/*
		 * All files are not stagable.
		 * Attempt to find an available volume.
		 */
		if (findStageVolume(ar) == FALSE) {
			ar->ArFlags |= AR_nonstage;
			return;
		}

		/*
		 * Resort volumes.
		 * We may have picked a completely different
		 * collection of volumes to stage from.
		 */
		qsort(fiList, ar->ArSelFiles, sizeof (struct FileInfo *),
		    cmp_VSNs);
	}
}


/*
 * Compare file modification times ascending.
 */
static int
cmp_fmodtime(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;

	if ((*f1)->FiModtime > (*f2)->FiModtime) {
		return (1);
	}
	if ((*f1)->FiModtime < (*f2)->FiModtime) {
		return (-1);
	}
	return (0);
}


/*
 * Compare file paths ascending.
 */
static int
cmp_fpath(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;
	int	icmp;

	icmp = strcmp((*f1)->FiName, (*f2)->FiName);
	if (icmp < 0) {
		return (-1);
	}
	if (icmp > 0) {
		return (1);
	}
	return (0);
}


/*
 * Compare file priorities descending.
 */
static int
cmp_fpriority(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;

	if ((*f2)->FiPriority < (*f1)->FiPriority) {
		return (-1);
	}
	if ((*f2)->FiPriority > (*f1)->FiPriority) {
		return (1);
	}
	return (0);
}


/*
 * Compare file sizes ascending.
 */
static int
cmp_fsize(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;

	if ((*f1)->FiFileSize > (*f2)->FiFileSize) {
		return (1);
	}
	if ((*f1)->FiFileSize < (*f2)->FiFileSize) {
		return (-1);
	}
	return (0);
}


/*
 * Compare file order ascending.
 */
static int
cmp_order(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;

	if (*f1 > *f2) {
		return (1);
	}
	if (*f1 < *f2) {
		return (-1);
	}
	return (0);
}


/*
 * Compare file modification times descending.
 */
static int
cmp_rmodtime(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;

	if ((*f1)->FiModtime > (*f2)->FiModtime) {
		return (-1);
	}
	if ((*f1)->FiModtime < (*f2)->FiModtime) {
		return (1);
	}
	return (0);
}


/*
 * Compare file paths descending.
 */
static int
cmp_rpath(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;
	int	icmp;

	icmp = strcmp((*f1)->FiName, (*f2)->FiName);
	if (icmp < 0) {
		return (1);
	}
	if (icmp > 0) {
		return (-1);
	}
	return (0);
}


/*
 * Compare file priorities ascending.
 */
static int
cmp_rpriority(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;

	if ((*f2)->FiPriority < (*f1)->FiPriority) {
		return (1);
	}
	if ((*f2)->FiPriority > (*f1)->FiPriority) {
		return (-1);
	}
	return (0);
}


/*
 * Compare file sizes descending.
 */
static int
cmp_rsize(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;

	if ((*f1)->FiFileSize > (*f2)->FiFileSize) {
		return (-1);
	}
	if ((*f1)->FiFileSize < (*f2)->FiFileSize) {
		return (1);
	}
	return (0);
}


/*
 * Compare segment order ascending.
 */
static int
cmp_segments(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;

	if ((*f1)->FiSegOrd > (*f2)->FiSegOrd) {
		return (1);
	}
	if ((*f1)->FiSegOrd < (*f2)->FiSegOrd) {
		return (-1);
	}
	return (0);
}


/*
 * Compare VSNs ascending.
 * Online files will sort first, and in original order.
 * Include position and offset in the compare to speed up staging.
 */
static int
cmp_VSNs(
	const void *p1,
	const void *p2)
{
	struct FileInfo **f1 = (struct FileInfo **)p1;
	struct FileInfo **f2 = (struct FileInfo **)p2;
	struct OfflineInfo *o1, *o2;
	int	icmp;

	if (!((*f1)->FiFlags & FI_offline) && !((*f2)->FiFlags & FI_offline)) {
		if (*f1 > *f2) {
			return (1);
		}
		if (*f1 < *f2) {
			return (-1);
		}
		return (0);
	}
	if (!((*f1)->FiFlags & FI_offline)) {
		return (-1);
	}
	if (!((*f2)->FiFlags & FI_offline)) {
		return (1);
	}

	o1 = LOC_OFFLINE_INFO(*f1);
	o2 = LOC_OFFLINE_INFO(*f2);
	icmp = strcmp(o1->OiVsn, o2->OiVsn);
	if (icmp < 0) {
		return (-1);
	}
	if (icmp > 0) {
		return (1);
	}
	if (o1->OiPosition > o2->OiPosition) {
		return (1);
	}
	if (o1->OiPosition < o2->OiPosition) {
		return (-1);
	}
	if (o1->OiOffset > o2->OiOffset) {
		return (1);
	}
	if (o1->OiOffset < o2->OiOffset) {
		return (-1);
	}
	return (0);
}


/*
 * Find volumes to stage files from.
 */
static boolean_t		/* TRUE if volumes found */
findStageVolume(
	struct ArchReq *ar)
{
	static struct sam_perm_inode pinode;
	static struct sam_ioctl_idstat idstatArgs = {
			{ 0, 0 }, sizeof (pinode), &pinode };
	struct FileSysEntry *fs;
	boolean_t found;
	int	fsFd;
	int	i;
	int	stgSim;

	/*
	 * Access the file system.
	 */
	for (i = 0; strcmp(FileSysTable->entry[i].FsName, ar->ArFsname) != 0;
	    i++) {
		if (i >= FileSysTable->count) {
			Trace(TR_MISC, "findStageVolume() fs %s not found",
			    ar->ArFsname);
			return (FALSE);
		}
	}
	fs = &FileSysTable->entry[i];
	if (!(fs->FsFlags & FS_mounted)) {
		Trace(TR_MISC, "findStageVolume() fs %s not mounted",
		    ar->ArFsname);
		return (FALSE);
	}
	if ((fsFd = OpenInodesFile(fs->FsMntPoint)) < 0) {
		Trace(TR_ERR, "open(%s)", fs->FsMntPoint);
		return (FALSE);
	}

	/*
	 * Examine the archived copies for each file to find
	 * an available volume to use for staging.
	 */
	found = FALSE;
	for (i = 0; i < ar->ArSelFiles; i++) {
		struct FileInfo *fi;
		struct OfflineInfo *oi;
		struct sam_disk_inode *dinode =
		    (struct sam_disk_inode *)&pinode;
		int	bit;
		int	offlineCopy;

		fi = fiList[i];
		oi = LOC_OFFLINE_INFO(fi);
		if (IsVolumeAvailable(oi->OiMedia, oi->OiVsn, &stgSim)) {
			found = TRUE;
			fi->FiFlags |= stgSim;
			continue;
		}

		/*
		 * idstat() the file to get a copy of the inode.
		 */
		idstatArgs.id = fi->FiId;
		if (ioctl(fsFd, F_IDSTAT, &idstatArgs) < 0) {
			if (errno != ENOENT) {
				Trace(TR_DEBUGERR, "stat(%d.%d, %s)",
				    idstatArgs.id.ino, idstatArgs.id.gen,
				    fi->FiName);
			}
			continue;
		}

		/*
		 * Check all copies for available volumes.
		 * Use first available volume.
		 * Skip damaged copies.
		 */
		for (offlineCopy = 0, bit = 1; offlineCopy < MAX_ARCHIVE;
		    offlineCopy++, bit <<= 1) {
			if ((dinode->arch_status & bit) &&
			    !(dinode->ar_flags[offlineCopy] & AR_damaged)) {
				if (IsVolumeAvailable(
				    dinode->media[offlineCopy],
				    pinode.ar.image[offlineCopy].vsn,
				    &stgSim)) {
					fi->FiFlags |= stgSim;
					break;
				}
			}
		}
		if (offlineCopy < MAX_ARCHIVE) {
			Trace(TR_ARDEBUG, "Unavailable %s %s", oi->OiVsn,
			    pinode.ar.image[offlineCopy].vsn);
			fi->FiStageCopy = (uchar_t)offlineCopy + 1;
			oi->OiPosition =
			    (fsize_t)pinode.ar.image[offlineCopy].position_u <<
			    32 | pinode.ar.image[offlineCopy].position;
			if (pinode.ar.image[offlineCopy].arch_flags &
			    SAR_size_block) {
				oi->OiOffset =
				    pinode.ar.image[offlineCopy].file_offset;
			} else {
				oi->OiOffset =
				    pinode.ar.image[offlineCopy].file_offset /
				    TAR_RECORDSIZE;
			}
			oi->OiMedia = dinode->media[offlineCopy];
			memmove(oi->OiVsn, pinode.ar.image[offlineCopy].vsn,
			    sizeof (oi->OiVsn));
			found = TRUE;
		}
	}
	(void) close(fsFd);
	return (found);
}


/*
 * Join files into archive files.
 * Sort the files using the join method property as the key.  This
 * collects the files with the same property together.
 * Add 'join' files that encapsulate the joined files.
 * Sort the files within the archive files.
 */
static struct ArchReq *
joinFiles(
	struct ArchReq *ar,
	struct ArchSet *as)
{
	struct FileInfo *fi;
	int	joins;
	int	i;
	int	start;

	ArchReqMsg(HERE, ar, 4316);
	makeFileInfoList(ar);
	makeJoinFile();
	joins = 0;
	start = 0;

	switch (as->AsJoin) {

	/*
	 * Join file entries with matching paths.
	 */
	case JM_path: {
		char	*pFirst;
		int	pLen;

		/*
		 * Sort file entries by path to get matching paths together.
		 */
		qsort(fiList, ar->ArSelFiles, sizeof (struct FileInfo *),
		    cmp_fpath);

		/*
		 * Step through file entries while paths match.
		 */
		pFirst = "";
		pLen = 1;
		for (i = 0; i < ar->ArSelFiles; i++) {
			char	*p, *pe;
			int	l;

			fi = fiList[i];
			/* Isolate path. */
			p = fi->FiName;
			pe = strrchr(p, '/');
			if (pe == NULL) {
				continue;
			}
			l = Ptrdiff(pe, p);
			if (l != pLen || strncmp(p, pFirst, l) != 0) {
				/*
				 * Paths do not match.  Enter join file.
				 */
				if (*pFirst != '\0') {
					memmove(fiJoin->FiName, pFirst, pLen);
					ar = addJoinFile(ar, as, start, i);
					joins++;
					start = i;
					/* ArchReq may have moved. */
					fi = fiList[i];
					p = fi->FiName;
				}
				pFirst = p;
				pLen = l;
			}
			/*
			 * Include file characteristics in join file.
			 */
			if (fi->FiFlags & FI_offline) {
				if (!(fiJoin->FiFlags & FI_offline)) {
					/*
					 * Enter offline information from
					 * first offline file.
					 */
					memmove(LOC_OFFLINE_INFO(fiJoin),
					    LOC_OFFLINE_INFO(fi),
					    sizeof (struct OfflineInfo));
					fiJoin->FiFlags |= FI_offline;
					fiJoin->FiStatus |= FIF_offline;
				}
			}
			fiJoin->FiFileSize +=
			    fi->FiFileSize;
			fiJoin->FiPriority =
			    max(fiJoin->FiPriority, fi->FiPriority);
			fiJoin->FiSpace +=
			    fi->FiSpace;
		}
		memmove(fiJoin->FiName, pFirst, pLen);
		ar = addJoinFile(ar, as, start, i);
		joins++;
		}
		break;

	default:
		ASSERT(as->AsJoin == JM_none);
		return (ar);
	}

	ar->ArSelFiles = joins;
	ar->ArFlags |= AR_join;
	Trace(TR_MISC, "%s joined by %s", arname, Joins[as->AsJoin].EeName);
	if (as->AsSort != SM_none) {
		if (as->AsSort < SM_rage) {
			Trace(TR_MISC, "%s sorted by %s",
			    arname, Sorts[as->AsSort].EeName);
		} else {
			Trace(TR_MISC, "%s reverse sorted by %s",
			    arname, Rsorts[as->AsSort-SM_rage+1].EeName);
		}
	}
	return (ar);
}


/*
 * Make a list of FileInfo pointers.
 * This list is used for performing ArchReq joins, sorts and separations.
 * The actual list is private to this module.  Its size will be increased
 * to hold the required list.
 */
static void
makeFileInfoList(
	struct ArchReq *ar)			/* ArchReq to use */
{
	size_t	size;
	int	*fii;
	int	i;

	fii = LOC_FILE_INDEX(ar);
	if (arFiList != NULL) {
		return;
	}
	arFiList = ar;
	size = ar->ArSelFiles * sizeof (struct FileInfo *);
	if (size > fiListAlloc) {
		fiListAlloc = size;
		SamRealloc(fiList, fiListAlloc);
	}
	for (i = 0; i < ar->ArSelFiles; i++) {
		fiList[i] = LOC_FILE_INFO(i);
	}
}


/*
 * Make the join file.
 * Other file characteristics are entered from the joined files.
 */
static void
makeJoinFile(void)
{
	size_t size;

	size = STRUCT_RND(sizeof (struct FileInfo)) +
	    STRUCT_RND(MAXPATHLEN + 4) +
	    STRUCT_RND(sizeof (struct OfflineInfo));
	if (fiJoin == NULL) {
		SamMalloc(fiJoin, size);
	}
	memset(fiJoin, 0, size);
	fiJoin->FiFlags = FI_join;
	fiJoin->FiType = 'J';
}


/*
 * Prepare an ArchReq.
 */
static void
prepareArchReq(
	struct ArchReq *ar)			/* ArchReq to prepare */
{
	int	*fii;
	int	i;
	int	j;

	fii = LOC_FILE_INDEX(ar);
	if (arFiList != NULL) {
		/*
		 * Copy file list entries to file index.
		 */
		i = 0;
		if (ar->ArFlags & AR_join) {
			/*
			 * Place joins first.
			 */
			for (j = ar->ArFiles - ar->ArSelFiles; j < ar->ArFiles;
			    j++) {
				struct FileInfo *fi;

				fi = fiList[j];
				fi->FiJoinStart += ar->ArSelFiles;
				fii[i++] = Ptrdiff(fi, ar);
			}
		}
		j = 0;
		while (i < ar->ArFiles) {
			fii[i++] = Ptrdiff(fiList[j], ar);
			j++;
		}
	}

	/*
	 * Step through archive request and compute archive request information.
	 */
	ar->ArMinSpace = FSIZE_MAX;
	ar->ArPriority = PR_MIN;
	ar->ArSelSpace = 0;
	ar->ArFlags &= ~(AR_offline | AR_segment);
	for (i = 0; i < ar->ArSelFiles; i++) {
		struct FileInfo *fi;

		fi = LOC_FILE_INFO(i);
		if (fi->FiCpi >= 0) {
			ar->ArMinSpace = min(ar->ArMinSpace, fi->FiSpace);
			ar->ArPriority = max(ar->ArPriority, fi->FiPriority);
			ar->ArSelSpace += fi->FiSpace;
			if (fi->FiFlags & FI_offline) {
				ar->ArFlags |= AR_offline;
			}
			if (fi->FiStatus & FIF_segment) {
				ar->ArFlags |= AR_segment;
			}
		}
	}
}


/*
 * Prune an ArchReq.
 * Remove archived files.
 */
static void
pruneArchReq(
	struct ArchReq *ar)			/* ArchReq to prune */
{
	int	*fii;
	int	i, j;

	fii = LOC_FILE_INDEX(ar);
	if (ar->ArFlags & AR_join) {
		int	fiIndex;

		/*
		 * Remove join files.
		 * Find first join file, they may have been sorted.
		 */
		fiIndex = INT_MAX;
		for (i = 0; i < ar->ArSelFiles; i++) {
			fiIndex = min(fii[i], fiIndex);
		}

		/*
		 * Move file index down.
		 */
		ar->ArFiles -= ar->ArSelFiles;
		ar->ArFileIndex = fiIndex;
		memmove((char *)ar + fiIndex, &fii[ar->ArSelFiles],
		    ar->ArFiles * sizeof (int));
		fii = LOC_FILE_INDEX(ar);
	}

	/*
	 * Step through archive request and remove file index entries for
	 * files that have been archived, have errors, or have been removed.
	 */
	ar->ArSpace = 0;
	for (i = 0, j = 0; i < ar->ArFiles; i++) {
		struct FileInfo *fi;

		fi = LOC_FILE_INFO(i);
		fi->FiCpi = 0;
		fi->FiFlags &= ~(FI_first | FI_stagesim);
		if (!FI_IGNORE && !(fi->FiStatus & FIF_remove)) {
			ar->ArSpace += fi->FiSpace;
			fii[j] = fii[i];
			j++;
		}
	}
	ar->ArFiles = j;
}


/*
 * Sort files.
 */
static void
sortFiles(
	struct ArchReq *ar,
	struct ArchSet *as)
{
	int (*compar) (const void *a1, const void *a2);

	if (ar->ArSelFiles == 0) {
		return;
	}

	/*
	 * Sort the file list.
	 */
	ArchReqMsg(HERE, ar, 4319);
	makeFileInfoList(ar);
	switch (as->AsSort) {
	case SM_age:
		compar = cmp_fmodtime;
		break;
	case SM_path:
		compar = cmp_fpath;
		break;
	case SM_priority:
		compar = cmp_fpriority;
		break;
	case SM_size:
		compar = cmp_fsize;
		break;
	case SM_rage:
		compar = cmp_rmodtime;
		break;
	case SM_rpath:
		compar = cmp_rpath;
		break;
	case SM_rpriority:
		compar = cmp_rpriority;
		break;
	case SM_rsize:
		compar = cmp_rsize;
		break;
	default:
		ASSERT(as->AsSort == SM_none);
		return;
	}
	qsort(fiList, ar->ArSelFiles, sizeof (struct FileInfo *), compar);
	if (as->AsSort < SM_rage) {
		Trace(TR_MISC, "%s sorted by %s",
		    arname, Sorts[as->AsSort].EeName);
	} else {
		Trace(TR_MISC, "%s reverse sorted by %s", arname,
		    Rsorts[as->AsSort-SM_rage+1].EeName);
	}
}


/*
 * Sort segments.
 */
static void
sortSegments(
	struct ArchReq *ar)
{
	ArchReqMsg(HERE, ar, 4317);
	(void) makeFileInfoList(ar);
	qsort(fiList, ar->ArSelFiles, sizeof (struct FileInfo *), cmp_segments);
}


/*
 * Wakeup the Compose() thread.
 */
void
wakeup(void)
{
	PthreadMutexLock(&composeWaitMutex);
	PthreadCondSignal(&composeWait);
	PthreadMutexUnlock(&composeWaitMutex);
}
