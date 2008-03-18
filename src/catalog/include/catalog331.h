/*
 * catalog331.h - structs and defines for 3.3.1 catalog.
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

#pragma ident "$Revision: 1.14 $"


#ifndef CATALOG331_H
#define	CATALOG331_H

#if !defined(_KERNEL)

#include "sam/types.h"

/* Catalog file/mmap */


#define	CATALOG_SLOT_MEDIA	0	/* Slot contains media */
#define	CATALOG_SLOT_DRIVE	1	/* Slot contains a drive */
#define	CATALOG_SLOT_MAIL	2	/* Slot contains a mailbox */

#define	CATALOG_CYGNET_SIZE	144	/* Max no. of Cygnet slots */
#define	CATALOG_RAPIDCHG_SIZE	6	/* Max no. of RapidChanger slots */

#define	CATALOG_CURRENT_VERSION	1	/* Increment whenever catalog is */
					/* incompatible (needs audit) with */
					/*  previous RELEASED system. */

/*
 * cat_ent_t - catalog table entry.
 */

typedef struct cat_ent_t {
	uint_t  type;		/* Type of slot */
	union {
		struct {
			uint_t
#if	defined(_BIT_FIELDS_HTOL)
			needs_audit:1,	/* slot needs to be looked at */
			slot_inuse:1,	/* slot can be empty but in use */
			labeled:1,	/* media is labeled */
			bad_media:1,	/* scanner detected bad media */

			slot_occupied:1, /* slot occupied */
			cleaning:1,	/* cleaning cartridge in this slot */
			bar_code:1,	/* bar codes in use */
			write_protect:1, /* Physical write protect */

			read_only:1,	/* media in slot is read-only */
					/*  a software flag. */
			recycle:1,	/* media is to be re-cycled */
			unavail:1,	/* slot is unavailable */
			export_slot:1,	/* an import/export slot */

			strange:1,	/* Media is not from sam */
			capacity_set:1,	/* set when user sets capacity */
			priority:1,	/* VSN have high priority  */

			unused:17;
#else	/* defined(_BIT_FIELDS_HTOL) */

			unused:17,

			priority:1,	/* VSN have high priority  */
			capacity_set:1,	/* flag set when user sets capacity */
			strange:1,	/* Media is not from sam */

			export_slot:1,	/* slot is an import/export slot */
			unavail:1,	/* slot is unavailable */
			recycle:1,	/* media is to be re-cycled */
			read_only:1,	/* media in slot is read-only */
					/*  a software flag. */

			write_protect:1, /* Physical write protect */
			bar_code:1,	/* bar codes in use */
			cleaning:1,	/* cleaning cartridge in this slot */
			slot_occupied:1, /* slot occupied */

			bad_media:1,	/* scanner detected bad media */
			labeled:1,	/* media is labeled */
			slot_inuse:1,	/* slot can be empty but in use */
			needs_audit:1;	/* this slot needs to be looked at */
#endif  /* _BIT_FIELDS_HTOL */
		} b;
		uint_t bits;
	} status;

	media_t	media;			/* Media type */
	vsn_t	vsn;			/* Volume identifier */
	int	access;			/* Count of accesses */
	uint_t	capacity;		/* Capacity of volume */
	uint_t	space;			/* Space left on volume */
	int	ptoc_fwa;		/* First word address of PTOC */
	uint_t	sector_size;		/* blocksize/sector size */
	int	res1[1];		/* Just hold some space */
	time_t	label_time;		/* time label writtin on tape */
	time_t	modification_time;	/* Last modification time */
	time_t	mount_time;		/* Last mount time */
	uchar_t	bar_code[BARCODE_LEN + 1]; /* Bar code (zero filled) */
} cat_ent_t;

/* defines for status word */

#define	CLST_NEEDS_AUDIT	0x80000000
#define	CLST_INUSE		0x40000000
#define	CLST_LABELED		0x20000000
#define	CLST_BAD_MEDIA		0x10000000

#define	CLST_OCCUPIED		0x08000000
#define	CLST_CLEANING		0x04000000
#define	CLST_BAR_CODE		0x02000000
#define	CLST_WRITEPROTECT	0x01000000

#define	CLST_READ_ONLY		0x00800000
#define	CLST_RECYCLE		0x00400000
#define	CLST_UNAVAIL		0x00200000
#define	CLST_EXPORT		0x00100000

#define	CLST_STRANGE		0x00080000
#define	CLST_CAPACITY_SET	0x00040000
#define	CLST_PRIORITY		0x00020000

/* catalog table. */

typedef struct catalog_tbl {
	time_t	audit_time;		/* Audit time */
	int	version;		/* Catalog version number */
	int	count;			/* Catalog table size */
	media_t	media;			/* Media type, if entire cat. is one */
	cat_ent_t p[1];			/* Catalog entry(s) */
} catalog_tbl_t;


#endif /* !defined(_KERNEL) */

#endif  /* defined CATALOG331_H */
