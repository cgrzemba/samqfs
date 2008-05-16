/*
 * volume.c - Archiver volume processing.
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

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

/* Solaris headers. */
#include <libgen.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/archiver.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/defaults.h"
#include "sam/lib.h"
#include "aml/robots.h"
#include "aml/diskvols.h"

/* Local headers. */
#include "common.h"
#include "archset.h"
#include "archreq.h"
#include "device.h"
#include "threads.h"
#include "volume.h"

/* Macros. */

/* Test for volume availability. */
#define	RM_VOLUME_AVAILABLE(ce) (((ce)->CeStatus & CES_inuse) && \
		*(ce)->CeMtype != 0 && *(ce)->CeVsn != '\0' && \
		!((ce)->CeStatus & VOL_UNAVAILABLE_STATUS))

/* Test for volume usability. */
#define	RM_VOLUME_USABLE(ce) (((ce)->CeStatus & CES_inuse) && \
		*(ce)->CeMtype != 0 && *(ce)->CeVsn != '\0' && \
		!((ce)->CeStatus & RM_VOL_UNUSABLE_STATUS) && \
		rmVolSpace((ce)->CeMtype, (ce)->CeSpace, (ce)->CeCapacity) != 0)

#define	DK_VOLUME_USABLE(dv) (!((dv)->DvFlags & DK_VOL_UNUSABLE_STATUS) && \
		dkVolSpace(dv) != 0)

/* Private data. */

/*
 * Volume reference table.
 * Each Volume has an entry in the Volume reservation table.
 */
static pthread_mutex_t volRefMutex = PTHREAD_MUTEX_INITIALIZER;
static struct VolRef {
	int	count;
	struct VolRefEntry {
		int	VrFlags;
		int	VrAln;			/* Archive library number */
		struct CatalogEntry *VrCe;	/* Catalog Entry */
	} entry[1];
} *volRefTable IVAL(NULL);

static struct DiskVolRef {
	int	count;
	struct  DiskVolRefEntry {
		int   VrFlags;
		vsn_t VrVsn;	/* Disk volume name */
		int   VrAln;	/* Archiver's disk volume library number */
	} entry[1];
} *diskVolRefTable IVAL(NULL);

#define	VR_selected	0x01	/* Selected once in GetVsns() */

static boolean_t catalogValid = FALSE;

/* Private functions. */
static boolean_t archSetVol(struct ArchSet *as, struct CatalogEntry *ce,
		char *asname, char *owner, char *fsname);
static int cmp_VolRef(const void *p1, const void *p2);
static void makeVolRef(void);
static void makeDiskVolRef(void);
static void setVolumeInfo(struct CatalogEntry *ce, struct VolInfo *vi);
static fsize_t rmVolSpace(char *mtype, fsize_t space, fsize_t capacity);
static fsize_t dkVolSpace(struct DiskVolumeInfo *dv);


/*
 * Get removable media archive set volume.
 * GetRmArchiveVol() returns a volume for the archive set.  The caller can
 * repeatedly call this function to obtain all the volumes for the
 * archive set.
 * The first call is identified by volNum == 0.  The continuation data
 * is maintained in static data in the function.
 * The volume will be usable for archiving.
 *
 * The search is performed in three phases:
 * Phase 1 - Return acceptable volumes that have been recently used.
 * Phase 2 - Return only volumes reserved to the Archive Set.
 * Phase 3 - Use the volume descriptor(s) to identify any volumes that
 *      the archive set may use.
 * RETURN: -1 if no more volumes.
 */
int
GetRmArchiveVol(
	struct ArchSet *as,
	int volNum,	/* 0 if first volume requested */
	char *owner_a,	/* Owner string (NULL if called from lister.c) */
	char *fsname_a,	/* File system name (NULL if called from lister) */
	struct VolInfo *vi)
{
	static vsndesc_t *vd;
	static struct VsnExp *ve;
	static struct VsnPool *vp;
	static char *asname;
	static char *fsname;
	static char *owner;

	static int	adn;
	static int	aln;

	static int	phase;		/* Search phase */
	static int	pxn;		/* Pool expression number */
	static int	vdn;		/* volume descriptor number */
	static int	vdnumof;
	static int	vin;		/* volume info table number */

	struct VolRefEntry *vr;
	struct CatalogEntry *ce;

	if (volRefTable->count == 0) {
		return (-1);
	}
	if (volNum == 0) {
		/*
		 * Perform initialization.
		 */
		if (as->AsReserve & RM_set) {
			asname = as->AsName;
		} else {
			asname = "";
		}
		if (as->AsReserve & RM_owner && owner_a != NULL) {
			owner = owner_a;
		} else {
			owner = "";
		}
		if (as->AsReserve & RM_fs && fsname_a != NULL) {
			fsname = fsname_a;
		} else {
			fsname = "";
		}

		/*
		 * Initialize phase 1.
		 */
		phase = 1;
		aln = 0;
		adn = 0;
	}

