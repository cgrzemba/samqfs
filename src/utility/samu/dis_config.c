/*
 * dis_config.c  - Display device configuration.
 *
 * Displays the device configuration.
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

#pragma ident "$Revision: 1.15 $"

#include <sys/sysmacros.h>

/* SAM-FS headers. */
#include "aml/device.h"
#include "sam/devnm.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "samu.h"

/* Private data. */
static int dev_cur;

static void DisConfigh(void);

/*
 * Display configuration - c display.
 */
void
DisConfig()
{
	int headln;

	Mvprintw(0, 0,
	    catgets(catfd, SET, 868, "Device configuration:"));
	Mvprintw(ln++, 0,
	    catgets(catfd, SET, 7301,
	    "ty   eq state   device_name\t\t\t   fs family_set"));
	headln = ln;
	DisConfigh();
	if (headln == ln) {
	dev_cur = 0;
	DisConfigh();
	}
}

/*
 * Display configuration helper.
 */
void
DisConfigh()
{
	dstate_t st;		/* Device state */
	dtype_t ty;			/* Device type */
	dev_ent_t *dev;		/* Device entry */
	int i;
	int dev_count;

	for (i = 0, dev_count = 0; i <= Max_Devices; i++) {

		if (Dev_Tbl->d_ent[i] == NULL)  continue;
		if (ln > LINES - 3) {
			Mvprintw(ln++, 0, catgets(catfd, SET, 2, "     more"));
			break;
		}
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[i]);

		st = dev->state;
		ty = dev->type;

		if (IS_DISK(dev) || IS_FS(dev))
			continue;  /* don't show fs equip. */
		if (++dev_count < dev_cur) continue;

		Mvprintw(ln++, 0, "%2s%5d %-7s %-32s",
		    device_to_nm(ty), i, dev_state[st], dev->name);
		if (dev->fseq != 0)  Printw("%5d", dev->fseq);
		else  Printw("     ");
		Printw(" %s", dev->set);
		Clrtoeol();
	}
}

/*
 * Keyboard processing.
 */

#define	TLINES MAX((LINES - 6), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)

boolean
KeyConfig(
	char key)
{
	switch (key) {

	case KEY_full_fwd:
		dev_cur += TLINES;
	break;

	case KEY_full_bwd:
		if ((dev_cur -= TLINES) < 0) dev_cur = 0;
	break;

	case KEY_half_bwd:
		if ((dev_cur -= HLINES) < 0) dev_cur = 0;
		break;

	case KEY_half_fwd:	/* Half page up */
		dev_cur += HLINES;
		break;

	default:
	return (FALSE);
		/*NOTREACHED*/
	break;
	}
	return (TRUE);
}
