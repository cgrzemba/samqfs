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
#pragma ident	"$Revision: 1.8 $"

#ifndef _SAMC_H
#define	_SAMC_H

#define	SAM_VERSION "4.1"

#ifndef numof
#define	numof(a) (sizeof (a)/sizeof (*(a))) /* Number of elements in array a */
#endif

/* Key code definitions. */
#ifndef CTRL
#define	CTRL(x)		((x) & 0x1f)
#endif

typedef enum {
	KEY_full_fwd = 0x06, 	/* ^F Full page forward */
	KEY_full_bwd = 0x02,	/* ^B Full page backward */
//	KEY_half_fwd = 0x04,	/* ^D Half page forward */
//	KEY_half_bwd = 0x15,	/* ^U Half page backward */
	KEY_details  = 0x09, 	/* ^I Enable details    */
	KEY_adv_fmt  = 0x0b,	/* ^K Advance format */
	KEY_backspace = 0x08,
	KEY_default = CTRL('d'), /* ^D Reset to default */
	KEY_enter = 0x0A,
	KEY_help  = '?',
	KEY_esc   = CTRL('[')

} Keys;

void ShowBanner(void);
boolean_t askw(int row, int col, char *question, char defaultansw);
void msgwin(char **text, int lines, char *title);
void help(void);

char *program_name;
#endif /* _SAMC_H */
