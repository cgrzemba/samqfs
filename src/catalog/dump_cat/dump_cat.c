/*
 * dump_cat.c - dump a SAM catalog file in ASCII text.
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

#pragma ident "$Revision: 1.27 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

/* SAM-FS headers. */
#define DEC_INIT
#include "sam/types.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/sam_malloc.h"

/* Local includes. */
#include "catlib_priv.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private functions. */
static void PrintCatalogEntry(int index, struct CatalogEntry *ce);
static int cmp_slot(const void *p1, const void *p2);

/* Private data. */
static boolean_t verbose = FALSE;

int
main(
	int argc,
	char **argv)
{
	struct CatalogHdr *ch;
	struct CatalogEntry **ce_sort;
	struct CatalogEntry ce_blank;
	size_t	size;
	boolean_t show_old = FALSE;
	boolean_t show_count = FALSE;
	void	*mp;
	char	*cf_name;
	char	*program_name;
	int	c;
	int	count;
	int	errors = 0;		/* Errors encountered */
	int	index;
	int	last_slot;
	int	n;
	int	version;		/* Catalog file version number */
	time_t	audit_time;

	program_name = *argv;

	/*
	 * Process arguments.
	 */
	while ((c = getopt(argc, argv, "Von")) != EOF) {
		switch (c) {
		case 'V':
			verbose = TRUE;
			break;
		case 'n':
			show_count = TRUE;
			break;
		case 'o':
			show_old = TRUE;
			break;
		default:
			errors++;
			break;
		}
	}

	/*
	 * Must have only one argument left for the file name.
	 */
	if ((argc - optind) > 1) {
		fprintf(stderr, "%s: %s\n", program_name, GetCustMsg(4600));
		errors++;
	}
	if ((errors != 0) || ((argc - optind) < 1) ||
	    (show_count && (verbose || show_old))) {
		fprintf(stderr,
		    "%s %s [-n] | [-o] [-V] catalog_file\n",
		    GetCustMsg(4601), program_name);
		exit(EXIT_USAGE);
	}

	/*
	 * The next argument is the catalog file.
	 */
	cf_name = argv[optind];

	version = CatalogMmapCatfile(cf_name, O_RDONLY, &mp, &size, NULL);

	/*
	 * Display the count of entries.
	 */
	ch = mp;
	if (show_count) {
		printf("%d\n", ch->ChNumofEntries);
		exit(EXIT_SUCCESS);
	}

	/*
	 * Print the catalog table header.
	 * build_cat ignores lines beginning with "#".
	 */
	ch = mp;

        audit_time = ch->ChAuditTime;
	/* ctime adds \n */
	printf("# audit_time %s", ctime(&audit_time));
	printf("# version %d  count %d mediatype %s\n",
	    ch->ChVersion, ch->ChNumofEntries, ch->ChMediaType);

	if (version != CF_VERSION) {
		fprintf(stderr, GetCustMsg(18450), version, CF_VERSION);
		fprintf(stderr, "\n");
		exit(EXIT_FAILURE);
	}

	printf("%s\n", GetCustMsg(18451));

/*  Print the mid now, just for debugging.  build_cat ignores it. hgm */
/*
 * # Index  VSN  Barcode Type PTOC Access Capacity Space Status Sector
 * Label time Slot Part Modification time Mount time Reserved time
 * Archive-Set/Owner/File-System
 */
	if (verbose) {
		printf("%s\n", GetCustMsg(18452));
/*
 * #      ---status---  ---label time----  --last mod time--  ----mount time---
 */
	}
	printf("#\n");

	/*
	 * Print all the catalog entries in slot order.
	 */
	size = ch->ChNumofEntries * sizeof (struct CatalogEntry *);
	SamMalloc(ce_sort, size);
	count = 0;
	for (n = 0; n < ch->ChNumofEntries; n++) {
		struct CatalogEntry *ce;

		ce = &ch->ChTable[n];
		if (ce->CeStatus & CES_inuse) {
			ce_sort[count++] = &ch->ChTable[n];
		}
	}
	qsort(ce_sort, count, sizeof (struct CatalogEntry *), cmp_slot);
	memset(&ce_blank, 0, sizeof (ce_blank));

	last_slot = 0;
	index = 0;
	for (n = 0; n < count; n++) {
		struct CatalogEntry *ce;

		ce = ce_sort[n];
		while (last_slot+1 < ce->CeSlot) {
			if (show_old) {
				PrintCatalogEntry(index, &ce_blank);
			}
			index++;
			if (strcmp(ch->ChMediaType, "mo") == 0) {
				if (show_old) {
					PrintCatalogEntry(index, &ce_blank);
				}
				index++;
			}
			last_slot++;

		}
		PrintCatalogEntry(index, ce);
		last_slot = ce->CeSlot;
		index++;
	}

	return (EXIT_SUCCESS);
}