	if (phase == 1) {

		/*
		 * Examine volumes recently used.
		 */
		while (aln < ArchLibTable->count) {
			struct CatalogEntry ced;
			struct ArchDriveEntry *ad;
			struct ArchLibEntry *al;

			al = &ArchLibTable->entry[aln];
			if (adn > al->AlDrivesNumof ||
			    al->AlFlags &
			    (AL_historian | AL_disk | AL_honeycomb)) {
				aln++;
				adn = 0;
				continue;
			}
			ad = &al->AlDriveTable[adn++];
			if (*ad->AdVi.VfMtype != '\0') {
				ce = CatalogGetCeByMedia(ad->AdVi.VfMtype,
				    ad->AdVi.VfVsn, &ced);
				if (ce != NULL && RM_VOLUME_USABLE(ce) &&
				    archSetVol(as, ce, asname, owner, fsname) &&
				    owner_a != NULL && fsname_a != NULL) {
					vi->VfAln = aln;
					setVolumeInfo(ce, vi);
					return (0);
				}
			}
		}

		/*
		 * Initialize phase 2.
		 */
		phase = 2;
		vin = 0;
	}

	if (phase == 2) {

		/*
		 * Examine only reserved volumes.
		 */
		while (as->AsReserve != RM_none && vin < volRefTable->count) {
			vr = &volRefTable->entry[vin++];
			ce = vr->VrCe;
			if (!(RM_VOLUME_USABLE(ce))) {
				break;
			}
			if (ce->r.CerTime != 0 &&
			    archSetVol(as, ce, asname, owner, fsname) &&
			    owner_a != NULL && fsname_a != NULL) {
				vi->VfAln = vr->VrAln;
				setVolumeInfo(ce, vi);
				return (0);
			}
		}


		/*
		 * Initialize phase 3.
		 * "Inner" loop initialization.
		 * Begin stepping through VSN descriptors by making the inner
		 * loop initialize.
		 */
		phase = 3;
		vin = volRefTable->count;

		/*
		 * "Outer" loop initialization.
		 * Set only descriptor or first descriptor in a list.
		 */
		vdn = 0;
		if ((as->AsVsnDesc & VD_mask) != VD_list) {
			vd = &as->AsVsnDesc;
			vdnumof = 1;
		} else {
			struct VsnList *vls;

			vls = (void *)((char *)VsnListTable +
			    (as->AsVsnDesc & ~VD_mask));
			vdnumof = vls->VlNumof;
			vd = vls->VlDesc;
		}
		vp = NULL;
	}

	/*
	 * Phase 3:
	 * Check all volumes against all VSN descriptors.
	 * This is a reenterable doubly nested for loop.
	 * "Inner" loop over all volumes.
	 * "Outer" loop over all VSN descriptors for the Archive Set.
	 */
	for (;;) {
		if (vin >= volRefTable->count) {
			if (vdn >= vdnumof) {
				return (-1);
			}

			/*
			 * Reset the VSN list accession and advance to the next
			 * VSN descriptor.  Set the regular expression from the
			 * descriptor.
			 */
			vin = 0;
			switch (*vd & VD_mask) {
			case VD_none:
				ve = VsnExpTable; /* ".*" entry is first */
				vdn++;
				vd++;
				break;
			case VD_exp:
				ve = (struct VsnExp *)(void *)((char *)
				    VsnExpTable + (*vd & ~VD_mask));
				vdn++;
				vd++;
				break;
			case VD_pool:
				if (vp == NULL) {
					/* Start the pool list. */
					vp = (struct VsnPool *)(void *)((char *)
					    VsnPoolTable + (*vd & ~VD_mask));
					pxn = 0;
				}
				/* Next pool entry. */
				ve = (struct VsnExp *)(void *)((char *)
				    VsnExpTable + vp->VpVsnExp[pxn]);
				pxn++;
				if (pxn >= vp->VpNumof) {
					vp = NULL;
					vdn++;
					vd++;
				}
				break;
			default:
				ASSERT(*vd == VD_mask);
				return (-1);
			}
		}

		/*
		 * Next volume.
		 */
		vr = &volRefTable->entry[vin++];
		ce = vr->VrCe;

		/*
		 * Make sure that the volume can be archived to.
		 */
		if (!(RM_VOLUME_USABLE(ce))) {
			/*
			 * Terminate search - all remaining are unusable.
			 */
			vin = volRefTable->count;
			continue;
		}

		if (ce->r.CerTime != 0) {
			/*
			 * Reserved volume.
			 * If the volume is not reserved for this archive set,
			 * skip it.
			 */
			if (strcmp(asname, ce->r.CerAsname) != 0 ||
			    strcmp(owner, ce->r.CerOwner) != 0 ||
			    strcmp(fsname, ce->r.CerFsname) != 0) {
				continue;
			}
		}

		/*
		 * Try VSN against the regexp.
		 */
		if (strcmp(as->AsMtype, ce->CeMtype) != 0) {
			continue;
		}
		if (ve == VsnExpTable) {
			/* ".*" >> any VSN is acceptable */
			break;
		}
		if (regex(ve->VeExpbuf, ce->CeVsn, NULL) != NULL) {
			break;
		}
	}

