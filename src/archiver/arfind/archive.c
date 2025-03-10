/*
 * archive.c - process archive requests.
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

#pragma ident "$Revision: 1.126 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* ARCH_TRACE	If defined, turn on DEBUG traces for ArchReqs processed */
/* FILE_TRACE	If defined, turn on DEBUG traces for files processed */
#if defined(DEBUG)
#define	FILE_TRACE
#define	ARCH_TRACE
#endif

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/uioctl.h"
#include "aml/tar.h"
#include "sam/fs/ino.h"

/* Local headers. */
#include "arfind.h"
#include "dir_inode.h"

/* Private data. */

/*
 * archReqTable - ArchReqs attached to arfind.
 * The first entry is the "null" ArchReq.
 */
static struct ArchReq **archReqTable;
static int archReqTableCount;

#define	ARCH_REQ_TABLE_INCR 10

/*
 * arCreates - Table of the ArchReqs being created for each Archive Set.
 * Entry is the index in the archReqsTable indexed by ArchSet number.
 */
static int *arCreates = NULL;
static int arCreatesNum = 0;


/*
 * idList -  List of the location of all inode ids in ArchReqs.
 * Each file in an ArchReq is referenced by an entry in the idList.
 * An idList entry "points" to a sam_id_t struct in the ArchReq using the
 * index of the ArchReq in the archReqTable.
 *
 * The idList section is sorted by inode number.  It is searched using a
 * binary search.
 */

/* Inode reference. */
struct IdRef {
	uint_t  IrAtIndex;
	uint_t  IrOffset;
};

static struct IdList {
	MappedFile_t Il;
	size_t	IlSize;				/* Size of all entries */
	int	IlCount;			/* Number of entries */
	struct IdRef IlEntry[1];
} *idList[4];			/* One for each copy */

#define	IDLIST "idlist_copy"
#define	IDLIST_MAGIC 012203101104
#define	IDLIST_INCR 1000
#define	ID_LOC(ir) ((void *) \
	((char *)(void *)archReqTable[(ir)->IrAtIndex] + (ir)->IrOffset))

static pthread_mutex_t archReqMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t archiveWait = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t archiveWaitMutex = PTHREAD_MUTEX_INITIALIZER;
static int archiveWaitCount = 0;	/* Count of ArchiveRun()-s */

static boolean_t doTrace = FALSE;
static boolean_t startRequests = FALSE;
static char arname[ARCHREQ_NAME_SIZE];

/* Private functions. */
static void addArchReq(struct ArchReq *ar);
static void archiveTrace(void);
static char *archReqName(struct ArchReq *ar, char *arname);
static struct ArchReq *checkQueue(int asn);
static int cmp_ino(const void *p1, const void *p2);
static struct ArchReq *createArchReq(struct ArchSet *as, int copy);
static void deleteArchReq(struct ArchReq *ar);
static boolean_t dequeueArchReq(struct ArchReq *ar);
static struct ArchReq *growArchReq(struct ArchReq *ar, int incr);
static struct IdRef *idListAdd(sam_id_t id, int copy);
static void idListCompress(int copy);
static void idListDelete(sam_id_t id, int copy, struct ArchReq *ar);
static struct IdRef *idListLookup(sam_id_t id, int copy);
static void initArchive(void);
static struct ArchReq *pruneArchReq(struct ArchReq *ar);
static struct ArchReq *queueArchReq(struct ArchReq *ar);
static boolean_t recoverAnArchReq(struct ArchReq *ar, char *arname);
static void recoverArchReqs(void);
static void setStartValues(struct ArchReq *ar, struct ArchSet *as);
static struct ArchReq *updateParams(struct ArchReq *ar, struct ArchSet *as);


/*
 * Thread - Manage ArchReq-s.
 */
void *
Archive(
	void *arg)
{
	time_t archiveTime;	/* Time at which to start archival */

	initArchive();
	ThreadsInitWait(ArchiveRun, ArchiveRun);

	while (Exec == ES_init) {
		ThreadsSleep(1);
	}
	recoverArchReqs();
	PthreadMutexUnlock(&archReqMutex);

	/*
	 * Wait for ArchReq-s that are ready.
	 * We can be awakened by the condition signal.
	 */
	archiveTime = time(NULL);
	while (Exec < ES_term) {
		time_t	timeNow;
		int	fileCountCreate;
		int	fileCountSchedule;
		int	fileCountArchive;
		int	ari;

		PthreadMutexLock(&archiveWaitMutex);
		while (archiveWaitCount <= 0) {
			char ts[ISO_STR_FROM_TIME_BUF_SIZE];

			Trace(TR_ARDEBUG, "Waiting until %s",
			    TimeToIsoStr(archiveTime, ts));
			ThreadsCondTimedWait(&archiveWait, &archiveWaitMutex,
			    archiveTime);
			archiveWaitCount++;
		}
		archiveWaitCount = 0;
		PthreadMutexUnlock(&archiveWaitMutex);
		timeNow = time(NULL);
		archiveTime = timeNow + WAIT_TIME;
		ThreadsReconfigSync(RC_wait);
		PthreadMutexLock(&archReqMutex);
		if (doTrace) {
			doTrace = FALSE;
			archiveTrace();
		}

		/*
		 * Check all ArchReqs.
		 */
		fileCountCreate = 0;
		fileCountSchedule = 0;
		fileCountArchive = 0;
		for (ari = 1; ari < archReqTableCount; ari++) {
			struct ArchReq *ar;

			ar = archReqTable[ari];
			if (ar == NULL) {
				continue;
			}
			if (!ArchReqValid(ar)) {
				/*
				 * Get rid of the invalid ArchReq.
				 */
				ar->Ar.MfValid = 0;
				deleteArchReq(ar);
				continue;
			}
			(void) archReqName(ar, arname);

			switch (ar->ArState) {

			case ARS_create: {
				if (ar->ArStatus & (ARF_changed | ARF_rmreq)) {
					/*
					 * ArchReq has changed - file(s) have
					 * been removed, or need to be removed.
					 */
					Trace(TR_QUEUE, "Archreq prune (%s)",
					    arname);
					ar = pruneArchReq(ar);
					if (ar == NULL) {
						continue;
					}
				}

				fileCountCreate += ar->ArCount;
				if (Exec != ES_run ||
				    ar->ArAsn <= MAX_ARCHIVE) {
					/*
					 * Do not queue any ArchReqs if not
					 * running.  Do not queue any allsets
					 * ArchReqs.
					 */
					continue;
				}
				if (startRequests && ar->ArCount != 0) {
					ar->ArStartTime = timeNow;
				}

				if (!(ar->ArStatus & ARF_merge) &&
				    (ar->ArCount >= ar->ArStartCount ||
				    ar->ArSpace >= ar->ArStartSize ||
				    timeNow >= ar->ArStartTime)) {

					struct ArchReq *arS;
					char arSname[ARCHREQ_NAME_SIZE];

					/*
					 * Conditions have been met for
					 * archival to start.  Check for
					 * an ArchReq already scheduled.
					 * Don't dequeue a full archreq
					 * for merging.
					 */
					arS = NULL;
					if (!(ar->ArStatus & ARF_full)) {
						arS = checkQueue(ar->ArAsn);
					}
					if (arS != NULL) {
						/*
						 * ArchReq scheduled.
						 * Dequeue it for merging.
						 */
						(void) archReqName(arS,
						    arSname);
						Trace(TR_QUEUE,
						    "Archreq mergeS(%s)",
						    arSname);
						arS->ArStatus |= ARF_merge;

						if (dequeueArchReq(arS)) {
							ar->ArStatus |=
							    ARF_merge;
							Trace(TR_QUEUE,
							    "Archreq merge "
							    "(%s)", arname);
							break;
						} else {
							/*
							 * Not dequeued.
							 * Probably archiving.
							 */
							arS->ArStatus &=
							    ~ARF_merge;
							if (arS->ArStatus !=
							    ARS_archive) {
								break;
							}
						}
					}

					/*
					 * Send the ArchReq to the archiver
					 * daemon for scheduling.
					 */
					Trace(TR_QUEUE,
					    "Archreq start (%s) count: %d",
					    arname, ar->ArCount);

					ar = queueArchReq(ar);
				} else {
					archiveTime = min(archiveTime,
					    ar->ArStartTime);
				}
				}
				break;

			case ARS_schedule:
				fileCountSchedule += ar->ArCount;
				if (ar->ArStatus & (ARF_changed | ARF_rmreq)) {
					/*
					 * ArchReq has changed - file(s) have
					 * been removed, or need to be removed.
					 */
					Trace(TR_QUEUE, "Archreq changes (%s)",
					    arname);
					(void) dequeueArchReq(ar);
				}
				if (ar->ArStatus & ARF_recover) {
					ar = queueArchReq(ar);
				}
				break;

			case ARS_archive:
				fileCountArchive += ar->ArCount;
				if (ar->ArStatus & ARF_rmreq) {
					/*
					 * We need to remove the ArchReq.
					 */
					Trace(TR_QUEUE, "Archreq remove (%s)",
					    arname);
					(void) dequeueArchReq(ar);
				}
				if (ar->ArStatus & ARF_recover) {
					ar = queueArchReq(ar);
				}
				break;

			case ARS_done:
				Trace(TR_QUEUE, "Archreq done (%s)", arname);
				ar = pruneArchReq(ar);
				if (ar != NULL && ar->ArState == ARS_done) {
					/*
					 * ArchReq has files that still
					 * need to be archived.
					 */
					ar->ArState = ARS_create;
					ar->ArStartTime = timeNow;
					ar = queueArchReq(ar);
				}
				break;

			default:
				break;
			}
		}
		PthreadMutexUnlock(&archReqMutex);
		startRequests = FALSE;
		ThreadsReconfigSync(RC_allow);
		State->AfFilesCreate = fileCountCreate;
		State->AfFilesSchedule = fileCountSchedule;
		State->AfFilesArchive = fileCountArchive;
	}

	ThreadsExit();
	/*NOTREACHED*/
	return (arg);
}


