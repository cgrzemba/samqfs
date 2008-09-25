/*
 *	catalog.h - SAM-FS library catalog information definitions.
 *
 *
 *	Defines the SAM-FS catalog structure and functions.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#ifndef SAM_CATALOG_H
#define	SAM_CATALOG_H

#ifdef sun
#pragma ident "$Revision: 1.15 $"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#define	BARCODE_LEN	36	/* length of barcode w/o null byte */
#define	MAX_CAT		5	/* max number of open catalogs per user */

/*
 * catalog table
 */

struct sam_cat_tbl {
	time_t	audit_time;	/* Audit time */
	int	version;	/* Catalog version number */
	int	count;		/* Number of slots */
	char	media[4];	/* Media type, if entire catalog is one */
};

/*
 * catalog table entry.
 */

struct sam_cat_ent {
	uint_t  type;			/* Type of slot */
	uint_t	status;			/* Catalog entry status */
	char	media[4];		/* Media type */
	char	vsn[32];		/* VSN */
	int	access;			/* Count of accesses */
	uint_t	capacity;		/* Capacity of volume */
	uint_t	space;			/* Space left on volume */
	int	ptoc_fwa;		/* First word address of PTOC */
	int	reserved[3];		/* Reserved space */
	time_t	modification_time;	/* Last modification time */
	time_t	mount_time;		/* Last mount time */
	uchar_t	bar_code[BARCODE_LEN + 1];	/* Bar code (zero filled) */
};

/* slot type definitions */

#define	CATALOG_SLOT_MEDIA		0	/* Slot contains media */
#define	CATALOG_SLOT_DRIVE		1	/* Slot contains a drive */
#define	CATALOG_SLOT_MAIL		2	/* Slot contains a mailbox */

/* defines for status word */

#define	CSP_NEEDS_AUDIT		0x80000000
#define	CSP_INUSE		0x40000000
#define	CSP_LABELED		0x20000000
#define	CSP_BAD_MEDIA		0x10000000

#define	CSP_OCCUPIED		0x08000000
#define	CSP_CLEANING		0x04000000
#define	CSP_BAR_CODE		0x02000000
#define	CSP_WRITEPROTECT	0x01000000

#define	CSP_READ_ONLY		0x00800000
#define	CSP_RECYCLE		0x00400000
#define	CSP_UNAVAIL		0x00200000
#define	CSP_EXPORT		0x00100000

#define	CS_NEEDS_AUDIT(status)	(((status) & CSP_NEEDS_AUDIT) != 0)
#define	CS_INUSE(status)	(((status) & CSP_INUSE) != 0)
#define	CS_LABELED(status)	(((status) & CSP_LABELED) != 0)
#define	CS_BADMEDIA(status)	(((status) & CSP_BAD_MEDIA) != 0)
#define	CS_OCCUPIED(status)	(((status) & CSP_OCCUPIED) != 0)
#define	CS_CLEANING(status)	(((status) & CSP_CLEANING) != 0)
#define	CS_BARCODE(status)	(((status) & CSP_BAR_CODE) != 0)
#define	CS_WRTPROT(status)	(((status) & CSP_WRITEPROTECT) != 0)
#define	CS_RDONLY(status)	(((status) & CSP_READ_ONLY) != 0)
#define	CS_RECYCLE(status)	(((status) & CSP_RECYCLE) != 0)
#define	CS_UNAVAIL(status)	(((status) & CSP_UNAVAIL) != 0)
#define	CS_EXPORT(status)	(((status) & CSP_EXPORT) != 0)


int	sam_opencat(const char *path, struct sam_cat_tbl *buf, size_t bufsize);
int	sam_getcatalog(int cat_handle, uint_t start_slot, uint_t end_slot,
		struct sam_cat_ent *buf, size_t entbufsize);
int	sam_closecat(int cat_handle);

#ifdef  __cplusplus
}
#endif

#endif /* SAM_CATALOG_H */