	vi->VfAln = vr->VrAln;
	setVolumeInfo(ce, vi);
	return (0);
}


/*
 * Get disk or honeycomb media archive set volume.
 *
 * GetDkArchiveVol() returns a disk volume or honeycomb silo for the
 * archive set.  The caller can repeatedly call this function to obtain
 * all the disk volumes or silos for the archive set.
 *
 * The first call is identified by volNum == 0.  The continuation data
 * is maintained in static data in the function.
 *
 * The returned disk volume or silo is be usable for archiving.
 *
 * RETURN: -1 if no more volumes or silos.
 */
int
GetDkArchiveVol(
	struct ArchSet *as,
	struct DiskVolsDictionary *diskVols,
	int volNum,		/* 0 if first request */
	struct VolInfo *vi)
{
	static vsndesc_t *vd;
	static struct VsnExp *ve;
	static struct VsnPool *vp;
	static struct VsnList *vls;
	static int	vin;		/* disk volume ref table iterator */
	static int	pxn;		/* pool expression iterator */
	static int	vdn;		/* volume expression iterator */
	static int	vdnumof;	/* number of volume expressions */

	struct DiskVolRefEntry *vr;
	int numofDiskVols;
	struct DiskVolumeInfo *dv;

	if (diskVols == NULL) {
		return (-1);
	}

	(void) diskVols->Numof(diskVols, &numofDiskVols, DV_numof_all);
	if (numofDiskVols == 0) {
		return (-1);
	}

	/*
	 * Check if need to initialize.
	 */
	if (volNum == 0) {
		/*
		 * Clear selected flag.
		 */
		for (vin = 0; vin < diskVolRefTable->count; vin++) {
			vr = &diskVolRefTable->entry[vin];
			vr->VrFlags &= ~VR_selected;
		}

		vin = numofDiskVols;
		ve = NULL;
		if ((as->AsVsnDesc & VD_mask) != VD_list) {
			vd = &as->AsVsnDesc;
			vdnumof = 1;
		} else {
			vls = (void *)((char *)VsnListTable +
			    (as->AsVsnDesc & ~VD_mask));
			vd = vls->VlDesc;
			vdnumof = vls->VlNumof;
		}
		vp = NULL;
		vdn = 0;
	}

	for (;;) {
		if (vin >= numofDiskVols) {
			if (vdn >= vdnumof) {
				return (-1);
			}

			/*
			 * Reset the VSN list iterator.
			 * Set the regular expression.
			 */
			vin = 0;

			switch (*vd & VD_mask) {
			case VD_none:
				ve = VsnExpTable;
				vdn++;
				vd++;
				break;
			case VD_exp:
				ve = (struct VsnExp *)(void *)((char *)
				    VsnExpTable + (*vd & ~VD_mask));
				vdn++;
				vd++;
				break;
			case VD_pool:
				if (vp == NULL) {
					/*
					 * Start the pool list.
					 */
					vp = (struct VsnPool *)(void *)((char *)
					    VsnPoolTable + (*vd & ~VD_mask));
					pxn = 0;
				}
				/*
				 * Next pool entry.
				 */
				ve = (struct VsnExp *)(void *)((char *)
				    VsnExpTable + vp->VpVsnExp[pxn]);
				pxn++;
				if (pxn >= vp->VpNumof) {
					vp = NULL;
					vdn++;
					vd++;
				}
				break;
			default:
				return (-1);
			}
		}

		/*
		 * Next volume.
		 */
		vr = &diskVolRefTable->entry[vin];
		(void) diskVols->Get(diskVols, vr->VrVsn, &dv);

		vin++;

		if (dv == NULL) {
			continue;
		}

		if (vr->VrFlags & VR_selected) {
			/* Returned this one already */
			continue;
		}

		if (regex(ve->VeExpbuf, vr->VrVsn, NULL) != NULL) {
			/*
			 * Found matching volume.  Check if available and
			 * make sure volume can be archived to.
			 */
			if (DiskVolsIsAvail(vr->VrVsn, dv, B_FALSE,
			    DVA_archiver) == B_TRUE) {
				if (DK_VOLUME_USABLE(dv)) {
					break;
				}
			}

#if defined(DEBUG)
			if (DiskVolsIsAvail(vr->VrVsn, dv, B_FALSE,
			    DVA_archiver) == B_FALSE) {
				Trace(TR_MISC,
				    "Disk volume '%s' not available",
				    vr->VrVsn);
			}
			if (DK_VOLUME_USABLE(dv) == 0) {
				Trace(TR_MISC,
				    "Disk volume '%s' not usable 0x%x",
				    vr->VrVsn, dv->DvFlags);
			}
#endif

		}
	}