/*
 * Add a file entry to an ArchReq.
 * RETURN: 0 if file added, -1 if not.
 */
int
ArchiveAddFile(
	struct PathBuffer *pb,
	struct sam_perm_inode *pinode,
	struct FilePropsEntry *fp,
	int copy,
	int age,
	int copiesReq,
	boolean_t unarch)
{
	static char arname[ARCHREQ_NAME_SIZE];
	static char ts[ISO_STR_FROM_TIME_BUF_SIZE];
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	struct IdRef *ir;
	struct FileInfo *fi;
	struct ArchReq *ar;
	struct ArchSet *as;
	int	fiSize;
	double	priority;
	int	an;
	int	asn;
	int	bit;
	int	copyBit;
	int	i;
	int	name_l;
	int	offlineCopy = 0;
	int	retval;

	retval = 0;
	an = copy;
	if (dinode->ar_flags[copy] & AR_rearch) {
		an += MAX_ARCHIVE;
	}
	asn = fp->FpAsn[an];
	as = &ArchSetTable[asn];
	if (dinode->status.b.offline) {
		offlineCopy = -1;
		if (dinode->ar_flags[copy] & AR_rearch) {
			if (as->AsFlags & AS_rearchstc) {
				offlineCopy = as->AsRearchStageCopy - 1;
			} else if (!(dinode->ar_flags[copy] & AR_damaged)) {
				offlineCopy = copy;
			}
		}
		if (offlineCopy == -1) {
			/*
			 * Find first copy to stage from.
			 */
			for (offlineCopy = 0, bit = 1; /* Terminated inside */;
			    offlineCopy++, bit <<= 1) {
				if (offlineCopy >= MAX_ARCHIVE) {
					/*
					 * No copy to stage from.
					 */
					return (-1);
				}
				if (dinode->arch_status & bit &&
				    !(dinode->ar_flags[offlineCopy] &
				    AR_damaged)) {
					break;
				}
			}
		}
	}

	/*
	 * Compute size of entry.
	 */
	name_l = Ptrdiff(pb->PbEnd, pb->PbPath);
	if (name_l == 0) {
		name_l = 2;
	} else {
		name_l++;
	}
	fiSize = STRUCT_RND(sizeof (struct FileInfo)) + STRUCT_RND(name_l);
	if (dinode->status.b.offline) {
		fiSize += STRUCT_RND(sizeof (struct OfflineInfo));
	}

	PthreadMutexLock(&archReqMutex);

	/*
	 * Make the filelist entry for the file.
	 * See if it is in an ArchReq.
	 */
	ir = idListAdd(dinode->id, copy);
	if (ir->IrAtIndex != 0) {
		ar = archReqTable[ir->IrAtIndex];
		fi = (struct FileInfo *)ID_LOC(ir);
		if (fi->FiId.gen >= dinode->id.gen) {
#if defined(FILE_TRACE)
			Trace(TR_DEBUG,
			    "ArchReq '%s' file exists inode: %d.%d (%s)",
			    archReqName(ar, arname),
			    dinode->id.ino, dinode->id.gen, pb->PbPath);
#endif
			goto out;
		}
	}

	ar = archReqTable[arCreates[asn]];
	if (ar == NULL) {
		/*
		 * New ArchReq needed.
		 */
		ar = createArchReq(as, copy);
	}
	if (ar->ArSize + fiSize >= ar->Ar.MfLen) {
		ar = growArchReq(ar, fiSize);
	}
#if defined(FILE_TRACE)
	Trace(TR_DEBUG, "Archreq add (%s) inode: %d.%d (%s) mod: %s",
	    archReqName(ar, arname),
	    dinode->id.ino, dinode->id.gen, pb->PbPath,
	    TimeToIsoStr(dinode->modify_time.tv_sec, ts));
#endif /* defined(FILE_TRACE) */

	/*
	 * Construct the FileInfo entry.
	 */
	fi = (struct FileInfo *)(void *)((char *)ar + ar->ArSize);
	ASSERT((char *)fi + fiSize < (char *)ar + ar->Ar.MfLen);

	copyBit = 1 << copy;
	if (*pb->PbPath != '\0') {
		memmove(fi->FiName, pb->PbPath, name_l);
	} else {
		*fi->FiName = '.';
		*(fi->FiName + 1) = '\0';
	}
	fi->FiId = dinode->id;
	fi->FiModtime = dinode->modify_time.tv_sec;
	fi->FiName_l = (ushort_t)name_l;

	/*
	 * Copy release controls.
	 */
	if (S_ISREG(dinode->mode)) {
		fi->FiCopiesNorel = fp->FpCopiesNorel;
		fi->FiCopiesRel	= fp->FpCopiesRel;
	} else {
		fi->FiCopiesNorel = 0;
		fi->FiCopiesRel	= 0;
	}
	fi->FiCopiesReq = (uchar_t)copiesReq;

	/*
	 * Flags.
	 */
	fi->FiFlags	= 0;
	fi->FiStatus = unarch ? FIF_exam : 0;
	if (S_ISSEGS(dinode)) {
		fi->FiStatus |= FIF_segment;
		fi->FiSegOrd = dinode->rm.info.dk.seg.ord;
	}
	if ((dinode->arch_status & copyBit) &&
	    (dinode->ar_flags[copy] & AR_rearch)) {
		fi->FiStatus |= FIF_rearch;
	}

	/*
	 * VSN reservation owner.
	 */
	if (as->AsReserve & RM_owner) {
		if (as->AsReserve & RM_dir) {
			fi->FiOwner.path_l = fp->FpPathSize;
			if (fi->FiOwner.path_l != 0) {
				fi->FiOwner.path_l++;
			}
		} else if (as->AsReserve & RM_user) {
			fi->FiOwner.uid = dinode->uid;
		} else if (as->AsReserve & RM_group) {
			fi->FiOwner.gid = dinode->gid;
		}
	}

	/*
	 * Set size of file.
	 * Size of file depends on file type.
	 * Set the file type (sfind character).
	 */
	if (S_ISLNK(dinode->mode)) {
		fi->FiFileSize = MAXPATHLEN;
		fi->FiType = 'l';
	} else if (S_ISREQ(dinode->mode)) {
		fi->FiFileSize = sizeof (struct sam_resource_file);
		fi->FiType = 'R';
	} else if (S_ISSEGI(dinode)) {
		fi->FiFileSize = dinode->rm.info.dk.seg.fsize;
		fi->FiType = 'I';
	} else {
		fi->FiFileSize = dinode->rm.size;
		if (S_ISDIR(dinode->mode)) {
			fi->FiType = 'd';
		} else if (S_ISSEGS(dinode)) {
			fi->FiType = 'S';
		} else if (S_ISREG(dinode->mode)) {
			fi->FiType = 'f';
			if ((as->AsDbgFlags & ASDBG_simread) &&
			    fi->FiFileSize == 0) {
				fsize_t	v;
				char	*name;

				/*
				 * Set pseudo file size from file name.  arcopy
				 * will do a simulated read from /dev/zero.
				 */
				name = strrchr(fi->FiName, '/');
				if (name != NULL &&
				    StrToFsize(name + 1, &v) != -1) {
					fi->FiFileSize = v;
					fi->FiStatus |= FIF_simread;
				}
			}
		} else if (S_ISBLK(dinode->mode)) {
			fi->FiType = 'b';
		} else {
			fi->FiType = '?';
		}
	}

	/*
	 * Set space required for tar file.
	 */
	fi->FiSpace = fi->FiFileSize + TAR_RECORDSIZE - 1;
	fi->FiSpace = (fi->FiSpace / TAR_RECORDSIZE) * TAR_RECORDSIZE;

	/* Add size of tar header. */
	fi->FiSpace += TAR_RECORDSIZE;

	/* Adjust for length of file name. */
	if (name_l > NAMSIZ) {
		fi->FiSpace += TAR_RECORDSIZE;	/* The long name header. */
		fi->FiSpace +=
		    ((name_l + TAR_RECORDSIZE - 1) / TAR_RECORDSIZE) *
		    TAR_RECORDSIZE;
	}

	/*
	 * Add info for offline file at end of file name.
	 */
	if (dinode->status.b.offline) {
		struct OfflineInfo *oi;

		fi->FiStatus |= FIF_offline;
		fi->FiFlags |= FI_offline;
		fi->FiStageCopy = (uchar_t)offlineCopy + 1;

		oi = LOC_OFFLINE_INFO(fi);
		oi->OiPosition =
		    (fsize_t)pinode->ar.image[offlineCopy].position_u << 32 |
		    pinode->ar.image[offlineCopy].position;
		if (pinode->ar.image[offlineCopy].arch_flags & SAR_size_block) {
			oi->OiOffset =
			    pinode->ar.image[offlineCopy].file_offset;
		} else {
			oi->OiOffset =
			    pinode->ar.image[offlineCopy].file_offset /
			    TAR_RECORDSIZE;
		}
		oi->OiMedia = dinode->media[offlineCopy];
		memmove(oi->OiVsn, pinode->ar.image[offlineCopy].vsn,
		    sizeof (oi->OiVsn));
	}

	/*
	 * Calculate archive priority.
	 */
	switch (copy) {
	case 0:
		priority = as->AsPrC1;
		break;
	case 1:
		priority = as->AsPrC2;
		break;
	case 2:
		priority = as->AsPrC3;
		break;
	case 3:
		priority = as->AsPrC4;
		break;
	default:
		priority = 0;
		break;
	}

	/* Count copies. */
	for (i = 0, bit = 1; i < MAX_ARCHIVE; i++, bit <<= 1) {
		if (i == copy) {
			continue;
		}
		if (dinode->arch_status & bit) {
			priority += as->AsPrCopies;
		}
	}
	if (fi->FiStatus & FIF_rearch) {
		priority += as->AsPrRearch;
	}
	if (dinode->ar_flags[copy] & AR_arch_i) {
		time_t	start;

		priority += as->AsPrArch_im;
		ar->ArStatus |= ARF_start;
		start = TIME_NOW(dinode) + EPSILON_TIME;
		if (ar->ArStartTime > start) {
			ar->ArStartTime = start;
			(void) TimeToIsoStr(ar->ArStartTime, ts);
			/* Start archive at %s */
			ArchReqMsg(HERE, ar, 4355, ts);
		}
	}
	if (fp->FpCopiesNorel & copyBit) {
		priority += as->AsPrReqrel;
	}
	priority += as->AsPrAge * age;
	priority += as->AsPrSize * dinode->rm.size;

	priority = min(priority, PR_MAX);
	priority = max(priority, PR_MIN);
	fi->FiPriority = priority;

	/*
	 * Update the idList entry.
	 */
	ir->IrAtIndex = ar->ArAtIndex;
	ir->IrOffset = Ptrdiff(fi, ar);

	/*
	 * Update ArchReq information.
	 */
	ar->ArCount++;
	ar->ArSize += fiSize;
	ar->ArSpace += fi->FiSpace;
	ar->ArMinSpace = min(ar->ArMinSpace, fi->FiSpace);
	ar->ArPriority = max(ar->ArPriority, fi->FiPriority);
	if (fi->FiStatus & FIF_offline) {
		ar->ArFlags |= AR_offline;
	}
	if (fi->FiStatus & FIF_segment) {
		ar->ArFlags |= AR_segment;
	}
	if (fi->FiStatus & FIF_rearch) {
		ar->ArStatus |= ARF_rearch;
	}

	if (ar->ArStatus & ARF_start && !(ar->ArStatus & ARF_merge)) {
		/*
		 * Check start archive conditions.
		 */
		if (ar->ArCount >= ar->ArStartCount ||
		    ar->ArSpace >= ar->ArStartSize ||
		    TIME_NOW(dinode) >= ar->ArStartTime) {
			/*
			 * Start archive conditions met.
			 */
			ar->ArStatus &= ~ARF_start;
			ArchiveRun();
		}
	}

out:
	PthreadMutexUnlock(&archReqMutex);
	return (retval);
}


