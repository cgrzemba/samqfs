/*
 * dis_catalog.c - Display robot catalog.
 *
 * Displays the contents of the vsn catalog for the
 * selected robotic device.
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


#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "aml/device.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "samu.h"

#define	MIN(x, y)		((x) < (y) ? (x) : (y))

enum csort_key_e {
	SK_SLOT,
	SK_COUNT,
	SK_USE,
	SK_VSN,
	SK_ACCESS,
	SK_BARCODE,
	SK_LABELTM,
	SK_LAST
};

enum search_type_e {
	ST_NONE = 0,
	ST_BARCODE,
	ST_VSN,
	ST_SLOT
};

/* Private data. */
static int ord = 0;		/* Entry ordinal */
static int count = 0;		/* previous count of slots in catalog */
static int details = 0; /* Detailed display: 1 = label etc, 2 = reserve info */
static int last_slot;
static int newcount;	/* count of slots in catalog */
static int slots_inuse;	/* slots containing cartridges */
static int rb = -1;	/* Current robot device */
static enum csort_key_e sort_key = SK_SLOT;	/* key for sorting display */
static int key_title[] = {
						7340 /* "slot       " */,
						7341 /* "count      " */,
						7342 /* "% used     " */,
						7343 /* "VSN        " */,
						7344 /* "access time" */,
						7345 /* "barcode    " */,
						7346 /* "label time " */,
						7347 /* "           " */
};

/* sorted pointers to catalog entries */
static	struct CatalogEntry **sort_index = NULL;

static char search_str[BARCODE_LEN + 1];
static int   search_type = ST_NONE;

/* Private functions. */

static int
Cmp_Barcode(const void *p1, const void *p2);
static int
Cmp_Count(const void *p1, const void *p2);
static int
Cmp_Ea(const void *p1, const void *p2);
static int
Cmp_LbTime(const void *p1, const void *p2);
static int
Cmp_MtTime(const void *p1, const void *p2);
static int
Cmp_Usage(const void *p1, const void *p2);
static int
Cmp_Vsn(const void *p1, const void *p2);

static int samu_get_space_percent(struct CatalogEntry *cv);