	vr->VrFlags |= VR_selected;
	memset(vi, 0, sizeof (struct VolInfo));
	memmove(vi->VfMtype, as->AsMtype, sizeof (vi->VfMtype));
	memmove(vi->VfVsn, vr->VrVsn, sizeof (vi->VfVsn));
	vi->VfCapacity = dv->DvCapacity;
	vi->VfSpace = dv->DvSpace;
	vi->VfAln = vr->VrAln;

	return (0);
}


/*
 * Get the volume information for a drive.
 */
void
GetDriveVolInfo(
	dev_ent_t *dev,
	struct VolInfo *vi)
{
	if (dev == NULL || dev->state != DEV_ON ||
	    !dev->status.b.ready ||
	    !dev->status.b.present ||
	    !dev->status.b.labeled) {
		*vi->VfVsn = '\0';
		vi->VfFlags |= VF_unusable;
		return;
	}
	strncpy(vi->VfMtype, sam_mediatoa(dev->type), sizeof (vi->VfMtype));
	memmove(vi->VfVsn, dev->vsn, sizeof (vi->VfVsn));
	vi->VfFlags = 0;
	vi->VfCapacity = (fsize_t)dev->capacity * 1024;
	vi->VfSpace = rmVolSpace(vi->VfMtype, dev->space, dev->capacity);
	if (*vi->VfVsn == 0 ||
	    vi->VfSpace == 0 ||
	    dev->status.b.bad_media ||
	    dev->status.b.read_only ||
	    dev->status.b.write_protect ||
	    dev->status.b.strange ||
	    dev->status.b.unload) {
		vi->VfFlags |= VF_unusable;
	}
	vi->VfFlags |= VF_loaded;
}


/*
 * Return data about a volume.
 * GetVolInfo() may be called repeatedly using an increasing value for
 * volNum.  Used in this manner, information about all usable volumes can be
 * obtained.
 * RETURN: 0 if no error, -1 if index not valid.
 */
int
GetVolInfo(
	int volNum,		/* Index of the volume in VolRef */
	struct VolInfo *vi)
{
	struct VolRefEntry *vr;
	struct CatalogEntry *ce;
	int	retval;