/*
 * Reconfigure.
 */
void
ArchiveReconfig(void)
{
	int	ari;

	if (ArchSetFile == NULL) {
		return;
	}
	PthreadMutexLock(&archReqMutex);
	if (arCreates == NULL || ArchSetMap != NULL) {
		int *oldArCreates;

		oldArCreates = arCreates;

		/*
		 * Make the ArCreates list.
		 */
		SamMalloc(arCreates, ArchSetNumof * sizeof (int *));
		memset(arCreates, 0, ArchSetNumof * sizeof (int *));
		if (oldArCreates != NULL) {
			int	i;
			/*
			 * Copy old arCreates entries.
			 */
			for (i = 0; i < arCreatesNum; i++) {
				int	j;

				j = ArchSetMap[i];
				if (j != -1) {
					arCreates[j] = oldArCreates[i];
				} else if (oldArCreates[i] != 0) {
					struct ArchReq *ar;

					/*
					 * Remove this ArchReq.
					 */
					ar = archReqTable[oldArCreates[i]];
					ar->ArStatus |= ARF_rmreq;
				}
			}
			SamFree(oldArCreates);
		}
		arCreatesNum = ArchSetNumof;
	}

	for (ari = 1; ari < archReqTableCount; ari++) {
		struct ArchReq *ar;

		ar = archReqTable[ari];
		if (ar == NULL ||
		    ar->ArCount == 0 ||
		    ar->ArState != ARS_create ||
		    (ar->ArStatus & ARF_rmreq)) {
			continue;
		}
		if (ar->ArAsn < MAX_AR) {
			ar->ArStatus |= ARF_rmreq;
		} else {
			ar = updateParams(ar, &ArchSetTable[ar->ArAsn]);
		}
	}
	PthreadMutexUnlock(&archReqMutex);
	ArchiveRun();
}


/*
 * Remove ArchReq(s).
 */