void
DisCatalog()
{
	equ_t eq;			/* Device equipment ordinal */
	int n;				/* Display ordinal */
	int slot;			/* Slot ordinal */
	struct CatalogEntry		*cv;
	static struct CatalogEntry *list = NULL;

	eq = DisRb;
	CatlibInit();

	newcount = CatalogGetEntries(eq, 0, INT_MAX, &list);
	if (newcount <= 0) {
		switch (errno) {
		case EINVAL:
		case ENOENT:
			Error(catgets(catfd, SET, 79,
			    "%d is not a robotic device.\n"), eq);
		default:
			return;
		}
	}

	Mvprintw(0, 0, catgets(catfd, SET, 2141,
	    "Robot VSN catalog by %s: eq %d"),
	    catgets(catfd, SET, key_title[sort_key], " "), eq);
	if (sort_key == SK_BARCODE) {
		Mvprintw(2, 0,
		    catgets(catfd, SET, 2352,
		    "slot     barcode          "
		    "count use   flags         ty vsn"));
	} else if (sort_key == SK_LABELTM) {
		Mvprintw(2, 0,
		    catgets(catfd, SET, 2353,
		    "slot           label time "
		    "count use   flags         ty vsn"));
	} else {
		Mvprintw(2, 0,
		    catgets(catfd, SET, 2354,
		    "slot          access time "
		    "count use   flags         ty vsn"));
	}
	if (details == 1) {
		Mvprintw(3, 0,
		    catgets(catfd, SET, 7328,
		    "              label  time    capacity      "
		    "space  blocksize barcode"));
	} else if (details == 2) {
		Mvprintw(3, 0,
		    catgets(catfd, SET, 7348,
		    "         reservation time reserved "
		    "archive set/owner/filesystem location"));
	} else {
		Mvprintw(3, 0, "");
	}
	ln = 4;

	/* robot or size changed */
	if ((DisRb != rb) || (newcount != count)) {

		rb = DisRb;
		ord = 0;

		if (sort_index) {
			free(sort_index);
			sort_index = NULL;
		}
		if (newcount > 0) {
			sort_index = (struct CatalogEntry **)
			    malloc(newcount *
			    sizeof (struct CatalogEntry *));

			if (sort_index == NULL) {
				Error("cannot malloc sort index (%d)",
				    newcount);
			}
		}
		count = newcount;
	}
	Mvprintw(1, 60, catgets(catfd, SET, 766, "count %d"), count);

	if (count == 0)
		return;

	/* generate indices in order */

	slots_inuse = 0;
	for (slot = 0; slot < count; slot++) {
		cv = &list[slot];

		if (cv->CeStatus & CES_inuse) {
			sort_index[slots_inuse] = cv;
			slots_inuse++;
		}
	}

	if (slots_inuse == 0)
		return;
	switch (sort_key) {

	default:
	case SK_SLOT:
		qsort(sort_index, slots_inuse,
		    sizeof (struct CatalogEntry *), Cmp_Ea);
		break;
	case SK_ACCESS:
		qsort(sort_index, slots_inuse,
		    sizeof (struct CatalogEntry *), Cmp_MtTime);
		break;
	case SK_COUNT:
		qsort(sort_index, slots_inuse,
		    sizeof (struct CatalogEntry *), Cmp_Count);
		break;
	case SK_USE:
		qsort(sort_index, slots_inuse,
		    sizeof (struct CatalogEntry *), Cmp_Usage);
		break;
	case SK_VSN:
		qsort(sort_index, slots_inuse,
		    sizeof (struct CatalogEntry *), Cmp_Vsn);
		break;
	case SK_BARCODE:
		qsort(sort_index, slots_inuse,
		    sizeof (struct CatalogEntry *), Cmp_Barcode);
		break;
	case SK_LABELTM:
		qsort(sort_index, slots_inuse,
		    sizeof (struct CatalogEntry *), Cmp_LbTime);
		break;
	}

	if (search_type == ST_BARCODE) {
		int len;

		len = MIN(strlen(search_str), BARCODE_LEN);
		for (n = 0; n < slots_inuse; n++) {
			if (strncmp(search_str,
			    sort_index[n]->CeBarCode, len) == 0) {
				ord = n;
				break;
			}
		}
		if (n == slots_inuse) {
			Mvprintw(LINES - 1, 0,
			    catgets(catfd, SET, 7354,
			    "barcode %s not found"), search_str);
		}
		search_type = ST_NONE;
	} else if (search_type == ST_VSN) {
		int len;

		len = MIN(strlen(search_str), sizeof (vsn_t));
		for (n = 0; n < slots_inuse; n++) {
			if (strncmp(search_str,
			    sort_index[n]->CeVsn, len) == 0) {
				ord = n;
				break;
			}
		}
		if (n == slots_inuse) {
			Mvprintw(LINES - 1, 0,
			    catgets(catfd, SET, 7357,
			    "VSN %s not found"), search_str);
		}
		search_type = ST_NONE;
	} else if (search_type == ST_SLOT) {
		int sslot;

		sslot = strtol(search_str, 0, 10);
		for (n = 0; n < slots_inuse; n++) {
			if (sort_index[n]->CeSlot == sslot) {
				ord = n;
				break;
			}
		}
		if (n == slots_inuse) {
			Mvprintw(LINES - 1, 0,
			    catgets(catfd, SET, 7359,
			    "Slot %s not found"), search_str);
		}
		search_type = ST_NONE;
	}

	last_slot = sort_index[ord]->CeSlot - 1;

	for (n = ord; n < slots_inuse; n++) {

		char *nm, *t_str = "";
		char label_time[40];
		char mount_time[40];
		char reservation_time[40];
		char status_str[32];
		char spart[20];
		int pct;
		int part;

		if (ln > LINES - 3) {
			Mvprintw(ln++, 0, catgets(catfd, SET, 2, "     more"));
			break;
		}
		cv = sort_index[n];
		if (cv->CeMid == ROBOT_NO_SLOT)
			continue;	/* shouldn't happen */

		if ((cv->CeSlot > last_slot+1) && (sort_key == SK_SLOT)) {
			Mvprintw(ln++, 0, "%4d ", last_slot+1);
		}
		last_slot = cv->CeSlot;
		pct = samu_get_space_percent(cv);
		nm = cv->CeMtype;
		(void) TimeString(cv->CeMountTime, mount_time,
		    sizeof (mount_time));
		(void) TimeString(cv->CeLabelTime, label_time,
		    sizeof (label_time));
		part = cv->CePart;
		if (part == 0) {
			strncpy(spart, "     ", 6);
		} else {
			sprintf(spart, ":%-3d ", part);
		}

		if (sort_key == SK_BARCODE) {
			Mvprintw(ln, 0,
			    "%4d%5s%-16s%5d %3d%% ",
			    cv->CeSlot,
			    spart,
			    cv->CeBarCode,
			    cv->CeAccess, pct);
		} else if (sort_key == SK_LABELTM) {
			Mvprintw(ln, 0,
			    "%4d%5s%-16s%5d %3d%% ",
			    cv->CeSlot,
			    spart,
			    label_time,
			    cv->CeAccess, pct);
		} else {
			Mvprintw(ln, 0,
			    "%4d%5s%-16s%5d %3d%% ",
			    cv->CeSlot,
			    spart,
			    mount_time,
			    cv->CeAccess, pct);
		}
		Mvprintw(ln, 36,
		    "%12s  %2s %s",
		    CatalogStatusToStr(cv->CeStatus, status_str),
		    nm,
		    (cv->CeVsn[0] != '\0') ? cv->CeVsn : "nolabel");
		Clrtoeol();

		if ((cv->CeStatus & CES_labeled) && cv->CeVsn[0] == '\0') {
			t_str = catgets(catfd, SET, 2880, "VSN MISSING");
		}
		if ((cv->CeStatus & CES_occupied) && cv->CeVsn[0] == '\0') {
		t_str = catgets(catfd, SET, 1767, "NO LABEL");
		}
		if (cv->CeStatus & CES_needs_audit)
		t_str = catgets(catfd, SET, 1737, "NEEDS AUDIT");
		if (cv->CeStatus & CES_bad_media)
		t_str = catgets(catfd, SET, 1634, "MEDIA ERROR");
		if (cv->CeStatus & CES_non_sam)
		t_str = "NOT SAM MEDIA";

		Mvprintw(ln, 68, t_str);
		ln++;
		if (details == 1) {
			char blksz[14], cap[14], spc[14];

			(void) StrFromFsize(cv->CeBlockSize, 0, blksz,
			    sizeof (blksz));
			strcpy(cap, FsizeToB(cv->CeCapacity * 1024ULL));
			strcpy(spc, FsizeToB(cv->CeSpace * 1024ULL));
			Mvprintw(ln++, 0,
			    "         %-14s  %10s %10s %10s %-31s",
			    label_time, cap, spc, blksz, cv->CeBarCode);
		} else if (details == 2) {
			if ((cv->r.CerTime != 0) ||
			    (cv->CeVolInfo[0] != '\0')) {
				(void) TimeString(cv->r.CerTime,
				    reservation_time,
				    sizeof (reservation_time));
				Mvprintw(ln++, 0,
				    "      R  %-14s  %s/%s/%s %s",
				    reservation_time,
				    cv->r.CerAsname, cv->r.CerOwner,
				    cv->r.CerFsname,
				    cv->CeVolInfo);
			} else {
				Mvprintw(ln++, 0, "");
			}
		}

	}
}


