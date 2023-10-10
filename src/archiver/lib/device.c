/*
 * device.c - Device processing.
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

#pragma ident "$Revision: 1.45 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */

/* ANSI C headers. */
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* Solaris headers. */
#include <sys/shm.h>

/* Solaris headers. */
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/diskvols.h"
#include "sam/lib.h"
#include "aml/robots.h"
#include "aml/shm.h"

/* Local headers. */
#include "common.h"
#include "archset.h"
#include "device.h"
#include "utility.h"
#include "volume.h"
#include "threads.h"

#if defined(lint)
#undef shmdt
#endif

/* Public data. */

/* Private data. */
static dev_ent_t *devhead = NULL;

/* Private functions. */
static void makeArchLib(void);


/*
 * Check devices.
 * Check all libraries.
 */
void
DeviceCheck(void)
{
	int	aln;

	for (aln = 0; aln < ArchLibTable->count; aln++) {
		struct ArchDriveEntry *ad;
		struct ArchLibEntry *al;
		struct dev_ent *dev;
		int	i;

		al = &ArchLibTable->entry[aln];

		/*
		 * Determine library availability.
		 */
		al->AlFlags &= ~AL_avail;
		if (al->AlFlags & (AL_disk | AL_honeycomb)) {
			al->AlFlags |= AL_avail;
			continue;
		} else if (master_shm.shared_memory != NULL) {
			/*
			 * Check only if sam-amld is running.
			 */
			if (al->AlFlags & AL_sim) {
				al->AlFlags |= AL_avail;
			} else {
				/*
				 * Library is available if on, ready and
				 * not auditing.
				 */
				dev = al->AlDevent;
				if (dev != NULL &&
				    dev->state == DEV_ON &&
				    dev->status.b.ready &&
				    !dev->status.b.audit) {
					al->AlFlags |= AL_avail;
				}
			}
		}

		/*
		 * Check each drive belonging to this library.
		 */
		al->AlDrivesAvail = 0;
		for (i = 0, ad = al->AlDriveTable; i < al->AlDrivesNumof;
		    i++, ad++) {
			ad->AdFlags &= ~AD_avail;
			if (al->AlFlags & AL_sim) {
				if (al->AlFlags & AL_avail) {
					ad->AdFlags |= AD_avail;
				}
			} else {
				/*
				 * Drive is available if library is available
				 * and drive is on.
				 */
				if ((al->AlFlags & AL_avail) &&
				    ad->AdDevent->state == DEV_ON) {
					ad->AdFlags |= AD_avail;
					if (ad->AdDevent->status.b.ready) {
						GetDriveVolInfo(ad->AdDevent,
						    &ad->AdVi);
					}
				}
			}
			if (ad->AdFlags & AD_avail) {
				al->AlDrivesAvail++;
			} else {
				memset(&ad->AdVi, 0, sizeof (struct VolInfo));
			}
		}
	}
}


/*
 * Configure device module.
 * Make device tables.
 */