int
ArchiveRmArchReq(
	char *arname)
{
	set_name_t asname;
	int	ari;
	int	msgnum;
	int	seqnum = 0;

	Trace(TR_MISC, "Archreq remove (%s)", arname);

	/*
	 * Parse the argument.
	 */
	if (strcmp(arname, "*") == 0) {
		strcpy(asname, arname);
		msgnum = 0;
	} else {
		char	*p;
		int	l;

		/*
		 * Isolate the Archive Set name.
		 */
		p = strrchr(arname, '.');
		if (p == NULL) {
			/* Invalid ArchReq - %s */
			return (4905);
		}
		l = Ptrdiff(p, arname);
		if (l > sizeof (asname)) {
			/* Invalid ArchReq - %s */
			return (4905);
		}
		strncpy(asname, arname, sizeof (asname));
		*(asname + l) = '\0';

		if (strcmp(p+1, "*") == 0) {
			/*
			 * All ArchReqs for the Archive Set.
			 */
			seqnum = -1;
		} else {
			/*
			 * Get the sequence number.
			 */
			errno = 0;
			seqnum = strtol(p + 1, &p, 10);
			if (*p != '\0' || errno != 0 || seqnum < 0) {
				/* Invalid ArchReq - %s */
				return (4905);
			}
		}

		/* ArchReq %s not found */
		msgnum = 4906;
	}

	PthreadMutexLock(&archReqMutex);
	for (ari = 1; ari < archReqTableCount; ari++) {
		struct ArchReq *ar;

		ar = archReqTable[ari];
		if (ar == NULL) {
			continue;
		}
		if (*asname != '*' && strcmp(ar->ArAsname, asname) != 0) {
			continue;
		}
		if (seqnum != -1 && ar->ArSeqnum != seqnum) {
			continue;
		}
		ar->ArStatus |= ARF_rmreq;
		msgnum = 0;
	}

	PthreadMutexUnlock(&archReqMutex);
	if (msgnum == 0) {
		ArchiveRun();
	}
	return (msgnum);
}


/*
 * Remove an inode from ArchReqs.
 */
void
ArchiveRmInode(
	sam_id_t id)
{
	int	copy;

	/*
	 * Mark the file removed, and mark the ArchReq changed.
	 */
	PthreadMutexLock(&archReqMutex);
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		struct IdRef *ir;

		ir = idListLookup(id, copy);
		if (ir != NULL && ir->IrAtIndex != 0) {
			struct ArchReq *ar;

			ar = archReqTable[ir->IrAtIndex];
			if (ar != NULL) {
				struct FileInfo *fi;

				fi = (struct FileInfo *)ID_LOC(ir);
				fi->FiStatus |= FIF_remove;
				ar->ArStatus |= ARF_changed;
#if defined(FILE_TRACE)
				Trace(TR_DEBUG,
				    "Archreq remove (%s) inode: %d.%d (%s)",
				    archReqName(ar, arname),
				    fi->FiId.ino, fi->FiId.gen,
				    fi->FiName);
#endif /* defined(FILE_TRACE) */
			}
		}
	}
	PthreadMutexUnlock(&archReqMutex);
}


/*
 * Run the Archive() thread.
 */
void
ArchiveRun(void)
{
	PthreadMutexLock(&archiveWaitMutex);
#if defined(ARCH_TRACE)
	Trace(TR_DEBUG, "Archive run");
#endif /* defined(ARCH_TRACE) */
	archiveWaitCount++;
	PthreadCondSignal(&archiveWait);
	PthreadMutexUnlock(&archiveWaitMutex);
}


/*
 * Start archiving for a directory.
 */
void
ArchiveStartDir(
	char *dirPath)
{
	int	fpn;
	int	starts;

#if defined(ARCH_TRACE)
	Trace(TR_DEBUG, "Archive start dir (%s)", dirPath);
#endif /* defined(ARCH_TRACE) */
	if (Exec != ES_run) {
		return;
	}
	starts = 0;
	for (fpn = 1; fpn < FileProps->FpCount; fpn++) {
		struct FilePropsEntry *fp;
		int	ari;

		/*
		 * Compare the directory path to the file properties path.
		 */
		fp = &FileProps->FpEntry[fpn];
		if (*fp->FpPath != '\0') {
			char *pp, *dp;

			pp = fp->FpPath;
			dp = dirPath;
			while (*pp != '\0' && *pp == *dp) {
				pp++;
				dp++;
			}
			if (*pp != '\0' || (*dp != '\0' && *dp != '/')) {
				continue;
			}
		}

		/*
		 * Match found.
		 * Find any ArchReqs waiting to start and start them.
		 */
		PthreadMutexLock(&archReqMutex);
		for (ari = 1; ari < archReqTableCount; ari++) {
			struct ArchReq *ar;

			ar = archReqTable[ari];
			if (ar != NULL &&
			    ar->ArState == ARS_create &&
			    ar->ArCount != 0 &&
			    !(ar->ArStatus & (ARF_changed | ARF_rmreq))) {
				int	an;

				for (an = 0; an < MAX_AR; an++) {
					if (ar->ArAsn == fp->FpAsn[an]) {
						ar->ArStartTime = time(NULL);
						starts++;
					}
				}
			}
		}
		PthreadMutexUnlock(&archReqMutex);
	}
	if (starts > 0) {
		ArchiveRun();
	}
}


/*
 * Start archive requests.
 */
void
ArchiveStartRequests(void)
{
	startRequests = TRUE;
	ArchiveRun();
}


/*
 * Trace archive data.
 */
void
ArchiveTrace(void)
{
	doTrace = TRUE;
	ArchiveRun();
}


/* Private functions. */


/*
 * Add an archreq to the table.
 */
static void
addArchReq(
	struct ArchReq *ar)
{
	int	i;

	for (i = 1; i < archReqTableCount; i++) {
		if (archReqTable[i] == NULL) {
			break;
		}
	}
	if (i >= archReqTableCount) {
		i = archReqTableCount;
		archReqTableCount += ARCH_REQ_TABLE_INCR;
		SamRealloc(archReqTable,
		    archReqTableCount * sizeof (struct ArchReq *));
		memset(&archReqTable[i], 0,
		    (archReqTableCount - i) * sizeof (struct ArchReq *));
	}
	archReqTable[i] = ar;
	ar->ArAtIndex = i;
}



/*
 * Trace archive data.
 */
static void
archiveTrace(void)
{
	static char arname[ARCHREQ_NAME_SIZE];
	struct ArchReq *ar;
	FILE	*st;
	int	ari;

	if ((st = TraceOpen()) == NULL) {
		return;
	}

	/*
	 * Print ArchReq table.
	 */
	fprintf(st, "Archive - ArchReqs:\n");
	for (ari = 1; ari < archReqTableCount; ari++) {
		ar = archReqTable[ari];
		if (ar != NULL) {
			fprintf(st, "%4d %s\n", ari, archReqName(ar, arname));
#if defined(ARCH_TRACE)
		} else {
			fprintf(st, "%4d NULL\n", ari);
#endif /* defined(ARCH_TRACE) */
		}
	}
	fprintf(st, "\n");
	TraceClose(INT_MAX);
}


/*
 * Return the name of an ArchReq.
 * The name of the ArchReq is returned in a user's buffer.
 * The format is:  ArchiveSetName.SequenceNumber
 */
static char *
archReqName(
	struct ArchReq *ar,
	char *arname)
{
	snprintf(arname, ARCHREQ_NAME_SIZE, "%s.%d",
	    ar->ArAsname, ar->ArSeqnum);
	return (arname);
}


/*
 * Check for scheduling queue backup.
 * Find any ArchReq for this file properties entry that is being scheduled.
 */
static struct ArchReq *
checkQueue(
	int asn)
{
	int	ari;

	for (ari = 1; ari < archReqTableCount; ari++) {
		struct ArchReq *ar;

		ar = archReqTable[ari];
		if (ar != NULL && ar->ArAsn == asn &&
		    ar->ArState == ARS_schedule &&
		    !(ar->ArStatus & ARF_full)) {
			return (ar);
		}
	}
	return (NULL);
}


/*
 * Compare idList entry inode numbers.
 */