/*
 * Display initialization.
 */
boolean
InitCatalog(
	void)
{

	if (Argc > 1) {
		int ac;

		DisRb = finddev(Argv[1]);
		for (ac = 2; ac < Argc; ac++) {
			switch (*Argv[ac]) {
			case 'I':
				if (++details > 2) details = 0;
				break;
			case '1':
				sort_key = SK_SLOT;
				break;
			case '2':
				sort_key = SK_COUNT;
				break;
			case '3':
				sort_key = SK_USE;
				break;
			case '4':
				sort_key = SK_VSN;
				break;
			case '5':
				sort_key = SK_ACCESS;
				break;
			case '6':
				sort_key = SK_BARCODE;
				break;
			case '7':
				sort_key = SK_LABELTM;
				break;
			}
		}
	}
	search_str[0] = '\0';
	search_type = ST_NONE;
	CatlibInit();
	return (TRUE);
}

/*
 * Compare barcodes.
 */
static int
Cmp_Barcode(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry **cv1 = (struct CatalogEntry **)p1;
	struct CatalogEntry **cv2 = (struct CatalogEntry **)p2;

	return (strcmp((*cv1)->CeBarCode, (*cv2)->CeBarCode));
}

/*
 * Compare access time descending.
 */
static int
Cmp_MtTime(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry **cv1 = (struct CatalogEntry **)p1;
	struct CatalogEntry **cv2 = (struct CatalogEntry **)p2;
	time_t t1 = (*cv2)->CeMountTime - (*cv1)->CeMountTime;
	if (t1 < 0)
		return (-1);
	if (t1 == 0)
		return (0);
	return (1);
}