void
DeviceConfig(
	ArchProg_t caller)
{
	Trace(TR_DEBUG, "DeviceConfig(%d)", caller);
	if (master_shm.shared_memory != NULL && shm_ptr_tbl->valid == 0) {
		/*
		 * Stale master shared memory segment.
		 */
		if (shmdt(master_shm.shared_memory) == -1) {
			Trace(TR_ERR, "Cannot detach master shared memory.");
		}
		master_shm.shared_memory = NULL;
	}
	if (master_shm.shared_memory == NULL) {
		(void) ShmatSamfs(O_RDONLY);
	}
	if (master_shm.shared_memory != NULL) {
		devhead =
		    (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
	} else {
		devhead = NULL;
	}
	if (master_shm.shared_memory == NULL || 
	    (devhead == NULL && caller == AP_archiver)) {
		/*
		 * Use the mcf if sam-amld's shared memory segment
		 * is not available.
		 */
		struct dev_ent *dev;
		struct dev_ent *next;
		int	count;
		int	high_eq;

		count = read_mcf(NULL, &devhead, &high_eq);
		if (count <= 0) {
			LibFatal(read_mcf, "");
		}
		for (dev = devhead; dev != NULL; dev = next) {
			next = dev->next;
			dev->next =
			    (struct dev_ent *)(void *)SHM_GET_OFFS(next);
		}
	}
	makeArchLib();
	DeviceCheck();
}


/* Private functions. */


/*
 * Make Archive Library table.
 */
static void
makeArchLib(void)
{
	struct ArchLib *oldArchLibTable;
	struct dev_ent *dev;
	struct DiskVolsDictionary *diskVols;
	size_t	size;
	int	aln;
	int	diskVolumeNumof;
	int	honeycombNumof;
	int	manualDrives;

	/*
	 * Count all libraries and manual drives.
	 * Make the archive library tables.
	 */
	aln = 0;
	manualDrives = 0;
	for (dev = devhead; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {

		if (dev->equ_type == DT_HISTORIAN) {
#if defined(USE_HISTORIAN)
			aln++;
#endif /* defined(USE_HISTORIAN) */
			continue;
		} else if (IS_ROBOT(dev) || dev->equ_type == DT_PSEUDO_SC) {
			aln++;
		} else if (dev->fseq == 0) {
			/*
			 * Manual drive.
			 */
			aln++;
			manualDrives++;
		}
	}

	diskVolumeNumof = 0;
	honeycombNumof  = 0;

	ThreadsDiskVols(B_TRUE);
	diskVols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT,
	    DISKVOLS_RDONLY);
	if (diskVols != NULL) {
		(void) diskVols->Numof(diskVols, &diskVolumeNumof,
		    DV_numof_disk);
		(void) diskVols->Numof(diskVols, &honeycombNumof,
		    DV_numof_honeycomb);
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	}
	ThreadsDiskVols(B_FALSE);

	/*
	 * Create libraries for disk archive and honeycomb data silo.
	 */
	if (diskVolumeNumof > 0) {
		aln++;
	}
	if (honeycombNumof > 0) {
		aln++;
	}

	/*
	 * If there are no libraries, just issue a message.
	 */
	if (aln == 0) {
		SendCustMsg(HERE, 4002);
		aln = 1;
	}

	/*
	 * Allocate library and drive tables and enter the data. If the
	 * table count is zero, allocate one table entry anyway.
	 */
	size = sizeof (struct ArchLib) +
	    (aln - 1) * STRUCT_RND(sizeof (struct ArchLibEntry));
	oldArchLibTable = ArchLibTable;

	SamMalloc(ArchLibTable, size);

	memset(ArchLibTable, 0, size);
	ArchLibTable->count = aln;
	ArchLibTable->AlDkDrives = diskVolumeNumof;
	ArchLibTable->AlHcDrives = honeycombNumof;

	aln = 0;
	for (dev = devhead; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
		struct ArchDriveEntry *ad;
		struct ArchLibEntry *al;

		al = &ArchLibTable->entry[aln];
		if (dev->type == DT_HISTORIAN) {
#if defined(USE_HISTORIAN)
			al->AlFlags  = 0;
			strncpy(al->AlName, "Historian", sizeof (al->AlName));
			al->AlFlags |= AL_historian;
			al->AlDrivesNumof = manualDrives;
#endif /* defined(USE_HISTORIAN) */
			continue;
		} else if (IS_ROBOT(dev) || dev->equ_type == DT_PSEUDO_SC) {
			dev_ent_t *de;

			memmove(al->AlName, dev->set, sizeof (al->AlName));
			al->AlFlags  = 0;
			if (dev->equ_type == DT_PSEUDO_SC) {
				al->AlFlags |= AL_remote;
			}
#if defined(DEBUG)
			if (strncmp(al->AlName, "SimLib", 6) == 0) {
				al->AlFlags |= AL_sim;
			}
#endif /* defined(DEBUG) */
			al->AlRmFn = ArchLibTable->AlRmDrives;

			/*
			 * Count all drives belonging to library.
			 */
			for (de = devhead; de != NULL;
			    de = (struct dev_ent *)SHM_REF_ADDR(de->next)) {

				if (de->eq != dev->eq && de->fseq == dev->eq) {
					al->AlDrivesNumof++;
				}
			}

			/*
			 * Make drive table.
			 */
			size = al->AlDrivesNumof *
			    sizeof (struct ArchDriveEntry);
			SamMalloc(al->AlDriveTable, size);
			memset(al->AlDriveTable, 0, size);
			ad = &al->AlDriveTable[0];
			for (de = devhead; de != NULL;
			    de = (struct dev_ent *)SHM_REF_ADDR(de->next)) {
				if (de->eq != dev->eq && de->fseq == dev->eq) {
					snprintf(ad->AdName,
					    sizeof (ad->AdName), "%s%d",
					    sam_mediatoa(de->type), de->eq);
					ad->AdDevent = de;
					ad++;
				}
			}
			ArchLibTable->AlDriveCount += al->AlDrivesNumof;
			ArchLibTable->AlRmDrives += al->AlDrivesNumof;
		} else if (dev->fseq == 0) {
			/*
			 * Manual drive.
			 */
			snprintf(al->AlName, sizeof (al->AlName), "man%d",
			    dev->eq);
			al->AlFlags = AL_manual;
			al->AlDrivesNumof = 1;
			al->AlRmFn = ArchLibTable->AlRmDrives;
			size = sizeof (struct ArchDriveEntry);
			SamMalloc(al->AlDriveTable, size);
			memset(al->AlDriveTable, 0, size);
			ad = &al->AlDriveTable[0];
			snprintf(ad->AdName, sizeof (ad->AdName), "man%d",
			    dev->eq);
			ad->AdDevent = dev;
			ArchLibTable->AlDriveCount++;
			ArchLibTable->AlRmDrives++;
		} else {
			continue;
		}

		al->AlDevent = dev;
		al->AlEq = dev->eq;
		al->AlDrivesAvail = al->AlDrivesNumof;
		al->AlDrivesAllow = al->AlDrivesNumof;
		aln++;
		ASSERT(aln <= ArchLibTable->count);
	}

	if (diskVolumeNumof > 0) {
		struct ArchLibEntry *al;

		ArchLibTable->AlDriveCount += diskVolumeNumof;
		al = &ArchLibTable->entry[aln];
		strncpy(al->AlName, "disk", sizeof (al->AlName)-1);
		al->AlFlags = AL_disk;
		al->AlDrivesNumof = diskVolumeNumof;
		al->AlDrivesAvail = diskVolumeNumof;
		al->AlDrivesAllow = diskVolumeNumof;
		aln++;
	}

	if (honeycombNumof > 0) {
		struct ArchLibEntry *al;

		ArchLibTable->AlDriveCount += honeycombNumof;
		al = &ArchLibTable->entry[aln];
		strncpy(al->AlName, "stk5800", sizeof (al->AlName)-1);
		al->AlFlags = AL_honeycomb;
		al->AlDrivesNumof = honeycombNumof;
		al->AlDrivesAvail = honeycombNumof;
		al->AlDrivesAllow = honeycombNumof;
		aln++;
	}

	ASSERT(aln <= ArchLibTable->count);
	if (oldArchLibTable != NULL) {
		/*
		 * Copy AlDrivesAllow from old ArchLibTable.
		 */
		int	i;

		for (aln = 0; aln < ArchLibTable->count; aln++) {
			struct ArchLibEntry *al;

			al = &ArchLibTable->entry[aln];
			for (i = 0; i < oldArchLibTable->count; i++) {
				if (strcmp(oldArchLibTable->entry[i].AlName,
				    al->AlName) == 0) {
					al->AlDrivesAllow =
					    oldArchLibTable->
					    entry[i].AlDrivesAllow;
					break;
				}
			}
		}
		for (i = 0; i < oldArchLibTable->count; i++) {
			struct ArchLibEntry *al;

			al = &oldArchLibTable->entry[i];
			if (al->AlDriveTable != NULL) {
				SamFree(al->AlDriveTable);
			}
		}
		SamFree(oldArchLibTable);
	}
}