static int
cmp_ino(
	const void *p1,
	const void *p2)
{
	struct IdRef *ir1 = (struct IdRef *)p1;
	struct IdRef *ir2 = (struct IdRef *)p2;
	sam_id_t *ip1, *ip2;

	ip1 = ID_LOC(ir1);
	ip2 = ID_LOC(ir2);
	if (ip1->ino < ip2->ino) {
		return (-1);
	}
	if (ip1->ino > ip2->ino) {
		return (1);
	}
	return (0);
}


/*
 * Create a new ArchReq.
 * RETURN: The new ArchReq.
 */
static struct ArchReq *
createArchReq(
	struct ArchSet *as,
	int copy)
{
	static char fname[ARCHREQ_FNAME_SIZE];
	struct ArchReq *ar;
	int	asn;

	/*
	 * Initialize an empty ArchReq.
	 */
	ar = MapFileCreate("tmp", ARCHREQ_MAGIC, ARCHREQ_START);
	if (ar == NULL) {
		LibFatal(create, "ArchReq");
	}
	memmove(ar->ArFsname, FsName, sizeof (ar->ArFsname));
	memmove(ar->ArAsname, as->AsName, sizeof (ar->ArAsname));
	if (State->AfSeqNum < 0 || State->AfSeqNum >= INT_MAX-1) {
		State->AfSeqNum = 0;
	}
	ar->ArSeqnum = State->AfSeqNum++;
	ar->ArVersion	= ARCHREQ_VERSION;
	ar->ArState		= ARS_create;
	ar->ArDrives	= (as->AsFlags & AS_drives) ? as->AsDrives : 1;

	ar->ArMinSpace	= FSIZE_MAX;
	ar->ArPriority	= PR_MIN;
	ar->ArSize		= sizeof (struct ArchReq) +
	    ((ar->ArDrives - 1) * sizeof (struct ArcopyInstance));
	ar->ArStartCount = INT_MAX;
	ar->ArStartSize	= FSIZE_MAX;
	ar->ArStartTime	= TIME_MAX;
	ar->ArTime		= time(NULL);
	asn = as - ArchSetTable;
	if (asn >= MAX_AR) {
		setStartValues(ar, as);
	} else {
		/* Cannot archive - no rules. */
		ArchReqMsg(HERE, ar, 4364);
	}
	ar->Ar.MfValid = 1;
	ar->ArAsn = asn;
	ar->ArCopy = (short)copy;
	ArchReqFileName(ar, fname);
	if (MapFileRename(ar, fname) == -1) {
		LibFatal(rename, fname);
	}
	addArchReq(ar);
	arCreates[asn] = ar->ArAtIndex;
	Trace(TR_QUEUE, "Archreq create (%s) msg: '%s'", archReqName(ar, fname),
	    ar->ArCpi[0].CiOprmsg);
	return (ar);
}


/*
 * Delete an ArchReq.
 * Called with archReqMutex held, or during initialization when the Archive
 * thread is not running.
 */
static void
deleteArchReq(
	struct ArchReq *ar)
{
	static char arname[ARCHREQ_NAME_SIZE];
	static char fname[ARCHREQ_FNAME_SIZE];
	int	copy;

	(void) archReqName(ar, arname);
	copy = ar->ArCopy;
	Trace(TR_QUEUE, "Archreq delete (%s)", arname);
	if (ar->Ar.MfValid != 0) {
		if (ar->ArCount != 0) {
			char	*fic;
			int	i;

			/*
			 * Delete all files from the idList.
			 */
			fic = FIRST_FILE_INFO_ADDR(ar);
			for (i = 0; i < ar->ArCount; i++) {
				struct FileInfo *fi;

				fi = (struct FileInfo *)(void *)fic;
				fic += FI_SIZE(fi);
				idListDelete(fi->FiId, copy, ar);
			}
		}

		/*
		 * Clear reference to this ArchReq.
		 */
		if (arCreates[ar->ArAsn] == ar->ArAtIndex) {
			arCreates[ar->ArAsn] = 0;
		}
		archReqTable[ar->ArAtIndex] = NULL;

		/*
		 * Mark ArchReq invalid, and remove it.
		 */
		ar->Ar.MfValid = 0;
		idListCompress(copy);
	} else {
		int	ari;
		int	i;

		/*
		 * Invalid ArchReq - the file list cannot be used.
		 * Delete files from idList that are for this ArchReq.
		 */
		for (ari = 1; ari < archReqTableCount; ari++) {
			if (archReqTable[ari] == ar) {
				archReqTable[ari] = NULL;
				break;
			}
		}
		if (ari < archReqTableCount) {
			Trace(TR_ARDEBUG, "Remove from file list.");
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				struct IdList *il;
				struct IdRef *ir;

				il = idList[copy];
				ir = &il->IlEntry[0];
				for (i = 0; i < il->IlCount; i++, ir++) {
					if (ir->IrAtIndex == ari) {
						ir->IrAtIndex = 0;
					}
				}
				idListCompress(copy);
			}
		}
	}
	snprintf(fname, sizeof (fname), ARCHREQ_DIR"/%s", arname);
	(void) unlink(fname);
	(void) ArMapFileDetach(ar);
}


/*
 * Dequeue an ArchReq.
 */
static boolean_t
dequeueArchReq(
	struct ArchReq *ar)
{
	static char arname[ARCHREQ_NAME_SIZE];

	(void) archReqName(ar, arname);
	if (ArchiverDequeueArchReq(ar->ArFsname, arname) == 0) {
		Trace(TR_MISC, "DequeueArchReq(%s)", arname);
		return (TRUE);
	}
	if (errno == EADDRNOTAVAIL) {
		ar->ArState = ARS_done;
		Trace(TR_DEBUG, "%s not dequeued - not found", arname);
	} else {
		Trace(TR_DEBUG, "%s not dequeued - busy", arname);
	}
	return (FALSE);
}


/*
 * Increase the size of an ArchReq.
 * RETURN: The larger ArchReq.
 */
static struct ArchReq *
growArchReq(
	struct ArchReq *ar,	/* ArchReq to grow */
	int incr)		/* Additional size */
{
	if (ar->ArSize + incr >= ARCHREQ_MAX_SIZE) {
		char ts[ISO_STR_FROM_TIME_BUF_SIZE];

		ar->ArStartTime = time(NULL);
		(void) TimeToIsoStr(ar->ArStartTime, ts);
		/* Start archive at %s */
		ArchReqMsg(HERE, ar, 4355, ts);
		Trace(TR_QUEUE, "Archreq full (%s)", archReqName(ar, arname));
		ar->ArStatus |= ARF_full;
		ar->ArStatus &= ~ARF_merge;

		ar = createArchReq(&ArchSetTable[ar->ArAsn], ar->ArCopy);
	}

	/*
	 * Assure room for entry.
	 */
	while (ar->ArSize + incr >= ar->Ar.MfLen) {
		Trace(TR_QUEUE, "Archreq grow (%s) count: %d",
		    archReqName(ar, arname), ar->ArCount);
		ar = ArchReqGrow(ar);
		archReqTable[ar->ArAtIndex] = ar;
	}
	return (ar);
}


/*
 * Add an entry to a idList.
 */
static struct IdRef *
idListAdd(
	sam_id_t id,
	int copy)
{
	struct IdList *il;
	struct IdRef *ir, *irEnd;
	int	i;
	int	new;
	int	l, u;

	/*
	 * Binary search list.
	 */
	il = idList[copy];
	l = 0;
	u = il->IlCount;
	new = 0;
	while (u > l) {
		sam_ino_t ino;

		i = (l + u) >> 1;	/* Find midpoint */
		ir = &il->IlEntry[i];
		if (ir->IrAtIndex != 0) {
			sam_id_t *ip;

			ip = ID_LOC(ir);
			ino = ip->ino;
		} else {
			ino = ir->IrOffset;
		}
		if (id.ino == ino) {
			return (ir);
		}
		if (id.ino < ino) {
			u = i;
			new = u;
		} else {
			/* id.ino > ino */
			l = i + 1;
			new = l;
		}
	}

	/*
	 * Need a new entry.
	 */
	while (il->IlSize + sizeof (struct IdRef) > il->Il.MfLen) {
		static char name[] = IDLIST"0";

		snprintf(name, sizeof (name), "%s%d", IDLIST, copy + 1);
		idList[copy] =
		    MapFileGrow(il, IDLIST_INCR * sizeof (struct IdRef));
		if (idList[copy] == NULL) {
			LibFatal(MapFileGrow, name);
		}
		il = idList[copy];
#if defined(ARCH_TRACE)
		Trace(TR_DEBUG, "%s grow count: %d", name, il->IlCount);
#endif /* defined(ARCH_TRACE) */
	}

	/*
	 * Set location where entry belongs in sorted order.
	 * Move remainder of list up.
	 */
	ir = &il->IlEntry[new];
	irEnd = &il->IlEntry[il->IlCount];
	if (Ptrdiff(irEnd, ir) > 0) {
		memmove(ir+1, ir, Ptrdiff(irEnd, ir));
	}
	il->IlCount++;
	il->IlSize += sizeof (struct IdRef);
	ir->IrAtIndex = 0;
	ir->IrOffset = id.ino;
	return (ir);
}


