/*
 * dis_trace.c - display trace information
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

#pragma ident "$Revision: 1.14 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/names.h"
#define	TRACE_CONTROL
#include "sam/sam_trace.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "samu.h"

/* Private data. */
static int tidFirst = 1;

void
DisTrace(void)
{
	static struct TraceCtlBin *tb = NULL;
	int		tid;

	if (tb == NULL) {
		tb = MapFileAttach(TRACECTL_BIN, TRACECTL_MAGIC, O_RDONLY);
		if (tb == NULL) {
			return;
		}
	}

	for (tid = tidFirst; tid < TI_MAX; tid++) {
		struct TraceCtlEntry *tc;

		tc = &tb->entry[tid];
		Mvprintw(ln++, 0, "%-14s ", traceIdNames[tid]);
		if (*tc->TrFname != '\0') {
			Printw("%s", tc->TrFname);
			Mvprintw(ln++, 0, "%14s %s", "",
			    TraceGetOptions(tc, NULL, 0));
			Mvprintw(ln++, 0, "%14s size %s  age %d", "",
			    StrFromFsize(tc->TrSize, 0, NULL, 0), tc->TrAge);
		} else {
			Printw("off");
			ln += 2;
		}
		ln++;
		if (ln > LINES - 3) {
			Mvprintw(LINES-2, 0,
			    catgets(catfd, SET, 2, "     more"));
			return;
		}
	}
}

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)/4), 1)

/*
 * Initialize display.
 */
boolean
InitTrace(void)
{
	return (TRUE);
}

boolean
KeyTrace(char key)
{
	switch (key) {
	case KEY_full_fwd:
		tidFirst += TLINES;
		break;

	case KEY_full_bwd:
		if ((tidFirst -= TLINES) < 0) tidFirst = 1;
		break;

	default:
		return (FALSE);
	}
	return (TRUE);
}