/*
 * Compare access count.
 */
static int
Cmp_Count(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry **cv1 = (struct CatalogEntry **)p1;
	struct CatalogEntry **cv2 = (struct CatalogEntry **)p2;
	int t1 = (*cv1)->CeAccess - (*cv2)->CeAccess;

	if (t1 < 0)
		return (-1);
	if (t1 == 0)
		return (0);
	return (1);
}

static int
Cmp_Ea(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry **cv1 = (struct CatalogEntry **)p1;
	struct CatalogEntry **cv2 = (struct CatalogEntry **)p2;
	int t1 = (*cv1)->CeSlot - (*cv2)->CeSlot;

	if (t1 < 0)
		return (-1);
	if (t1 == 0) {
		int	part1, part2;

		part1 = (*cv1)->CePart;
		part2 = (*cv2)->CePart;
		t1 = part1 - part2;
		if (t1 < 0)
			return (-1);
		if (t1 == 0)
			return (0);
	}
	return (1);
}

/*
 * Compare label time descending.
 */
static int
Cmp_LbTime(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry **cv1 = (struct CatalogEntry **)p1;
	struct CatalogEntry **cv2 = (struct CatalogEntry **)p2;
	time_t t1 = (*cv2)->CeLabelTime - (*cv1)->CeLabelTime;

	if (t1 < 0)
		return (-1);
	if (t1 == 0)
		return (0);
	return (1);
}

/*
 * Compare usage.
 */
static int
Cmp_Usage(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry **cv1 = (struct CatalogEntry **)p1;
	struct CatalogEntry **cv2 = (struct CatalogEntry **)p2;
	int	pct1, pct2, pct;

	if ((*cv1)->CeStatus & CES_inuse) {
		pct1 = samu_get_space_percent(*cv1);
	} else  pct1 = 0;

	if ((*cv2)->CeStatus & CES_inuse) {
		pct2 = samu_get_space_percent(*cv2);
	} else  pct2 = 0;

	pct = (pct2 - pct1);
	if (pct < 0)
		return (-1);
	if (pct == 0)
		return (0);
	return (1);
}

/*
 * Compare VSNs.
 */
static int
Cmp_Vsn(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry **cv1 = (struct CatalogEntry **)p1;
	struct CatalogEntry **cv2 = (struct CatalogEntry **)p2;

	return (strcmp((*cv1)->CeVsn, (*cv2)->CeVsn));
}

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)

/*
 * Keyboard processing.
 */
