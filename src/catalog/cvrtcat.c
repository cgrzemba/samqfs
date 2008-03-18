/*
 * cvrtcat.c - convert catalog files to latest version.
 *
 * Catalog files are converted to the latest version.
 * The original catalog is renamed with the version number appended
 * to the name, a 3.3.1 catalog named pm30_cat becomes pm30_cat.331.
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

#pragma ident "$Revision: 1.16 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#define	MAX_TRIALS 10

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

/* POSIX headers. */
#include "sys/types.h"
#include "fcntl.h"
#include "unistd.h"
#include "sys/stat.h"

/* Solaris headers. */
#include <sys/mman.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "sam/sam_malloc.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/* Local includes. */
#include "catlib_priv.h"
#include "catalog331.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private functions. */
static struct CatalogHdr *Cvrt331(void *mp, int *NumofEntries);
static struct CatalogHdr *Cvrt349(void *mp, int *NumofEntries);
static struct CatalogHdr *Cvrt353(void *mp, int *NumofEntries);
static struct CatalogHdr *Cvrt350(void *mp, int *NumofEntries);
static struct CatalogHdr *Cvrt351(void *mp, int *NumofEntries);

/*
 * Convert catalog to latest version.
 * Backup catalog.
 */
int					/* -1 if unsuccessful */
CvrtCatalog(
	char *cf_name,			/* Catalog file name */
	int version,			/* Version number from catalog */
	void **mp,			/* Mmaped version of catalog file */
	size_t *size)			/* Size of mmaped file */
{
	struct CatalogHdr *ch;
	struct CatalogHdr *ch_cvrt;
	void *mp_unmap;
	char bak_name[sizeof (upath_t) + 10];
	int NumofEntries;
	int size_unmap;
	int retval;
	int trials;

	ch_cvrt = NULL;	/* Initialize for error handler */
	retval = -1;
	mp_unmap = *mp;
	size_unmap = *size;

	/*
	 * Convert the catalog to the latest format.
	 */
	switch (version) {
	case 331:
		ch_cvrt = Cvrt331(*mp, &NumofEntries);
		break;

	case 349:
		ch_cvrt = Cvrt349(*mp, &NumofEntries);
		break;

	case 353:
		ch_cvrt = Cvrt353(*mp, &NumofEntries);
		break;

	case 350:
		ch_cvrt = Cvrt350(*mp, &NumofEntries);
		break;

	case 351:
		ch_cvrt = Cvrt351(*mp, &NumofEntries);
		break;

	default:
		SysError(HERE, "Cannot convert catalog file %s.", cf_name);
		goto error;
	}

	/*
	 * Back up a previous catalog.
	 * Start with the name with version appended.
	 * If this file exists, then try a series with name.version.nums
	 */
	sprintf(bak_name, "%s.%d", cf_name, version);
	for (trials = 1; trials < MAX_TRIALS; trials++) {
		struct stat st;
		int ret;

		ret = stat(bak_name, &st);
		if (ret == -1 && errno == ENOENT)  break;
		sprintf(bak_name, "%s.%d.%d", cf_name, version, trials);
	}
	if (trials >= MAX_TRIALS) {
		SysError(HERE, "Cannot create backup for catalog file %s",
		    cf_name);
		goto error;
	}

	if (rename(cf_name, bak_name) == -1) {
		SysError(HERE, "Cannot rename %s to %s", cf_name, bak_name);
		goto error;
	}
	SendCustMsg(HERE, 18000, cf_name, version, bak_name);

	/*
	 * Create a new catalog file.
	 */
	if (CatalogCreateCatfile(cf_name, NumofEntries, mp, size, NULL) == -1) {
		/* Messages have been sent. */
		goto error;
	}

	/*
	 * Copy the conversion to the mmap-ed catalog.
	 */
	ch = *mp;
	memmove(ch->ChTable, ch_cvrt->ChTable,
	    *size - sizeof (struct CatalogHdr));
	ch->ChAuditTime = ch_cvrt->ChAuditTime;
	memmove(ch->ChMediaType, ch_cvrt->ChMediaType,
	    sizeof (ch->ChMediaType));
	retval = 0;

error:
	if (munmap(mp_unmap, size_unmap) == -1) {
		SysError(HERE, "Cannot munmap %s", bak_name);
	}
	if (ch_cvrt != NULL)  SamFree(ch_cvrt);
	return (retval);
}


/*
 * Convert 3.3.1 catalog to 4.1.
 */