	retval = 0;
	PthreadMutexLock(&volRefMutex);
	if (volNum >= volRefTable->count) {
		memset(vi, 0, sizeof (struct VolInfo));
		vi->VfFlags |= VF_unusable;
		retval = -1;
	} else {
		vr = &volRefTable->entry[volNum];
		ce = vr->VrCe;
#if !defined(AR_DEBUG)
		if (!RM_VOLUME_USABLE(ce)) {
			retval = -1;
#else /* !defined(AR_DEBUG) */
		if (!(ce->CeStatus & CES_inuse)) {
			memset(vi, 0, sizeof (struct VolInfo));
			vi->VfAln = -1;
			vi->VfFlags |= VF_unusable;
#endif /* !defined(AR_DEBUG) */
		} else {
			vi->VfAln = vr->VrAln;
			setVolumeInfo(ce, vi);
		}
	}
	PthreadMutexUnlock(&volRefMutex);
	return (retval);
}


/*
 * Is a volume available for access.
 */
boolean_t
IsVolumeAvailable(
	media_t media,
	vsn_t vsn,
	int *stgSim)
{
	struct CatalogEntry ced;
	boolean_t avail;
	char	*mtype;

	*stgSim = 0;
	if (is_third_party(media)) {
		return (TRUE);
	}
	mtype = sam_mediatoa(media);
	if (strcmp(mtype, "dk") == 0) {
		struct DiskVolsDictionary *diskVols;
		struct DiskVolumeInfo *dv;

		dv = NULL;
		avail = FALSE;

		ThreadsDiskVols(B_TRUE);
		diskVols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT,
		    DISKVOLS_RDONLY);
		if (diskVols != NULL) {
			(void) diskVols->Get(diskVols, vsn, &dv);
			if (dv != NULL) {
				avail = TRUE;
				if (dv->DvFlags &
				    (DV_unavail | DV_bad_media)) {
					avail = FALSE;
				}
			}
			(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
		}
		ThreadsDiskVols(B_FALSE);
		return (avail);
	}

	avail = FALSE;
	PthreadMutexLock(&volRefMutex);
	if (catalogValid && CatalogGetCeByMedia(mtype, vsn, &ced) != NULL &&
	    RM_VOLUME_AVAILABLE(&ced)) {
		static int aln = -1;

		/*
		 * Look up library.
		 * Last library lookup is cached.
		 */
		if (aln < 0 || ArchLibTable->entry[aln].AlEq != ced.CeEq) {
			for (aln = 0;
			    ArchLibTable->entry[aln].AlEq != ced.CeEq; aln++) {
				if (aln >= ArchLibTable->count) {
					aln = -1;
					break;
				}
			}
		}
		/*
		 * Check for library and drive availability.
		 */
		if (aln >= 0 && (ArchLibTable->entry[aln].AlFlags & AL_avail) &&
		    ArchLibTable->entry[aln].AlDrivesAvail != 0) {
			avail = TRUE;
			*stgSim = (ArchLibTable->entry[aln].AlFlags & AL_sim) ?
			    FI_stagesim : 0;
		}
	}
	PthreadMutexUnlock(&volRefMutex);
	return (avail);
}


/*
 * Initialize module catalog access.
 * Called with volRefMutex held, during initialization.
 * Public access for arcopy.
 */
int
VolumeCatalog(void)
{
	int	retval;

	retval = CatalogInit(program_name);
	catalogValid = (retval == -1) ? FALSE : TRUE;
	Trace(TR_ARDEBUG, "catalogValid");
	return (retval);
}


/*
 * Check module.
 */
void
VolumeCheck(void)
{
	if (ArchLibTable->AlRmDrives == 0) {
		return;
	}
	PthreadMutexLock(&volRefMutex);
	if (catalogValid) {
		if (CatalogSync() != 0) {
			catalogValid = FALSE;
			CatalogTerm();	/* Remove present catalog tables */
			Trace(TR_MISC, "Catalog found invalid");
		}
	}
	if (!catalogValid || volRefTable == NULL) {
		PthreadMutexUnlock(&volRefMutex);
		VolumeConfig();
		PthreadMutexLock(&volRefMutex);
	}
	qsort(&volRefTable->entry, volRefTable->count,
	    sizeof (struct VolRefEntry), cmp_VolRef);
	PthreadMutexUnlock(&volRefMutex);
}


/*
 * Configure module.
 */
void
VolumeConfig(void)
{
	PthreadMutexLock(&volRefMutex);
	if (ArchLibTable->AlRmDrives != 0) {
		if (volRefTable != NULL) {
			/*
			 * Discard present table.
			 */
			SamFree(volRefTable);
			volRefTable = NULL;
		}
		if (VolumeCatalog() == -1) {
			Trace(TR_ERR, "catalog initialization failed");
		}
	}
	makeVolRef();
	qsort(&volRefTable->entry, volRefTable->count,
	    sizeof (struct VolRefEntry), cmp_VolRef);
	PthreadMutexUnlock(&volRefMutex);
	makeDiskVolRef();
}


/*
 * Print Vsn information.
 */
void
PrintVolInfo(
	FILE *st,
	struct VolInfo *vi)
{
	struct CatalogEntry ced;
	char status_str[13];

	if (*vi->VfVsn == '\0') {
		fprintf(st, "\n");
		return;
	}
	fprintf(st, "%s.", vi->VfMtype);
	fprintf(st, "%-20s ", vi->VfVsn);

	if (strcmp(vi->VfMtype, "dk") == 0) {
		fprintf(st, "\n");
		return;
	}

	if (catalogValid) {
		if (CatalogGetCeByMedia(vi->VfMtype, vi->VfVsn, &ced) == NULL) {
			fprintf(st, "  ??\n");
			return;
		}
	} else {
		fprintf(st, "\n");
		return;
	}

	vi->VfCapacity = (fsize_t)ced.CeCapacity * 1024;
	/* capacity: */
	fprintf(st, "%s %6s ", GetCustMsg(4635),
	    StrFromFsize(vi->VfCapacity, 1, NULL, 0));