boolean
KeyCatalog(
	char key)
{
	boolean fwd = TRUE;
	int n;
	int lastslot;
	int tlines;

	if (details > 0) {
		tlines = HLINES;
	} else {
		tlines = TLINES;
	}

	switch (key) {

	case '1':
		sort_key = SK_SLOT;
		break;
	case '2':
		sort_key = SK_COUNT;
		break;
	case '3':
		sort_key = SK_USE;
		break;
	case '4':
		sort_key = SK_VSN;
		break;
	case '5':
		sort_key = SK_ACCESS;
		break;
	case '6':
		sort_key = SK_BARCODE;
		break;
	case '7':
		sort_key = SK_LABELTM;
		break;

	case '%':
		Mvprintw(LINES - 1, 0,
		    catgets(catfd, SET, 7356, "Search for barcode: "));
		Clrtoeol();
		cbreak();
		echo();
		getstr(search_str);
		noecho();
		Mvprintw(LINES - 1, 0, " ");
		search_type = ST_BARCODE;
		sort_key = SK_BARCODE;
		break;

	case '/':
		Mvprintw(LINES - 1, 0,
		    catgets(catfd, SET, 7355, "Search for VSN: "));
		Clrtoeol();
		cbreak();
		echo();
		getstr(search_str);
		noecho();
		Mvprintw(LINES - 1, 0, " ");
		search_type = ST_VSN;
		sort_key = SK_VSN;
		break;

	case '$':
		Mvprintw(LINES - 1, 0,
		    catgets(catfd, SET, 7358, "Search for slot: "));
		Clrtoeol();
		cbreak();
		echo();
		getstr(search_str);
		noecho();
		Mvprintw(LINES - 1, 0, " ");
		search_type = ST_SLOT;
		sort_key = SK_SLOT;
		break;

	case KEY_details:
		if (++details > 2) details = 0;
		break;

	case KEY_adv_fmt:
		if (sort_key < SK_LAST-1) {
			sort_key++;
		} else {
			sort_key = SK_SLOT;
		}
		break;

	case KEY_full_fwd:
		if (sort_index == NULL)
			return (FALSE);
		if (slots_inuse == 0)
			return (FALSE);

		lastslot = sort_index[ord]->CeSlot - 1;
		for (n = 0; n < tlines; ord++) {

			if (ord >= slots_inuse) {
				ord = 0;
				break;
			}
			if (sort_key == SK_SLOT &&
			    sort_index[ord]->CeSlot > lastslot + 1) {
				n++;
			}
			n++;
			lastslot = sort_index[ord]->CeSlot;
		}
		if (ord >= slots_inuse) ord = 0;
		break;

	case KEY_full_bwd:
		if (slots_inuse == 0)
			return (FALSE);
		if (sort_index == NULL)
			return (FALSE);
		if (ord == 0) {
			ord = slots_inuse - 1;
			break;
		}
		if (ord >= slots_inuse) ord = slots_inuse - 1;
		lastslot = sort_index[ord]->CeSlot + 1;
		for (n = 0; n < tlines; ord--) {
			if (ord <= 0)  {
				ord = 0;
				break;
			}
			if (sort_key == SK_SLOT &&
			    sort_index[ord]->CeSlot < lastslot - 1) {
					n++;
			}
			n++;
			lastslot = sort_index[ord]->CeSlot;
		}
		if (ord <= 0) ord = 0;
		break;

	case KEY_half_bwd:
		fwd = FALSE;
		/* FALLTHROUGH */
	case KEY_half_fwd: {	/* Go to next robot */
		dev_ent_t *dev;

		n = (int)DisRb;
		while (((fwd) ? ++n : --n) != (int)DisRb) {
			if (n > Max_Devices)  n = 0;
			if (n < 0)  n = Max_Devices;
			if (Dev_Tbl->d_ent[n] == NULL)
				continue;
			dev = (dev_ent_t *)
			    SHM_ADDR(master_shm, Dev_Tbl->d_ent[n]);
			if ((dev->type & DT_CLASS_MASK) == DT_ROBOT ||
			    dev->type == DT_HISTORIAN ||
			    dev->type == DT_PSEUDO_SC ||
			    0 == dev->fseq)
				break;
		}
		DisRb = (equ_t)n;
		ord = 0;
		}
		break;

	default:
		return (FALSE);
		/* NOTREACHED */
		break;
	}
	return (TRUE);
}

static int
samu_get_space_percent(struct CatalogEntry *cv)
{
	uint64_t c = cv->CeCapacity;
	uint64_t s = cv->CeSpace;
	double p;

#ifdef ajm
	if (is_optical(cv->media)) {
		c = 2 * cv->ptoc_fwa - cv->capacity;
	}
#endif

	if (c == 0 || c == s)
		return (0);
	p = (double)100*(((double)c-(double)s)/(double)c)+0.5;
	return ((int)p);
}
