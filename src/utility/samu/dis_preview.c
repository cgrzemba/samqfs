/*
 * dis_preview.c - Removable media mount requests.
 *
 * Displays pending mount requests.
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

#pragma ident "$Revision: 1.20 $"


/* ANSI C headers. */
#include <time.h>
#include <stdlib.h>

/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
#include <curses.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "aml/preview.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "samu.h"

enum sort_key_e {
    SK_SLOT,
    SK_PRIORITY,
    SK_LAST
};

/* Private data. */

static enum sort_key_e sort_key = SK_SLOT; /* key for sorting display */

static preview_t **sort_index = NULL; /* sorted pointers to preview entries */

static struct {
	char sel;
	int nname;
	char *nname_msg;
} format[] = {
	{ '0', 7336, "both" },
	{ '1', 7337, "manual" },
	{ '2', 7338, "robot" },
	{ '3', 7339, "priority" }
};
static int fmt = 0;
static int start_req = 0;
static media_t media = 0;

/* Private functions. */

static int
Cmp_Priority(const void *p1, const void *p2);

/*
 * Removable media mount requests.
 */
void
DisPreview(void)
{
	preview_t *p;		/* Preview entry */
	int avail;		/* Available entry count */
	int count;		/* Active entry count */
	char *dtnm;		/* Media type mnemonic */
	char bits[9];		/* Entry status */
	time_t current_time;	/* Current time */
	time_t elapse_time;	/* Elapse (wait) time */
	int i;

	dtnm = (media == 0) ? catgets(catfd, SET, 7331, "all") :
	    sam_mediatoa(media);

	Mvprintw(0, 30, "%s %s", dtnm,
	    catgets(catfd, SET, format[fmt].nname, format[fmt].nname_msg));
	if (Max_Preview < 0) {
		Mvprintw(2, 0,
		    catgets(catfd, SET, 1954,
		    "Preview data not available"));
		return;
	}

	avail = Preview_Tbl->avail;
	count = Preview_Tbl->ptbl_count;
	if (sort_index) free(sort_index);
	sort_index =
	    (struct preview **)malloc(avail * sizeof (struct preview *));

	if (sort_index == NULL) {
		Error("cannot malloc sort index (%d)", avail);
	}
	Mvprintw(1, 50, "%s: %d",
	    catgets(catfd, SET, 7330, "count"), count);

	/* generate indices in order */

	for (i = 0; i < avail; i++) sort_index[i] = &Preview_Tbl->p[i];

	switch (sort_key) {

	default:
	case SK_SLOT:
		break;
	case SK_PRIORITY:
		qsort(sort_index, avail, sizeof (struct preview_t *),
		    Cmp_Priority);
		break;
	}

	if (3 == fmt) {
		Mvprintw(ln++, 0,
		    catgets(catfd, SET, 7329,
		    "index type pid     priority     rb   flags     "
		    "wait count  vsn"));
	} else {
		Mvprintw(ln++, 0,
		    catgets(catfd, SET, 7332,
		    "index type pid     user         rb   flags     "
		    "wait count  vsn"));
	}

	current_time = time(0l);


	for (i = start_req; i < avail && count != 0; i++) {
	if (ln > LINES - 3) {
		Mvprintw(ln++, 0, catgets(catfd, SET, 2, "     more"));
		break;
	}
		p = sort_index[i];
		if (!p->in_use)  continue;
		count--;
		if (fmt == 1 && (p->robot_equ != 0))  continue;
		if (fmt == 2 && (p->robot_equ == 0))  continue;
		if (media != 0 && !DT_EQUIV(p->resource.archive.rm_info.media,
		    media)) {
			continue;
		}
		dtnm = device_to_nm(p->resource.archive.rm_info.media);
		bits[0] = p->write ? 'W' : '-';
		bits[1] = p->busy ? 'b' : '-';
		bits[2] = p->p_error ? 'C' : '-';
		bits[3] = p->fs_req ? 'f' : '-';
		bits[4] = p->block_io ? 'B' :
		    (p->callback == CB_NOTIFY_TP_LOAD) ? 'N' :'-';
		bits[5] = p->flip_side ? 'S' : '-';
		bits[6] = p->stage ? 's' : '-';
		bits[7] = '\0';

		if (3 == fmt) {
			Mvprintw(ln++, 0, "%4d  %2s   %-6d  %8.0f    ",
			    i, dtnm, p->handle.pid, p->priority);
		} else {
			Mvprintw(ln++, 0, "%4d  %2s   %-6d  %-12s ",
			    i, dtnm, p->handle.pid, getuser(p->handle.uid));
		}

		if (p->robot_equ != 0)  Printw("%3d", p->robot_equ);
		else  Printw("   ");

		elapse_time = (current_time - p->ptime) / 60;	/* minutes */
		if (elapse_time > 1440) {
			Printw("  %s  %3dd", bits, elapse_time / 1440);
								/* days */
		} else {
			Printw("  %s  %2d:%2.2d", bits, elapse_time / 60,
			    elapse_time % 60);
		}

		if (p->stage)  Printw(" %5d", p->count);
		else  Printw("      ");

		Printw("  %-12.22s", p->resource.archive.vsn);
		Clrtoeol();
	}
}

/*
 * Compare priorities.
 */
static int
Cmp_Priority(const void *p1, const void *p2)
{
	struct preview *a1 = *(struct preview **)p1;
	struct preview *a2 = *(struct preview **)p2;

	/*
	 * The following three lines of code are just an implementation of:
	 * return(a2->priority - a1->priority);
	 * I had problems with qsort() referencing data outside the array.
	 */
	if (a2->priority < a1->priority)
		return (-1);
	if (a2->priority > a1->priority)
		return (1);
	return (0);

}

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)

/*
 * Keyboard processing.
 */
boolean
KeyPreview(char key)
{

	switch (key) {

	case KEY_full_fwd:
		start_req += TLINES;
		break;
	case KEY_full_bwd:
		if ((start_req -= TLINES) < 0) start_req = 0;
		break;
	case KEY_half_fwd:
		start_req += HLINES;
		break;
	case KEY_half_bwd:
		if ((start_req -= HLINES) < 0) start_req = 0;
		break;
	case KEY_adv_fmt:
		if (++fmt >= numof(format))  fmt = 0;
		if (fmt == 3) {
			sort_key = SK_PRIORITY;
		} else {
			sort_key = SK_SLOT;
		}
		break;
	case '0':
		fmt = 0;
		sort_key = SK_SLOT;
		break;
	case '1':
		fmt = 1;
		sort_key = SK_SLOT;
		break;
	case '2':
		fmt = 2;
		sort_key = SK_SLOT;
		break;
	case '3':
		fmt = 3;
		sort_key = SK_PRIORITY;
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}


/*
 * Initialize display.
 */
boolean
InitPreview(void)
{
	media_t m = 0;

	if (Argc > 1) {
		if ((m = sam_atomedia(Argv[1])) == 0) {
			Error(catgets(catfd, SET, 2768,
			"Unknown media type (%s)"), Argv[1]);
		}
		media = m;
	}
	return (TRUE);
}
