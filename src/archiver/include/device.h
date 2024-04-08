/*
 * device.h - Archiver device processing definitions.
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

#ifndef ARCH_DEVICE_H
#define	ARCH_DEVICE_H

#pragma ident "$Revision: 1.23 $"

/* Local headers. */
#include "volume.h"

/* Tables. */

/*
 * Archive Library table.
 * Each archive library or manually mounted drive is referenced by
 * the archive library structure.
 */
struct ArchLib {
	int	count;
	int	AlDriveCount;
	int	AlDkDrives;
	int	AlHcDrives;
	int	AlRmDrives;
	struct ArchLibEntry {
		uchar_t AlFlags;
		uname_t	AlName;		/* Name for display */
		int	AlDrivesAllow;	/* Number of drives allowed to use */
		int	AlDrivesAvail;	/* Number of drives available to use */
		int	AlDrivesNumof;	/* Number of drives in library */
		int	AlEq;		/* Equipment number */
		int	AlRmFn;		/* Base removable media file number */
		dev_ent_t *AlDevent;	/* Device entry */
		struct ArchDriveEntry {
			uchar_t	AdFlags;
			uname_t	AdName;		/* Name for display */
			dev_ent_t *AdDevent;	/* Device entry */
			struct VolInfo AdVi;	/* Loaded VSN */
		} *AlDriveTable;
	} entry[1];
};

DCL struct ArchLib *ArchLibTable IVAL(NULL);

/* Archive library flags. */
#define	AL_avail	0x01	/* Library is available for scheduling */
#define	AL_manual	0x02	/* Manual drive */
#define	AL_remote	0x04	/* Remote */
#define	AL_historian	0x08	/* Historian */
#define	AL_disk		0x10	/* Disk */
#define	AL_honeycomb	0x20	/* Honeycomb */
#define	AL_sim		0x40	/* Simulate */

/* Used while reading archiver.cmd file */
#define	AL_allow	0x80	/* Drives allowed set */

/* Archive Drive flags. */
#define	AD_avail	0x01	/* Drive is available for scheduling */
#define	AD_busy		0x02	/* Drive is busy */

/* Public functions. */
void DeviceCheck(void);
void DeviceConfig(ArchProg_t caller);

#endif /* ARCH_DEVICE_H */