/*
 * Print a catalog entry.
 */
void
PrintCatalogEntry(
	int index,
	struct CatalogEntry *ce)
{
	int blanks = FALSE;
	char TmpBarCode[BARCODE_LEN+3];
	char TmpVolInfo[VOLINFO_LEN+3];

	/*
	 * Search the barcode for blanks. If present, place
	 * quotes around the barcode so this output can be used by build_cat.
	 */
	if (strchr(ce->CeBarCode, ' ') != NULL) {
		blanks = TRUE;
	}

	if ((*ce->CeBarCode != '\0') && (ce->CeStatus & CES_bar_code)) {
		if (blanks) {
			sprintf(TmpBarCode, "\"%s\"", ce->CeBarCode);
		} else {
			sprintf(TmpBarCode, "%s", ce->CeBarCode);
		}
	} else {
		sprintf(TmpBarCode, "%s", NO_BAR_CODE);
	}

	/*
	 * Now do the same for the volume information field.
	 */
	blanks = FALSE;
	if (strchr(ce->CeVolInfo, ' ') != NULL) {
		blanks = TRUE;
	}

	if (*ce->CeVolInfo != '\0') {
		if (blanks) {
			sprintf(TmpVolInfo, "\"%s\"", ce->CeVolInfo);
		} else {
			sprintf(TmpVolInfo, "%s", ce->CeVolInfo);
		}
	} else {
		sprintf(TmpVolInfo, "%s", NO_INFORMATION);
	}

	printf(
	    "%c%5d %6s %6s %2s %#10llx %4d %7lld %7lld %#10x %5d %#10x %5d "
	    "%3d %#10x %#10x %#10x %s/%s/%s %s\n",
	    (ce->CeStatus & CES_inuse) ? ' ' : '#',
	    index,
	    (*ce->CeVsn != '\0') ? ce->CeVsn : "?",
	    TmpBarCode,
	    (*ce->CeMtype != '\0') ? ce->CeMtype : "??",
	    ce->m.CePtocFwa,
	    ce->CeAccess,
	    ce->CeCapacity,
	    ce->CeSpace,
	    *(int *)&ce->CeStatus,
	    ce->CeBlockSize,
	    ce->CeLabelTime,
	    ce->CeSlot,
	    ce->CePart,
	    ce->CeModTime,
	    ce->CeMountTime,
	    ce->r.CerTime,
	    ce->r.CerAsname,
	    ce->r.CerOwner,
	    ce->r.CerFsname,
	    TmpVolInfo);

	if (verbose) {

		/*
		 * Print status and times.
		 * build-cat ignores these since they print as comments.
		 */
		time_t	Time;
		struct tm *tm;
		char	buf[80];

		printf("#      %s", CatalogStatusToStr(ce->CeStatus, buf));
		Time = ce->CeLabelTime;
		tm = localtime(&Time);
		strftime(buf, sizeof (buf), "%x %X", tm);
		printf("  %s", buf);
		Time = ce->CeModTime;
		tm = localtime(&Time);
		strftime(buf, sizeof (buf), "%x %X", tm);
		printf("  %s", buf);
		Time = ce->CeMountTime;
		tm = localtime(&Time);
		strftime(buf, sizeof (buf), "%x %X", tm);
		printf("  %s\n", buf);

		/*
		 * ReservedVSNs entry.
		 */
		if (ce->r.CerTime != 0) {
			Time = ce->r.CerTime;
			tm = localtime(&Time);
			strftime(buf, sizeof (buf), "%Y/%m/%d %H:%M:%S", tm);
			printf("#R %s %s %s/%s/%s %s\n",
			    ce->CeMtype,
			    ce->CeVsn,
			    ce->r.CerAsname,
			    ce->r.CerOwner,
			    ce->r.CerFsname,
			    buf);
		}
	}
}


/*
 * Compare slot numbers.
 */
static int
cmp_slot(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry *ce1 = *(struct CatalogEntry **)p1;
	struct CatalogEntry *ce2 = *(struct CatalogEntry **)p2;

	if (ce1->CeSlot < ce2->CeSlot) {
		return (-1);
	}
	if (ce1->CeSlot > ce2->CeSlot) {
		return (-1);
	}
	if (ce1->CePart < ce2->CePart) {
		return (-1);
	}
	if (ce1->CePart > ce2->CePart) {
		return (1);
	}
	return (0);
}
