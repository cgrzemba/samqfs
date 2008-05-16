/*
 * dis_prequeue.c - Display preview staging queues.
 *
 * Displays preview staging queues.
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

#pragma ident "$Revision: 1.17 $"

/* ANSI headers. */
#include <memory.h>
#include <string.h>

/* SAM-FS headers. */
#include "aml/fifo.h"
#include "aml/id_to_path.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "samu.h"

/* Private data. */
static media_t media;
static int FileFirst = 0;
static int pshow_path;

/* Private functions. */

/* External functions. */
extern void DisplayStageStreams(boolean_t display_active,
	int file_first, int display_filename, media_t media);


void
DisPrequeue(void)
{
	char *display_media;

	display_media = (media == 0) ? "all" : sam_mediatoa(media);
	Mvprintw(0, 0, catgets(catfd, SET, 7378,
	    "Pending stage queue by media type: %s"), display_media);

	DisplayStageStreams(B_FALSE, FileFirst, pshow_path, media);
}

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)
#define	QLINES MAX(((LINES - 6)/4), 1)

/*
 * Keyboard processing.
 */
boolean
KeyPrequeue(char key)
{
	int tlines, hlines;

	if (pshow_path) {
		tlines = HLINES;
		hlines = QLINES;
	} else {
		tlines = TLINES;
		hlines = HLINES;
	}

	switch (key) {

	case KEY_adv_fmt:
	case KEY_details:
		pshow_path ^= 1;
		break;

	case KEY_full_fwd:
		FileFirst += tlines;
		break;

	case KEY_full_bwd:
		if ((FileFirst -= tlines) < 0) FileFirst = 0;
		break;

	case KEY_half_bwd:
		if ((FileFirst -= hlines) < 0) FileFirst = 0;
		break;

	case KEY_half_fwd:
		FileFirst += hlines;
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
InitPrequeue(void)
{
	media_t m = (media_t)0;
	int i = 1;

	pshow_path = FALSE;

	while (Argc > i) {
		if ((m = sam_atomedia(Argv[i])) != 0) {
			media = m;
		} else if (strcmp(Argv[i], "all") == 0) {
			media = 0;
		} else if (strcmp(Argv[i], "path") == 0) {
			pshow_path = TRUE;
		} else {
			Error(catgets(catfd, SET, 2768,
			"Unknown media type (%s)"), Argv[i]);
		}
		i++;
	}
	return (TRUE);
}
