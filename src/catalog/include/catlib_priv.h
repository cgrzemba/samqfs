/*
 * catlib_priv.h - private catalog library definitions.
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

#if !defined(CATLIB_PRIV_H)
#define	CATLIB_PRIV_H

#pragma ident "$Revision: 1.17 $"

#define	CT_MAGIC 0030124240214

#define	CATALOG_TABLE_INCR 32
#define	CATALOG_TABLE_MAX 500000

#define	NO_BAR_CODE "NO_BAR_CODE"
#define	NO_INFORMATION "NO_INFORMATION"

#define	CLEANING_BAR_LEN	3
#define	CLEANING_BAR_CODE	"CLN"
#define	CLEANING_FULL_LEN	5
#define	CLEANING_FULL_CODE	"CLEAN"

/*
 * Catalog file data.
 */
struct CatalogMap {
	int	CmEq;			/* SAM equipment number */
	struct CatalogHdr *CmHdr;	/* Location of catalog file header */
	size_t	CmSize;			/* Size of mmap-ped catalog file */
	upath_t CmFname;		/* Catalog file name */
};


/*
 * Catalog table header.
 */
struct CatalogTableHdr {
	MappedFile_t Ct;
	int	CtNumofFiles;	/* Number of catalog files */
	time_t	CtTime;		/* Time file created (change detection) */
	int	CtLastMid;
	upath_t CtFname[1];	/* Catalog file names (dynamic array) */
};

/* Public functions. */
#if defined(CAT_SERVER)
int CatalogAccess(char *TableName, void (*MsgFunc)(int code, char *msg));
int FindCatalog(int eq);
#endif /* defined(CAT_SERVER) */
struct CatalogEntry *_Cl_IfGetCeByMid(int mid, struct CatalogEntry *ce);
struct CatalogEntry *CS_CatalogGetEntry(struct VolId *vid);
struct CatalogEntry *CS_CatalogGetCeByLoc(int eq, int slot, int part);
struct CatalogEntry *CS_CatalogGetCeByMedia(char *media_type, vsn_t vsn);
struct CatalogEntry *CS_CatalogGetCeByBarCode(int eq, char *media_type,
	char *string);

/* Public data. */
extern struct CatalogTableHdr *CatalogTable;
extern struct CatalogMap *Catalogs;

#endif /* defined(CATLIB_PRIV_H) */