/*
 * Compress the idList.
 */
static void
idListCompress(
	int copy)
{
	struct IdList *il;
	int	i, j;

	/*
	 * Eliminate deleted entries.
	 */
	il = idList[copy];
	for (i = j = 0; i < il->IlCount; i++) {
		if (il->IlEntry[i].IrAtIndex != 0) {
			il->IlEntry[j] = il->IlEntry[i];
			j++;
		}
	}
	il->IlCount = j;
	il->IlSize = sizeof (struct IdList) + (j-1) * sizeof (struct IdRef);
}


/*
 * Delete an entry from the idList.
 */
static void
idListDelete(
	sam_id_t id,
	int copy,
	struct ArchReq *ar)
{
	struct IdRef *ir;

	ir = idListLookup(id, copy);
	if (ir != NULL && archReqTable[ir->IrAtIndex] == ar) {
		ir->IrAtIndex = 0;
		ir->IrOffset = id.ino;
	} else {
		Trace(TR_DEBUG, "idListDelete(%d, %d) not found", id.ino, copy);
	}
}


/*
 * Lookup file in idList.
 */
static struct IdRef *
idListLookup(
	sam_id_t id,
	int copy)
{
	struct IdList *il;
	struct IdRef *ir;
	int	i;
	int	l, u;

	/*
	 * Binary search list.
	 */
	il = idList[copy];
	l = 0;
	u = il->IlCount;
	while (u > l) {
		sam_ino_t ino;

		i = (l + u) >> 1;	/* Find midpoint */
		ir = &il->IlEntry[i];
		if (ir->IrAtIndex != 0) {
			sam_id_t *ip;

			ip = ID_LOC(ir);
			ino = ip->ino;
			if (ino == id.ino) {
				if (ip->gen == id.gen) {
					return (ir);
				}
				return (NULL);
			}
		} else {
			ino = ir->IrOffset;
			if (id.ino == ino) {
				return (NULL);
			}
		}
		if (id.ino < ino) {
			u = i;
		} else {
			/* id.ino > ino */
			l = i + 1;
		}
	}
	return (NULL);
}


/*
 * Initialize module.
 */
static void
initArchive(void)
{
	int	i;

	/*
	 * Make the ArchReq table.
	 */
	archReqTableCount = ARCH_REQ_TABLE_INCR;
	SamMalloc(archReqTable, archReqTableCount * sizeof (struct ArchReq *));
	memset(archReqTable, 0, archReqTableCount * sizeof (struct ArchReq *));

	/*
	 * Make the idLists.
	 */
	for (i = 0; i < MAX_ARCHIVE; i++) {
		struct IdList *il;

		snprintf(ScrPath, sizeof (ScrPath), "%s%d", IDLIST, i + 1);
		idList[i] = MapFileCreate(ScrPath, IDLIST_MAGIC,
		    IDLIST_INCR * sizeof (struct IdRef));
		il = idList[i];
		il->Il.MfValid = 1;
		il->IlCount = 0;
		il->IlSize = sizeof (struct IdList);
	}

	ArchiveReconfig();

	/*
	 * Lock mutex to prevent creation of ArchReqs.
	 */
	PthreadMutexLock(&archReqMutex);
}


/*
 * Check the completed ArchReq.
 */
static struct ArchReq *
pruneArchReq(
	struct ArchReq *ar)
{
	static char arDname[ARCHREQ_NAME_SIZE];
	struct ArchReq *arD, *arS;
	time_t	timeNow;
	char	*fic;
	int	copy;
	int	copyBit;
	int	i;
	int	reexamine;

	(void) archReqName(ar, arname);
	Trace(TR_QUEUE, "Archreq prune (%s) count: %d state: '%s' status: 0x%x",
	    archReqName(ar, arname), ar->ArCount,
	    (ar->ArState == ARS_done)    ? "done" :
	    ((ar->ArState == ARS_create) ? "create" : "??"),
	    ar->ArStatus);

	/*
	 * Select the source and destination ArchReq-s.
	 * The default source is the ArchReq being pruned.
	 * The default destination is the create ArchReq.
	 */
	arS = ar;
	arD = archReqTable[arCreates[ar->ArAsn]];
	if (arD == ar) {
		/*
		 * This is the ArchReq being created.
		 * Start a new one.
		 */
		arCreates[ar->ArAsn] = 0;
		arD = NULL;
	} else if (ar->ArState == ARS_done) {
		*ar->ArCpi[0].CiOprmsg = '\0';
	}

	/*
	 * Remove file index if present.
	 */
	if (arD != NULL && arD->ArFileIndex != 0) {
		arD->ArSize = arD->ArFileIndex;
		arD->ArFiles = 0;
		arD->ArFileIndex = 0;
	}

	(void) archReqName(arS, arname);
	if (arD != NULL) {
		Trace(TR_QUEUE,
		    "Archreq prune (%s) count: %d dest: (%s) count: %d",
		    arname, arS->ArCount, archReqName(arD, arDname),
		    arD->ArCount);
	} else {
		Trace(TR_QUEUE, "Archreq prune (%s) count: %d",
		    arname, arS->ArCount);
	}