	/* space: */
	vi->VfSpace = rmVolSpace(vi->VfMtype, ced.CeSpace, ced.CeCapacity);
	fprintf(st, "%s %6s ", GetCustMsg(4636),
	    StrFromFsize((fsize_t)vi->VfSpace, 1, NULL, 0));

	fprintf(st, " %s", CatalogStatusToStr(ced.CeStatus, status_str));

#if defined(VSN_FLAGS)
	fprintf(st, " flags:0x%04x", vi->VfFlags);
#endif /* defined(VSN_FLAGS) */
	fprintf(st, "\n");
	if (vi->VfFlags & VF_reserved) {
		/* reserved: */
		fprintf(st, "   %s %s/%s/%s\n", GetCustMsg(4637),
		    ced.r.CerAsname, ced.r.CerOwner, ced.r.CerFsname);
	}
	if (vi->VfFlags & VF_debug) {
		fprintf(st, "   aln:%d %s%2d", vi->VfAln,
			/* slot: */
		    GetCustMsg(4648), vi->VfSlot);
		fprintf(st, "\n");
	}
}



/* Private functions. */


/*
 * Determine whether a volume can be used for the archive set.
 */
static boolean_t
archSetVol(
	struct ArchSet *as,
	struct CatalogEntry *ce,
	char *asname,
	char *owner,
	char *fsname)
{
	vsndesc_t *vd;
	int	vdnumof;

	if (strcmp(as->AsMtype, ce->CeMtype) != 0) {
		/*
		 * Wrong media type.
		 */
		return (FALSE);
	}
	if (ce->r.CerTime != 0) {
		/*
		 * Volume reserved.  Verify that it is reserved
		 * to this Archive Set.
		 */
		if (strcmp(asname, ce->r.CerAsname) != 0 ||
		    strcmp(owner, ce->r.CerOwner) != 0 ||
		    strcmp(fsname, ce->r.CerFsname) != 0) {
			return (FALSE);
		}
	}
	if ((as->AsVsnDesc & VD_mask) != VD_list) {
		/*
		 * A single entry.
		 */
		vd = &as->AsVsnDesc;
		vdnumof = 1;
	} else {
		struct VsnList *vls;

		/*
		 * A list of entries.
		 */
		vls = (struct VsnList *)(void *)
		    ((char *)VsnListTable + (as->AsVsnDesc & ~VD_mask));
		vd = vls->VlDesc;
		vdnumof = vls->VlNumof;
	}

	while (vdnumof-- > 0) {
		switch (*vd & VD_mask) {

		case VD_none:
			/*
			 * This implies ".*" or match any VSN.
			 */
			return (TRUE);

		case VD_exp: {
			struct VsnExp *ve;

			/*
			 * A regular expression.
			 */
			ve = (struct VsnExp *)(void *)((char *)VsnExpTable +
			    (*vd & ~VD_mask));
			if (regex(ve->VeExpbuf, ce->CeVsn, NULL) != NULL) {
				return (TRUE);
			}
			}
			break;

		case VD_pool: {
			struct VsnPool *vp;
			int	i;

			/*
			 * A vsn pool.
			 */
			vp = (struct VsnPool *)(void *)
			    ((char *)VsnPoolTable + (*vd & ~VD_mask));
			for (i = 0; i < vp->VpNumof; i++) {
				struct VsnExp *ve;

				ve = (struct VsnExp *)(void *)((char *)
				    VsnExpTable + vp->VpVsnExp[i]);
				if (regex(ve->VeExpbuf, ce->CeVsn,
				    NULL) != NULL) {
					return (TRUE);
				}
			}
			}
			break;

		default:
			break;
		}
		vd++;
	}
	return (FALSE);
}


/*
 * Compare volRef entries.
 */
static int
cmp_VolRef(
	const void *p1,
	const void *p2)
{
	time_t	time1;
	struct VolRefEntry *v1 = (struct VolRefEntry *)p1;
	struct VolRefEntry *v2 = (struct VolRefEntry *)p2;
	struct CatalogEntry *ce1, *ce2;

	ce1 = v1->VrCe;
	ce2 = v2->VrCe;

	if (!RM_VOLUME_USABLE(ce1) && !RM_VOLUME_USABLE(ce2)) {
		return (0);
	}
	if (!RM_VOLUME_USABLE(ce1)) {
		return (1);
	}
	if (!RM_VOLUME_USABLE(ce2)) {
		return (-1);
	}

	/*
	 * Compare label time in ascending order.
	 */
	time1 = ce2->CeLabelTime - ce1->CeLabelTime;
	if (time1 > 0) {
		return (-1);
	}
	if (time1 < 0) {
		return (1);
	}

	/*
	 * Keep VSNs in catalog order.
	 */
	if (v1->VrAln > v2->VrAln) {
		return (1);
	}
	if (v1->VrAln < v2->VrAln) {
		return (-1);
	}
	if (ce1->CeSlot > ce2->CeSlot) {
		return (1);
	}
	if (ce1->CeSlot < ce2->CeSlot) {
		return (-1);
	}
	return (0);
}


/*
 * Make VSN information table.
 * The catalogs for all archive libraries are examined and the table is
 * made.
 */
static void
makeVolRef(void)
{
	struct ArchLibEntry *al;
	struct MediaParamsEntry *mp;
	struct CatalogEntry *ce;
	mtype_t	firstMedia;
	size_t	size;
	int	aln;
	int	asn;
	int	cen;
	int	diskVols;
	int	vrn;

	/*
	 * Compute size of VolRef table and allocate it.
	 */
	vrn = 0;
	diskVols = 0;
	for (aln = 0; aln < ArchLibTable->count; aln++) {
		al = &ArchLibTable->entry[aln];
		if (al->AlFlags & (AL_disk | AL_honeycomb)) {
			diskVols++;
		} else if (catalogValid) {
			cen = CatalogGetEntries(al->AlEq, 0, INT_MAX, &ce);
			if (cen > 0) {
				vrn += cen;
			} else {
				Trace(TR_ERR, "Get catalog entries(%d) failed",
				    al->AlEq);
			}
		}
	}
	if (vrn == 0) {
		/*
		 * Make an empty table.
		 */
		SamMalloc(volRefTable, sizeof (struct VolRef));
		memset(volRefTable, 0, sizeof (struct VolRef));
		volRefTable->count = 0;
		if (diskVols == 0) {
			Trace(TR_MISC, "No archive media available");
		}
		return;
	}

	size = sizeof (struct VolRef) +
	    (vrn - 1) * STRUCT_RND(sizeof (struct VolRefEntry));
	SamMalloc(volRefTable, size);
	memset(volRefTable, 0, size);
	volRefTable->count = vrn;

	/*
	 * Examine each library.
	 */
	vrn = 0;
	*firstMedia = '\0';
	for (aln = 0; aln < ArchLibTable->count; aln++) {
		int	i;

		al = &ArchLibTable->entry[aln];
		if (al->AlFlags & AL_disk) {
			mp = MediaParamsGetEntry("dk");
			if (mp != NULL) {
				mp->MpFlags |= MP_avail;
			}
			continue;
		}
		if (al->AlFlags & AL_honeycomb) {
			mp = MediaParamsGetEntry("cb");
			if (mp != NULL) {
				mp->MpFlags |= MP_avail;
			}
			continue;
		}

		cen =  CatalogGetEntries(al->AlEq, 0, INT_MAX, &ce);
		if (cen <= 0) {
			Trace(TR_ERR, "Get catalog entries(%d) failed",
			    al->AlEq);
			continue;
		}
		for (i = 0; i < cen; i++) {
			struct VolRefEntry *vr;

			if (*firstMedia == '\0') {
				strncpy(firstMedia, ce->CeMtype,
				    sizeof (firstMedia));
			}
			if (ce->CeStatus & CES_inuse) {
				/*
				 * Note that this media type is available.
				 * Set drives available for the media at the
				 * first slot for the archive library.
				 */
				mp = MediaParamsGetEntry(ce->CeMtype);
				if (mp != NULL) {
					mp->MpFlags |= MP_avail;
				}
			}
			vr = &volRefTable->entry[vrn];
			vr->VrAln	= aln;
			vr->VrCe	= ce;
			vrn++;
			ASSERT(vrn <= volRefTable->count);
			ce++;
		}
	}
	ASSERT(vrn == volRefTable->count);
	/*
	 * Replace unknown media in archive sets.
	 */
	for (asn = 1; asn < ArchSetNumof; asn++) {
		struct ArchSet *as;

		as = &ArchSetTable[asn];
		if (strcmp(as->AsMtype, "??") == 0) {
			strncpy(as->AsMtype, firstMedia, sizeof (as->AsMtype));
		}
	}
}


/*
 * Make the disk volume and honeycomb silo reference table.
 * The database is examined and the table constructed.
 */
static void
makeDiskVolRef(void)
{
	struct ArchLibEntry *al;
	struct DiskVolsDictionary *diskVols;
	struct DiskVolumeInfo *dv;
	size_t	size;
	char	*volName;
	int	idx;
	int	numofDiskVols;
	int	aln;

	if (diskVolRefTable != NULL) {
		/*
		 * Discard present table.
		 */
		SamFree(diskVolRefTable);
		diskVolRefTable = NULL;
	}
	numofDiskVols = 0;
	if ((ArchLibTable->AlDkDrives != 0) ||
	    (ArchLibTable->AlHcDrives != 0)) {
		ThreadsDiskVols(B_TRUE);
		diskVols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT,
		    DISKVOLS_RDONLY);
		if (diskVols != NULL) {
			(void) diskVols->Numof(diskVols, &numofDiskVols,
			    DV_numof_all);
		}
	}

