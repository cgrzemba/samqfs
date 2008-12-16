/*
 * catalog.h - Catalog file definitions.
 *
 * Definitions for the catalog file for a media changer library.
 * Separate catalogs are maintained for each library, manual drive,
 * and historian.  These so called 'device catalogs' are files that
 * contain information about each cartridge in the library in an array
 * of structures.  The files also contain additional data for control and
 * verification purposes.  Each catalog entry represents a volume.
 *
 * By default, the files reside in the directory /var/opt/SUNWsamfs/catalog,
 * and are named according to the equipment name (e.g. pm30, man40).
 * The file name may be set by using the mcf.
 *
 * The device catalogs are managed by the catalog manager, a server daemon.
 * The device catalogs treated as a single catalog by the catalog manager
 * for the purpose of media lookup.  The device catalogs can grow in size
 * while in operation.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifndef CATALOG_H
#define	CATALOG_H

#pragma ident "$Revision: 1.16 $"

#define	CF_MAGIC 03640030124
#define	CF_VERSION 349		/* Increment whenever catalog format changes */

#define	MAX_SLOTS 0x7fffffff
#define	MAX_PARTITIONS 1024


/* Volume identification. */

struct VolId349 {
	uint8_t	ViFlags;	/* Valid fields */
	char	ViMtype[3];	/* Media type */
	vsn_t	ViVsn;		/* Volume serial number */
	uint16_t ViEq;		/* Library equipment number */
	uint32_t ViSlot;	/* Storage slot in library */
	uint16_t ViPart;	/* Partition: Ampex D2 Media - partition ID */
				/* Two sided media - side (1 or 2) */
};

/* Volume id field validity flags */
enum VI_fields349 {
	VI_NONE349	= 0,
	VI_mtype349	= 0x01,
	VI_vsn349	= 0x02,
	VI_eq349	= 0x10,
	VI_slot349	= 0x20,
	VI_part349	= 0x40,

	/* Composite definitions. */
	/*
	 * If the definition is not VI_logical, the the VolId is a physical
	 * reference.
	 * A labeled Catalog entry will have flags set to VI_labeled.
	 * An unlabeled Catalog entry will have flags set to VI_cart.
	 */
	VI_logical349	= 0x03,		/* Logical reference */
	VI_cart349	= 0x30,		/* Cartridge - all partitions */
	VI_onepart349	= 0x70,		/* Partition on a cartridge */
	VI_labeled349	= 0x73,		/* Labeled catalog entry */
	VI_MAX349
};


/* Catalog table entry.  */

struct CatalogEntry {
	uint32_t CeStatus;	/* Status bits */
	struct VolId349 i;
	uint32_t CeAccess;	/* Count of accesses */
	uint64_t CeCapacity;	/* Capacity of volume */
	uint64_t CeSpace;	/* Space left on volume */
	uint32_t CeBlockSize;	/* Block size:  Optical media - sector size */
	time_t	CeLabelTime;	/* Time label written */
	time_t	CeModTime;	/* Last modification time */
	time_t	CeMountTime;	/* Last mount time */
	char	CeBarCode[BARCODE_LEN + 1]; /* Bar code (zero filled) */
					/* Two sided media - side (1 or 2) */
	union {	/* Media dependent */
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

/* Status field bit definitions. */
/*
 * NOTE:  CES_inuse and CES_occupied are maintained by the catalog
 * server.
 */
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
#define	CES_dupvsn	 0x00004000	/* Duplicate VSN */


/* Catalog file header. */
/* The header is at the beginning of the catalog file. */

#if 0	/* This is the same in later versions */
/* Catalog file types. */
enum CH_type {
	CH_NONE,
	CH_historian,
	CH_library,
	CH_manual,
	CH_MAX
};
#endif /* 0 */

struct CatalogHdr {
	uint32_t ChMagic;	/* Catalog magic number */
	int	ChVersion;	/* Catalog version number */
	time_t	ChAuditTime;	/* Audit time */
	uint16_t ChEq;		/* Equipment number to which catalog belongs */
	upath_t	ChFname;	/* Name of the catalog file */
	char	ChMediaType[3];	/* Media type, if entire catalog is the same */
	int	ChNumofEntries;	/* Number of catalog entries */
	enum CH_type ChType;	/* Catalog type */
	struct CatalogEntry ChTable[1];	/* Catalog table - Catalog entry(s) */
};

#endif /* CATALOG_H */
