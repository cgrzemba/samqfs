/*
 * catalog.h - Catalog file definitions.
 *
 * Definitions for the catalog file for a media changer library.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.9 $"

#if	!defined(_AML_CATALOG_H)
#define	_AML_CATALOG_H

#define	CF_MAGIC 03640030124
#define	CF_VERSION 351		/* Increment whenever catalog format changes */

typedef char mtype2_t[3];	/* Use 4.0 mtype_t of 3 characters */

/* Catalog table entry.  */

struct CatalogEntry {
	uint32_t CeStatus;	/* Status bits */
	mtype2_t CeMtype;	/* Media type */
	vsn_t	CeVsn;		/* Volume serial number */
	uint32_t CeSlot;	/* Storage slot in library */
	uint16_t CePart;	/* Partition: Ampex D2 Media - partition ID */
				/* Two sided media - side (1 or 2) */
	uint32_t CeAccess;	/* Count of accesses */
	uint64_t CeCapacity;	/* Capacity of volume */
	uint64_t CeSpace;	/* Space left on volume */
	uint32_t CeBlockSize;	/* Block size:  Optical media - sector size */
	time32_t CeLabelTime;	/* Time label written */
	time32_t CeModTime;	/* Last modification time */
	time32_t CeMountTime;	/* Last mount time */
	char CeBarCode[BARCODE_LEN + 1]; /* Bar code (zero filled) */

	union {		/* Media dependent */
		uint64_t CePtocFwa;	/* Optica: First word addr of PTOC */
		uint64_t CeLastPos;	/* Tape: Last position found */
	} m;

	struct {	/* Archiver reserve information */
		time32_t CerTime;	/* Time reservation made */
		uname_t CerAsname;	/* Archive set */
		uname_t CerOwner;	/* Owner */
		uname_t CerFsname;	/* File system */
	} r;

	/* To be obsoleted */
	uint16_t CeEq;		/* Library equipment number */
	int	CeMid;		/* Media id */
};

/* Status field bit definitions. */
/*
 * NOTE:  CES_inuse and CES_occupied are maintained by the catalog
 * server.
 */
#define	CES_needs_audit 0x80000000	/* this entry needs to be looked at */
#define	CES_inuse	0x40000000	/* slot can be unoccupied but in use */
#define	CES_labeled	0x20000000	/* media is labeled */
#define	CES_bad_media	0x10000000	/* scanner detected bad media */

#define	CES_occupied	0x08000000	/* slot occupied */
#define	CES_cleaning	0x04000000	/* cleaning cartridge in this slot */
#define	CES_bar_code	0x02000000	/* bar codes in use */
#define	CES_writeprotect 0x01000000	/* Physical write protect */

#define	CES_read_only	0x00800000	/* User set read only */
#define	CES_recycle	0x00400000	/* media is to be re-cycled */
#define	CES_unavail	0x00200000	/* slot is unavailable */
#define	CES_export_slot	0x00100000	/* slot is an import/export slot */

#define	CES_non_sam	0x00080000	/* Media is not from sam */
#define	CES_capacity_set 0x00010000	/* User set capacity */

#define	CES_priority	0x00008000	/* VSN has high priority */
#define	CES_dupvsn	0x00004000	/* Duplicate VSN */
#define	CES_reconcile	0x00002000	/* Reconcile catalog/library entries */
#define	CES_partitioned	0x00001000	/* This tape has been partitioned */

#define	CES_archfull	0x00000800	/* Archiver found volume full */

/* Catalog file header. */
struct CatalogHdr {
	uint32_t ChMagic;	/* Catalog magic number */
	int	ChVersion;	/* Catalog version number */
	time_t	ChAuditTime;	/* Audit time */
	uint16_t ChEq;		/* Equipment number to which catalog belongs */
	upath_t ChFname;	/* Name of the catalog file */
	mtype2_t ChMediaType;	/* Media type, if entire catalog is the same */
	int ChNumofEntries;	/* Number of catalog entries */
	enum CH_type ChType;	/* Catalog type */
	struct CatalogEntry ChTable[1]; /* Catalog table - Catalog entry(s) */
};

#endif /* defined(_AML_CATALOG_H) */