static struct CatalogHdr *
Cvrt331(
	void *mp,			/* Catalog file read */
	int *NumofEntries)
{
	struct catalog_tbl *ct3;
	struct CatalogHdr *ch;
	size_t	size;
	int n;

	ct3 = (struct catalog_tbl *)mp;
	*NumofEntries = ct3->count;
	if (*NumofEntries <= 0) {
		*NumofEntries = CATALOG_TABLE_INCR;
	}
	size = sizeof (struct CatalogHdr) +
	    *NumofEntries * sizeof (struct CatalogEntry);
	SamMalloc(ch, size);
	memset(ch, 0, size);
	ch->ChAuditTime = ct3->audit_time;
	memmove(ch->ChMediaType, sam_mediatoa(ct3->media),
	    sizeof (ch->ChMediaType));

	for (n = 0; n < ct3->count; n++) {
		struct CatalogEntry *ce;
		struct cat_ent_t *ce3;
		media_t	media;

		ce = &ch->ChTable[n];
		ce3 = &ct3->p[n];
		ce->CeStatus = 0;
		ce->CeStatus |= (ce3->status.b.needs_audit) ?
		    CES_needs_audit : 0;
		ce->CeStatus |= (ce3->status.b.slot_inuse) ? CES_inuse  : 0;
		ce->CeStatus |= (ce3->status.b.labeled) ? CES_labeled : 0;
		ce->CeStatus |= (ce3->status.b.bad_media) ? CES_bad_media : 0;

		ce->CeStatus |= (ce3->status.b.slot_occupied) ?
		    CES_occupied : 0;
		ce->CeStatus |= (ce3->status.b.cleaning) ? CES_cleaning : 0;
		ce->CeStatus |= (ce3->status.b.bar_code) ? CES_bar_code : 0;
		ce->CeStatus |= (ce3->status.b.write_protect) ?
		    CES_writeprotect : 0;
		ce->CeStatus |= (ce3->status.b.read_only) ? CES_read_only : 0;
		ce->CeStatus |= (ce3->status.b.recycle) ? CES_recycle : 0;
		ce->CeStatus |= (ce3->status.b.unavail) ? CES_unavail : 0;
		ce->CeStatus |= (ce3->status.b.export_slot) ?
		    CES_export_slot : 0;
		ce->CeStatus |= (ce3->status.b.strange) ? CES_non_sam : 0;
		ce->CeStatus |= (ce3->status.b.capacity_set) ?
		    CES_capacity_set : 0;
		ce->CeStatus |= (ce3->status.b.priority) ? CES_priority : 0;

		memmove(ce->CeMtype, sam_mediatoa(ce3->media),
		    sizeof (ce->CeMtype));
		memmove(ce->CeVsn, ce3->vsn, sizeof (ce->CeVsn));
		ce->CeAccess = ce3->access;
		ce->CeCapacity = ce3->capacity;
		ce->CeSpace = ce3->space;
		ce->CeBlockSize = ce3->sector_size;
		ce->CeLabelTime = ce3->label_time;
		ce->CeModTime = ce3->modification_time;
		ce->CeMountTime = ce3->mount_time;
		memmove(ce->CeBarCode, ce3->bar_code, sizeof (ce->CeBarCode));
		ce->m.CePtocFwa = ce3->ptoc_fwa;

		/*
		 * Catalog slot is based on the 331.
		 * If slot is occupied, use equipment media.
		 */
		if (ce->CeStatus & CES_occupied)  media = ce3->media;
		else  media = ct3->media;
		if (media == DT_ERASABLE ||
		    media == DT_PLASMON_UDO ||
		    media == DT_WORM_OPTICAL ||
		    media == DT_WORM_OPTICAL_12 ||
		    media == DT_OPTICAL) {
			ce->CeSlot = n / 2;
			ce->CePart = 1 + (n % 2);
		} else {
			ce->CeSlot = n;
		}
		ce++;
		ce3++;
	}
	return (ch);
}


/*
 * Convert 349 catalog.
 */
#define	CatalogHdr CatalogHdr349
#define	CatalogEntry CatalogEntry349
#undef CATALOG_H
#undef CF_VERSION
#include <catalog349.h>
#undef CatalogHdr
#undef CatalogEntry