	/*
	 * Examine all files.
	 */
	copy = arS->ArCopy;
	copyBit = 1 << copy;
	reexamine = (State->AfExamine >= EM_noscan) ? FIF_exam : 0;
	timeNow = time(NULL);
	fic = FIRST_FILE_INFO_ADDR(arS);
	for (i = 0; i < arS->ArCount; i++) {
		static struct sam_perm_inode pinode;
		struct sam_disk_inode *dinode;
		struct FileInfo *fi;
		int	fiSize;

		/*
		 * Delete archived and files with errors.
		 */
		fi = (struct FileInfo *)(void *)fic;
		fiSize = FI_SIZE(fi);
		fic += fiSize;

		dinode = NULL;
#if defined(FILE_TRACE)
		if (GetPinode(fi->FiId, &pinode) == 0) {
			dinode = (struct sam_disk_inode *)&pinode;
			Trace(TR_DEBUG,
			    "ArchReq prune (%s) inode: %d.%d (%s) %x %d %d %d",
			    arname, fi->FiId.ino, fi->FiId.gen, fi->FiName,
			    dinode->arch_status,
			    dinode->status.b.archdone,
			    dinode->status.b.archnodrop,
			    dinode->status.b.offline);
		}
#endif /* defined(FILE_TRACE) */
		if (fi->FiFlags & FI_error) {
			/*
			 * Files with archive errors are re-examined.
			 */
			fi->FiStatus |= FIF_remove;
			if (fi->FiErrno != ENOENT) {
				static char errnoStr[STR_FROM_ERRNO_BUF_SIZE];

				/*
				 * Rescan files that have archive errors.
				 */
				fi->FiStatus |= reexamine;
#if defined(FILE_TRACE)
				Trace(TR_DEBUG, "Archreq file error "
				    "inode: %d.%d (%s) : '%s'",
				    fi->FiId.ino, fi->FiId.gen, fi->FiName,
				    StrFromErrno(fi->FiErrno, errnoStr,
				    sizeof (errnoStr)));
#endif /* defined(FILE_TRACE) */
			}
		} else if (fi->FiFlags & FI_archived) {

			/*
			 * Files archived are validated.
			 */
			fi->FiStatus |= FIF_remove;
			if (dinode != NULL ||
			    GetPinode(fi->FiId, &pinode) == 0) {
				dinode = (struct sam_disk_inode *)&pinode;
				if (!(dinode->arch_status & copyBit)) {
					/*
					 * File was probably written after
					 * being archived.
					 */
					fi->FiStatus |= reexamine;
					fi->FiFlags &= ~FI_archived;
					Trace(TR_MISC,
					    "Archreq copy missing inode: "
					    "%d.%d (%s)",
					    fi->FiId.ino, fi->FiId.gen,
					    fi->FiName);
				}
			}
		}
		if ((fi->FiStatus & reexamine) | (arS->ArStatus & ARF_rmreq)) {
			fi->FiStatus |= FIF_remove;
#if defined(FILE_TRACE)
			Trace(TR_DEBUG, "Archreq recheck inode: %d.%d (%s)",
			    fi->FiId.ino, fi->FiId.gen, fi->FiName);
#endif /* defined(FILE_TRACE) */
			ExamInodesAddEntry(fi->FiId, EXAM_INODE,
			    timeNow + EPSILON_TIME, "pruneArchreq");
		}
		if (fi->FiStatus & FIF_remove) {
			idListDelete(fi->FiId, copy, arS);
		} else {
			struct IdRef *ir;
			char	*ficD;

			if ((ir = idListLookup(fi->FiId, copy)) == NULL) {
				Trace(TR_DEBUG,
				    "prune-copy: inode %d not found",
				    fi->FiId.ino);
				continue;
			}
			fi->FiStatus &= ~FIF_remove;
			if (arD == NULL) {
				arD = createArchReq(&ArchSetTable[arS->ArAsn],
				    copy);
			}

			if (arD->ArSize + fiSize >= arD->Ar.MfLen) {
				arD = growArchReq(arD, fiSize);
			}

			/*
			 * Copy the FileInfo and update the idList entry.
			 */
			ficD = (char *)arD + arD->ArSize;
			memmove(ficD, fi, fiSize);
			ir->IrAtIndex = arD->ArAtIndex;
			ir->IrOffset = Ptrdiff(ficD, arD);

			/*
			 * Update ArchReq information.
			 */
			arD->ArCount++;
			arD->ArSize += fiSize;
			arD->ArSpace += fi->FiSpace;
			arD->ArMinSpace = min(arD->ArMinSpace, fi->FiSpace);
			arD->ArPriority = max(arD->ArPriority, fi->FiPriority);
			if (fi->FiStatus & FIF_offline) {
				arD->ArFlags |= AR_offline;
			}
			if (fi->FiStatus & FIF_segment) {
				arD->ArFlags |= AR_segment;
			}
			if (fi->FiStatus & FIF_rearch) {
				arD->ArStatus |= ARF_rearch;
			}

		}
	}

	if (arD != NULL) {
		arD->ArStatus &= ~(ARF_changed | ARF_merge | ARF_rmreq);
		Trace(TR_QUEUE, "Archreq prune (%s) complete count: %d",
		    archReqName(arD, arname), arD->ArCount);
	} else {
		Trace(TR_QUEUE, "Archreq prune (%s) complete",
		    archReqName(arS, arname));
	}

	arS->ArCount = 0;
	deleteArchReq(arS);
	return (arD);
}


/*
 * Send an ArchReq to sam-archiverd for scheduling.
 */
static struct ArchReq *
queueArchReq(
	struct ArchReq *ar)
{
	static char arname[ARCHREQ_NAME_SIZE];
	ushort_t saveStatus;
	ushort_t saveState;

	if (Exec != ES_run) {
		return (ar);
	}
	if (ar->ArFiles == 0) {
		size_t	size;
		char	*fic;
		int	*fii;
		int	i;

		/*
		 * Append the ArchReq file list.
		 */
		ar->ArFileIndex = ar->ArSize;
		size = ar->ArCount * sizeof (int);
		while (ar->ArSize + size >= ar->Ar.MfLen) {
			ar = ArchReqGrow(ar);
			archReqTable[ar->ArAtIndex] = ar;
		}
		ar->ArSize += size;
		fii = LOC_FILE_INDEX(ar);
		fic = FIRST_FILE_INFO_ADDR(ar);
		for (i = 0; i < ar->ArCount; i++) {
			struct FileInfo *fi;

			fi = (struct FileInfo *)(void *)fic;
			fic += FI_SIZE(fi);
			fii[i] = Ptrdiff(fi, ar);
		}
		ar->ArFiles = ar->ArCount;
	}

	/*
	 * Initialize for scheduling.
	 */
	saveState = ar->ArState;
	saveStatus = ar->ArStatus;
	ar->ArState = ARS_schedule;
	ar->ArStatus &= ~(ARF_start | ARF_changed | ARF_merge |
	    ARF_recover | ARF_rmreq);
	(void) archReqName(ar, arname);
	if (ArchiverQueueArchReq(FsName, arname) == 0) {
		static char sizebuf[STR_FROM_FSIZE_BUF_SIZE];

		Trace(TR_MISC, "Archreq queue (%s) files: %d space: %s",
		    arname, ar->ArFiles,
		    StrFromFsize(ar->ArSpace, 3, sizebuf, sizeof (sizebuf)));
		if (arCreates[ar->ArAsn] == ar->ArAtIndex) {
			arCreates[ar->ArAsn] = 0;
		}
	} else {
		Trace(TR_ERR, "Archreq queue (%s) failed", arname);
		/*
		 * Reset state - we will try it later.
		 */
		ar->ArState = saveState;
		ar->ArStatus = saveStatus;
	}
	return (ar);
}


/*
 * Recover an ArchReq.
 * RETURN: TRUE if recovered.
 */
