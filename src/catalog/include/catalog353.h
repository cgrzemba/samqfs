/*
 * catalog.h - Catalog file definitions.
 *
 * Definitions for the catalog file for a media changer library.
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

#ifndef CATALOG_H
#define	CATALOG_H

#pragma ident "$Revision: 1.14 $"

#define	CF_MAGIC 03640030124
#define	CF_VERSION 353		/* Increment whenever catalog format changes */


/* Catalog table entry.  */

struct CatalogEntry {
	uint32_t CeStatus;	/* Status bits */
	media_t	CeMedia;	/* Media type */
	vsn_t	CeVsn;		/* Volume identifier */
	uint32_t CeAccess;	/* Count of accesses */
	uint64_t CeCapacity;	/* Capacity of volume */
	uint64_t CeSpace;	/* Space left on volume */
	uint32_t CeBlockSize;	/* Block size:  Optical media - sector size */
	time_t	CeLabelTime;	/* Time label written */
	time_t	CeModTime;	/* Last modification time */
	time_t	CeMountTime;	/* Last mount time */
	char	CeBarCode[BARCODE_LEN + 1]; /* Bar code (zero filled) */

	uint16_t CeEq;		/* Library equipment number */
	uint32_t CeSlot;	/* Storage slot in library */
	uint16_t CePartition;	/* Partition: D2 Media - partition ID */
				/* Two sided media - side (1 or 2) */

	union {				/* Media dependent */
		uint64_t CePtocFwa;	/* Optical: First word addr of PTOC */
		uint64_t CeLastPos;	/* Tape: Last position found */
	} m;

	union {		/* Archiver reserve information */
		time_t	CerTime;	/* Time reservation made */
		uname_t CerAsname;	/* Archive set */
		uname_t CerOwner;	/* Owner */
		uname_t CerFsname;	/* File system */
	} r;
	int		CeMid;		/* Media id */
};

/* Status field definitions. */

#define	CES_needs_audit	 0x80000000	/* this entry needs to be looked at */
#define	CES_inuse	 0x40000000	/* slot can be unoccupied but in use */
#define	CES_labeled	 0x20000000	/* media is labeled */
#define	CES_bad_media	 0x10000000	/* scanner detected bad media */

#define	CES_occupied	 0x08000000	/* slot occupied */
#define	CES_cleaning	 0x04000000	/* cleaning cartridge in this slot */
#define	CES_bar_code	 0x02000000	/* bar codes in use */
#define	CES_writeprotect 0x01000000	/* Physical write protect */

#define	CES_read_only	 0x00800000	/* User set read only */
#define	CES_recycle	 0x00400000	/* media is to be re-cycled */
#define	CES_unavail	 0x00200000	/* slot is unavailable */
#define	CES_export_slot	 0x00100000	/* slot is an import/export slot */

#define	CES_non_sam	 0x00080000	/* Media is not from sam */
#define	CES_capacity_set 0x00010000	/* User set capacity */
#define	CES_priority	 0x00008000	/* VSN has high priority */


/* Catalog file header. */
struct CatalogHdr {
	uint32_t ChMagic;		/* Catalog magic number */
	int	ChVersion;		/* Catalog version number */
	int	ChNumofEntries;		/* Number of catalog entries */
	time_t	ChAuditTime;		/* Audit time */
	media_t	ChMedia;		/* Media type */
	uint16_t ChEq;			/* Eq number to which catalog belongs */
	struct CatalogEntry ChTable[1];	/* Catalog table - Catalog entry(s) */
};

#endif /* CATALOG_H */