static struct CatalogHdr *
Cvrt349(
	void *mp,			/* Catalog file read */
	int *NumofEntries)
{
	struct CatalogHdr *ch;
	struct CatalogHdr349 *cho;
	size_t	size;
	int n;

	cho = (struct CatalogHdr349 *)mp;
	*NumofEntries = cho->ChNumofEntries;
	size = sizeof (struct CatalogHdr) +
	    *NumofEntries * sizeof (struct CatalogEntry);
	SamMalloc(ch, size);
	memset(ch, 0, size);
	ch->ChAuditTime = cho->ChAuditTime;
	memmove(ch->ChMediaType, cho->ChMediaType, sizeof (ch->ChMediaType));
	ch->ChEq = cho->ChEq;

	for (n = 0; n < *NumofEntries; n++) {
		struct CatalogEntry *ce;
		struct CatalogEntry349 *ceo;

		ce = &ch->ChTable[n];
		ceo = &cho->ChTable[n];
		ce->CeStatus = ceo->CeStatus;
		memmove(ce->CeMtype, ceo->i.ViMtype, sizeof (ce->CeMtype));
		memmove(ce->CeVsn, ceo->i.ViVsn, sizeof (ce->CeVsn));
		ce->CeAccess = ceo->CeAccess;
		ce->CeCapacity = ceo->CeCapacity;
		ce->CeSpace = ceo->CeSpace;
		ce->CeBlockSize = ceo->CeBlockSize;
		ce->CeLabelTime = ceo->CeLabelTime;
		ce->CeModTime = ceo->CeModTime;
		ce->CeMountTime = ceo->CeMountTime;
		memmove(ce->CeBarCode, ceo->CeBarCode, sizeof (ce->CeBarCode));
		ce->CeEq = ceo->i.ViEq;
		ce->CeSlot = ceo->i.ViSlot;
		ce->CePart = ceo->i.ViPart;
		memmove(&ce->m, &ceo->m, sizeof (ce->m));
		memmove(&ce->r, &ceo->r, sizeof (ce->r));

		ce++;
		ceo++;
	}
	return (ch);
}


/*
 * Convert 353 catalog.
 */
#define	CatalogHdr CatalogHdr353
#define	CatalogEntry CatalogEntry353
#undef CATALOG_H
#undef CF_VERSION
#include <catalog353.h>
#undef CatalogHdr
#undef CatalogEntry

static struct CatalogHdr *
Cvrt353(
	void *mp,	/* Catalog file read */
	int *NumofEntries)
{
	struct CatalogHdr *ch;
	struct CatalogHdr353 *cho;
	size_t	size;
	int n;

	cho = (struct CatalogHdr353 *)mp;
	*NumofEntries = cho->ChNumofEntries;
	size = sizeof (struct CatalogHdr) +
	    *NumofEntries * sizeof (struct CatalogEntry);
	SamMalloc(ch, size);
	memset(ch, 0, size);
	ch->ChAuditTime = cho->ChAuditTime;
	memmove(ch->ChMediaType, sam_mediatoa(cho->ChMedia),
	    sizeof (ch->ChMediaType));
	ch->ChEq = cho->ChEq;

	for (n = 0; n < *NumofEntries; n++) {
		struct CatalogEntry *ce;
		struct CatalogEntry353 *ceo;

		ce = &ch->ChTable[n];
		ceo = &cho->ChTable[n];
		ce->CeStatus = ceo->CeStatus;
		memmove(ce->CeMtype, sam_mediatoa(ceo->CeMedia),
		    sizeof (ce->CeMtype));
		memmove(ce->CeVsn, ceo->CeVsn, sizeof (ce->CeVsn));
		ce->CeAccess = ceo->CeAccess;
		ce->CeCapacity = ceo->CeCapacity;
		ce->CeSpace = ceo->CeSpace;
		ce->CeBlockSize = ceo->CeBlockSize;
		ce->CeLabelTime = ceo->CeLabelTime;
		ce->CeModTime = ceo->CeModTime;
		ce->CeMountTime = ceo->CeMountTime;
		memmove(ce->CeBarCode, ceo->CeBarCode, sizeof (ce->CeBarCode));
		ce->CeEq = ceo->CeEq;
		ce->CeSlot = ceo->CeSlot;
		ce->CePart = ceo->CePartition;
		memmove(&ce->m, &ceo->m, sizeof (ce->m));
		memmove(&ce->r, &ceo->r, sizeof (ce->r));

		ce++;
		ceo++;
	}
	return (ch);
}


/*
 * Convert development 350 catalog.
 */
#define	CatalogHdr CatalogHdr350
#define	CatalogEntry CatalogEntry350
#undef CATALOG_H
#undef CF_VERSION
#include <catalog350.h>
#undef CatalogHdr
#undef CatalogEntry