static boolean_t
recoverAnArchReq(
	struct ArchReq *ar,
	char *arname)
{
	int	fpi;

	if (arCreates == NULL) {
		/*
		 * Make the ArCreates list.
		 */
		SamMalloc(arCreates, ArchSetNumof * sizeof (int *));
		memset(arCreates, 0, ArchSetNumof * sizeof (int *));
	}

	/*
	 * Find matching archive set.
	 */
	for (fpi = 0; fpi < FileProps->FpCount; fpi++) {
		struct FilePropsEntry *fp;
		int	an;

		fp = &FileProps->FpEntry[fpi];
		for (an = 0; an < MAX_AR; an++) {
			struct IdList *il;
			struct IdRef *ir;
			char	*fic;
			int	copy;
			int	i;

			if (strcmp(ar->ArAsname,
			    ArchSetTable[fp->FpAsn[an]].AsName) != 0) {
				continue;
			}

			if ((ar->ArStatus & ARF_rearch) && an < MAX_ARCHIVE) {
				an += MAX_ARCHIVE;
			}
			ar->ArAsn = fp->FpAsn[an];
			addArchReq(ar);
			copy = ar->ArCopy;

			/*
			 * Allocate space in idList.
			 */
			il = idList[copy];
			while (il->IlSize +
			    (ar->ArCount * sizeof (struct IdRef)) >
			    il->Il.MfLen) {
				idList[copy] = MapFileGrow(il,
				    IDLIST_INCR * sizeof (struct IdRef));
				if (idList[copy] == NULL) {
					LibFatal(MapFileGrow, IDLIST);
				}
				il = idList[copy];
#if defined(ARCH_TRACE)
				Trace(TR_DEBUG, "Grow idlist %d: %d",
				    copy, il->IlCount);
#endif /* defined(ARCH_TRACE) */
			}

			/*
			 * Add files to the idList.
			 */
			ir = &il->IlEntry[il->IlCount];
			il->IlCount += ar->ArCount;
			fic = FIRST_FILE_INFO_ADDR(ar);
			for (i = 0; i < ar->ArCount; i++) {
				struct FileInfo *fi;

				fi = (struct FileInfo *)(void *)fic;
				ir->IrAtIndex = ar->ArAtIndex;
				ir->IrOffset = Ptrdiff(fi, ar);
				fic += FI_SIZE(fi);
				ir++;
			}

			/*
			 * Sort idList entries and eliminate duplicates.
			 */
			qsort(&il->IlEntry[0], il->IlCount,
			    sizeof (struct IdRef), cmp_ino);
			ir = &il->IlEntry[0];
			for (i = 0; i < il->IlCount; i++) {
				struct IdRef *ir2;
				sam_id_t *ip, *ip2;

				ip = ID_LOC(ir);
				ir2 = &il->IlEntry[i];
				ip2 = ID_LOC(ir2);
				if (ip->ino != ip2->ino) {
					ir++;
					*ir = *ir2;
				}
			}
			il->IlCount = ir - &il->IlEntry[0] + 1;

			switch (ar->ArState) {

			case ARS_create: {
				if (arCreates[ar->ArAsn] == 0) {
					arCreates[ar->ArAsn] = ar->ArAtIndex;
					ar = updateParams(ar,
					    &ArchSetTable[ar->ArAsn]);
					Trace(TR_MISC,
					    "ArchReq %s (create) recovered",
					    arname);
					return (TRUE);
				}
				Trace(TR_MISC,
				    "Already have create ArchReq for %s",
				    ar->ArAsname);
				}
				break;

			case ARS_schedule:
			case ARS_archive:
				ar->ArStatus |= ARF_recover;
				if (State->AfExec == ES_fs_wait) {
					/* Waiting for :arrun fs.%s */
					ArchReqMsg(HERE, ar, 4353, FsName);
				} else if (State->AfExec == ES_wait) {
					/* Waiting for :arrun %s */
					ArchReqMsg(HERE, ar, 4352, "");
				} else {
					*ar->ArCpi[0].CiOprmsg = '\0';
				}
				Trace(TR_MISC,
				    "ArchReq %s (schedule) recovered", arname);
				return (TRUE);
/*LINTED statement not reached */
				break;

			case ARS_done:
				/*
				 * This will be processed in the main
				 * archive loop.
				 */
				Trace(TR_MISC,
				    "ArchReq recovered done (%s)", arname);
				return (TRUE);
/*LINTED statement not reached */
				break;

			default:
				Trace(TR_MISC, "Invalid ArchReq %s", arname);
				break;
			}
			deleteArchReq(ar);
			return (TRUE);
		}
	}
	Trace(TR_MISC, "Archive Set for ArchReq(%s) not found", arname);
	return (FALSE);
}


/*
 * Recover ArchReqs.
 */
static void
recoverArchReqs(void)
{
	DIR		*dirp;
	struct dirent *dirent;

	/*
	 * Attach ArchReq-s.
	 */
	if ((dirp = opendir(ARCHREQ_DIR)) == NULL) {
		Trace(TR_ERR, "opendir(%s) failed", ARCHREQ_DIR);
		return;
	}
	while ((dirent = readdir(dirp)) != NULL) {
		if (*dirent->d_name != '.') {
			struct ArchReq *ar;
			char	*arname;
			char	fname[ARCHREQ_FNAME_SIZE];
			int	i;

			arname = dirent->d_name;
			for (i = 1; i < archReqTableCount; i++) {
				ar = archReqTable[i];
				if (ar != NULL &&
				    strcmp(archReqName(ar, fname), arname) ==
				    0) {
					break;
				}
			}
			if (i < archReqTableCount) {
				continue;
			}
			ar = NULL;
			if (!Recover ||
			    (ar =
			    ArchReqAttach(FsName, arname, O_RDWR)) == NULL) {
				goto rmArchReq;
			}
			if (!ArchReqValid(ar)) {
#if defined(DEBUG)

				/*
				 * Save the bad ArchReq.
				 */
				MakeDir(ARCHREQ_DIR"/bad");
				snprintf(fname, sizeof (fname),
				    ARCHREQ_DIR"/%s", arname);
				snprintf(ScrPath, sizeof (ScrPath),
				    ARCHREQ_DIR"/bad/%s", arname);
				(void) rename(fname, ScrPath);
#endif /* defined(DEBUG) */
				goto rmArchReq;
			}
			if (recoverAnArchReq(ar, arname)) {
				/*
				 * Recovered.
				 */
				continue;
			}

			/*
			 * Remove the ArchReq.
			 */
rmArchReq:
			if (ar != NULL) {
				(void) ArMapFileDetach(ar);
			}
			snprintf(ScrPath, sizeof (ScrPath),
			    ARCHREQ_DIR"/%s", arname);
			(void) unlink(ScrPath);
		}
	}
	(void) closedir(dirp);
}


/*
 * Set start values.
 */
static void
setStartValues(
	struct ArchReq *ar,
	struct ArchSet *as)
{
	int	flags;

	flags = as->AsFlags & (AS_startage | AS_startcount | AS_startsize);
	if (State->AfExamine >= EM_noscan) {
		flags |= AS_startage | AS_startcount | AS_startsize;
		ar->ArStartTime = ar->ArTime + STARTAGE;
		ar->ArStartCount = STARTCOUNT;
		/* Set STARTSIZE to 90% of ARCHMAX */
		ar->ArStartSize = (fsize_t)((0.9) * GetArchmax(as, NULL));
	}
	if (as->AsFlags & AS_startage) {
		ar->ArStartTime = ar->ArTime + as->AsStartAge;
	}
	if (as->AsFlags & AS_startcount) {
		ar->ArStartCount = as->AsStartCount;
	}
	if (as->AsFlags & AS_startsize) {
		ar->ArStartSize = as->AsStartSize;
	}
	if (flags != 0) {
		static char msg_buf[256];
		char	*p, *pe;
		char	*sep;

		ar->ArStatus |= ARF_start;
		p = msg_buf;
		pe = p + sizeof (msg_buf) - 1;
		sep = "";
		if (flags & AS_startage) {
			(void) TimeToIsoStr(ar->ArStartTime, p);
			p += strlen(p);
			sep = " | ";
		}
		if (flags & AS_startcount) {
			snprintf(p, Ptrdiff(pe, p), "%s%d files",
			    sep, ar->ArStartCount);
			p += strlen(p);
			sep = " | ";
		}
		if (flags & AS_startsize) {
			snprintf(p, Ptrdiff(pe, p), "%s%s bytes", sep,
			    StrFromFsize(ar->ArStartSize, 1, NULL, 0));
		}
		/* Start archive at %s */
		ArchReqMsg(HERE, ar, 4355, msg_buf);
	}
}


/*
 * Update Archive Set parameters for an ArchReq.
 */
static struct ArchReq *
updateParams(
	struct ArchReq *ar,
	struct ArchSet *as)
{
	setStartValues(ar, as);
	if ((as->AsFlags & AS_drives) && ar->ArDrives != as->AsDrives) {
		char	*fic;
		int	copy;
		int	i;
		int	incr;

		/*
		 * Drive count is different.
		 * Adjust size of ArchReq for new drive count.
		 * Relocate file information.
		 */
		incr = (as->AsDrives - ar->ArDrives) *
		    sizeof (struct ArcopyInstance);
		while (ar->ArSize + incr >= ar->Ar.MfLen) {
			ar = ArchReqGrow(ar);
			archReqTable[ar->ArAtIndex] = ar;
		}
		fic = FIRST_FILE_INFO_ADDR(ar);
		memmove(fic + incr, fic, ar->ArSize - Ptrdiff(fic, ar));
		if (incr > 0) {
			memset(fic, 0, incr);
		}
		ar->ArDrives = as->AsDrives;
		ar->ArSize += incr;

		/*
		 * Adjust idList offsets.
		 */
		copy = ar->ArCopy;
		fic = FIRST_FILE_INFO_ADDR(ar);
		for (i = 0; i < ar->ArCount; i++) {
			struct FileInfo *fi;
			struct IdRef *ir;

			fi = (struct FileInfo *)(void *)fic;
			ir = idListAdd(fi->FiId, copy);
			ir->IrAtIndex = ar->ArAtIndex;
			ir->IrOffset = Ptrdiff(fi, ar);
			fic += FI_SIZE(fi);
		}
	}
	return (ar);
}
