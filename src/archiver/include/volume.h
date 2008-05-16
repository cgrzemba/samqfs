/*
 * volume.h - Archiver volume processing definitions.
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

#if !defined(ARCH_VOLUME_H)
#define	ARCH_VOLUME_H

#pragma ident "$Revision: 1.34 $"

/* SAM-FS headers. */
#include "aml/device.h"
#include "aml/diskvols.h"

/* Volume usability and availability catalog entry status bits. */
#define	VOL_UNAVAILABLE_STATUS (CES_needs_audit | CES_cleaning | CES_dupvsn | \
	CES_unavail | CES_non_sam)

#define	RM_VOL_UNUSABLE_STATUS (CES_needs_audit | CES_cleaning | CES_dupvsn | \
	CES_unavail | CES_non_sam | CES_bad_media | CES_read_only | \
	CES_recycle | CES_writeprotect | CES_archfull)

#define	DK_VOL_UNUSABLE_STATUS (DV_unavail | DV_read_only | DV_bad_media | \
	DV_archfull)

/*
 * Information about a volume.
 * Volumes used by the archiver are examined and used based on this information.
 */
struct VolInfo {
	ushort_t VfFlags;
	mtype_t	VfMtype;
	vsn_t	VfVsn;		/* Volume label */
	fsize_t VfCapacity;	/* Capacity off volume */
	fsize_t VfSpace;	/* Space remaining on volume */
	int	VfAln;		/* Archive Library number */
	int	VfSlot;		/* Slot number in library */
};

#define	VF_busy		0x0001	/* busy archiving */
#define	VF_loaded	0x0002	/* loaded */
#define	VF_reserved	0x0004	/* reserved */
#define	VF_unusable	0x0008	/* not usable for archiving */
#define	VF_debug	0x8000

/* Public functions. */
int GetRmArchiveVol(struct ArchSet *as, int VolNum, char *owner, char *fsname,
	struct VolInfo *vi);
int GetDkArchiveVol(struct ArchSet *as, struct DiskVolsDictionary *diskvols,
	int VolNum, struct VolInfo *vi);
void GetDriveVolInfo(dev_ent_t *dev, struct VolInfo *vi);
int GetVolInfo(int VsnNum, struct VolInfo *vi);
boolean_t IsVolumeAvailable(media_t media, vsn_t vsn, int *stgSim);
void PrintVolInfo(FILE *st, struct VolInfo *vi);
int VolumeCatalog(void);
void VolumeCheck(void);
void VolumeConfig(void);

#endif /* !defined(ARCH_VOLUME_H) */