static struct CatalogHdr *
Cvrt350(
	void *mp,		/* Catalog file read */
	int *NumofEntries)
{
	struct CatalogHdr *ch;
	struct CatalogHdr350 *cho;
	size_t	size;
	int		n;

	cho = (struct CatalogHdr350 *)mp;
	*NumofEntries = cho->ChNumofEntries;
	size = sizeof (struct CatalogHdr) +
	    *NumofEntries * sizeof (struct CatalogEntry);
	SamMalloc(ch, size);
	memset(ch, 0, size);
	ch->ChAuditTime = cho->ChAuditTime;
	memmove(ch->ChMediaType, cho->ChMediaType,
	    sizeof (ch->ChMediaType));
	ch->ChEq = cho->ChEq;

	for (n = 0; n < *NumofEntries; n++) {
		struct CatalogEntry *ce;
		struct CatalogEntry350 *ceo;

		ce = &ch->ChTable[n];
		ceo = &cho->ChTable[n];
		ce->CeStatus = ceo->CeStatus;
		memmove(ce->CeMtype, ceo->i.ViMtype, sizeof (ce->CeMtype));
		memmove(ce->CeVsn, ceo->i.ViVsn, sizeof (ce->CeVsn));
		ce->CeEq = ceo->i.ViEq;
		ce->CeSlot = ceo->i.ViSlot;
		ce->CePart = ceo->i.ViPart;
		ce->CeAccess = ceo->CeAccess;
		ce->CeCapacity = ceo->CeCapacity;
		ce->CeSpace = ceo->CeSpace;
		ce->CeBlockSize = ceo->CeBlockSize;
		ce->CeLabelTime = ceo->CeLabelTime;
		ce->CeModTime = ceo->CeModTime;
		ce->CeMountTime = ceo->CeMountTime;
		memmove(ce->CeBarCode, ceo->CeBarCode, sizeof (ce->CeBarCode));
		memmove(&ce->m, &ceo->m, sizeof (ce->m));
		memmove(&ce->r, &ceo->r, sizeof (ce->r));

		ce++;
		ceo++;
	}
	return (ch);
}


/*
 * Convert 351 catalog.
 */
#define	CatalogHdr CatalogHdr351
#define	CatalogEntry CatalogEntry351
#undef _AML_CATALOG_H
#undef CF_VERSION
#include <catalog351.h>
#undef CatalogHdr
#undef CatalogEntry

static struct CatalogHdr *
Cvrt351(
	void *mp,			/* Catalog file read */
	int *NumofEntries)
{
	struct CatalogHdr *ch;
	struct CatalogHdr351 *cho;
	size_t	size;
	int		n;

	cho = (struct CatalogHdr351 *)mp;
	*NumofEntries = cho->ChNumofEntries;
	size = sizeof (struct CatalogHdr) +
	    *NumofEntries * sizeof (struct CatalogEntry);
	SamMalloc(ch, size);
	memset(ch, 0, size);
	ch->ChAuditTime = cho->ChAuditTime;
	memmove(ch->ChMediaType, cho->ChMediaType,
	    sizeof (ch->ChMediaType));
	ch->ChEq = cho->ChEq;

	for (n = 0; n < *NumofEntries; n++) {
		struct CatalogEntry *ce;
		struct CatalogEntry351 *ceo;

		ce = &ch->ChTable[n];
		ceo = &cho->ChTable[n];
		ce->CeStatus = ceo->CeStatus;
		memmove(ce->CeMtype, ceo->CeMtype, sizeof (ce->CeMtype));
		memmove(ce->CeVsn, ceo->CeVsn, sizeof (ce->CeVsn));
		ce->CeEq = ceo->CeEq;
		ce->CeSlot = ceo->CeSlot;
		ce->CePart = ceo->CePart;
		ce->CeAccess = ceo->CeAccess;
		ce->CeCapacity = ceo->CeCapacity;
		ce->CeSpace = ceo->CeSpace;
		ce->CeBlockSize = ceo->CeBlockSize;
		ce->CeLabelTime = ceo->CeLabelTime;
		ce->CeModTime = ceo->CeModTime;
		ce->CeMountTime = ceo->CeMountTime;
		memmove(ce->CeBarCode, ceo->CeBarCode, sizeof (ce->CeBarCode));
		memmove(&ce->m, &ceo->m, sizeof (ce->m));
		memmove(&ce->r, &ceo->r, sizeof (ce->r));

		ce++;
		ceo++;
	}
	return (ch);
}