	if (numofDiskVols == 0) {
		/*
		 * Make an empty table.
		 */
		SamMalloc(diskVolRefTable, sizeof (struct DiskVolRef));
		memset(diskVolRefTable, 0, sizeof (struct DiskVolRef));
		diskVolRefTable->count = 0;
		if (diskVols != NULL) {
			(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
			ThreadsDiskVols(B_FALSE);
		}
		return;
	}

	/* Find the disk archive library table. */
	aln = -1;
	for (aln = 0; aln < ArchLibTable->count; aln++) {
		al = &ArchLibTable->entry[aln];
		if (al->AlFlags & (AL_disk | AL_honeycomb)) {
			break;
		}
	}

	size = sizeof (struct DiskVolRef) +
	    (numofDiskVols - 1) * STRUCT_RND(sizeof (struct DiskVolRefEntry));
	SamMalloc(diskVolRefTable, size);
	memset(diskVolRefTable, 0, size);
	diskVolRefTable->count = numofDiskVols;

	idx = 0;
	diskVols->BeginIterator(diskVols);
	while (diskVols->GetIterator(diskVols, &volName, &dv) == 0) {
		struct DiskVolRefEntry *vr;

		vr = &diskVolRefTable->entry[idx];
		strncpy(vr->VrVsn, volName, sizeof (vr->VrVsn));
		vr->VrAln = aln;
		idx++;
	}
	diskVols->EndIterator(diskVols);
	(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	ThreadsDiskVols(B_FALSE);
}


/*
 * Set volume info.
 */
static void
setVolumeInfo(
	struct CatalogEntry *ce,
	struct VolInfo *vi)
{
	memmove(vi->VfMtype, ce->CeMtype, sizeof (vi->VfMtype));
	memmove(vi->VfVsn, ce->CeVsn, sizeof (vi->VfVsn));
	vi->VfFlags = 0;
	vi->VfSlot  = ce->CeSlot;
	vi->VfCapacity = (fsize_t)ce->CeCapacity * 1024;
	vi->VfSpace = rmVolSpace(ce->CeMtype, ce->CeSpace, ce->CeCapacity);

	/*
	 * Set status bits.
	 */
	if (ce->r.CerTime != 0) {
		vi->VfFlags |= VF_reserved;
	}
	if (!RM_VOLUME_USABLE(ce)) {
		vi->VfFlags |= VF_unusable;
	} else {
		/*
		 * Volume is usable, see if it is loaded.
		 */
		struct ArchLibEntry *al;
		int	adn;

		al = &ArchLibTable->entry[vi->VfAln];
		for (adn = 0; adn < al->AlDrivesNumof; adn++) {
			struct ArchDriveEntry *ad;

			ad = &al->AlDriveTable[adn];
			if (strcmp(ad->AdVi.VfMtype, vi->VfMtype) == 0 &&
			    strcmp(ad->AdVi.VfVsn, vi->VfVsn) == 0) {
				vi->VfFlags |= VF_loaded;
				break;
			}
		}
	}
}


/*
 * Get removable media volume space.
 */
static fsize_t
rmVolSpace(
	char *mtype,
	fsize_t space,
	fsize_t capacity)
{
	media_t media;
	double	fspace;

	media = sam_atomedia(mtype);
	if (!is_optical(media)) {
		/*
		 * Adjust for unknown size of tape.
		 */
		fspace = ((double)space * 1024.0) -
		    (TAPE_SPACE_FACTOR * (double)capacity * 1024.0);
	} else {
		/*
		 * Optical should be exact, but allow some slack.
		 * Use the old algorithm until we test thoroughly.
		 */
		fspace = (SPACE_DISCOUNT * (double)space * 1024.0) -
		    (SPACE_FACTOR * 1024.0);
	}
	if (fspace <= 0.0) {
		fspace = 0.0;
	}
	return ((fsize_t)fspace);
}


/*
 * Get disk media volume space.
 */
static fsize_t
dkVolSpace(
	struct DiskVolumeInfo *dv)
{
	fsize_t fspace;

	if (DISKVOLS_IS_HONEYCOMB(dv)) {
		fspace = (fsize_t)1.0;
	} else {
		fspace = dv->DvSpace;
	}
	return (fspace);
}
