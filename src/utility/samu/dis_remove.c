/*
 * dis_remove.c - Display removable media.
 *
 * Displays the status of all removable media drives.
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

#pragma ident "$Revision: 1.18 $"

#include <sys/sysmacros.h>

/* SAM-FS headers. */
#include "aml/device.h"
#include "sam/devnm.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "samu.h"

/* Private data. */
static int media = 0;
static int devrm_cur = 0;


void
DisRemove(void)
{
	dstate_t st;		/* Device state */
	dev_ent_t *dev;		/* Device entry */
	int rdy;		/* Device ready status */
	int lab;		/* Device label status */
	int pct;		/* Space used percentage */
	char *dtnm;		/* Media type mnemonic */
	int class;		/* Device class type */
	int i;
	int devrm_count;

	dtnm = (media != 0) ? sam_mediatoa(media) : "all";

	Mvprintw(0, 0,
	    catgets(catfd, SET, 2092, "Removable media status: %s"),
	    dtnm);
	Mvprintw(ln++, 0, catgets(catfd, SET, 7303,
	    "ty   eq  status      act  use  state \tvsn"));

	for (i = 0, devrm_count = 0; i <= Max_Devices; i++) {
		if (ln > LINES - 3) {
			Mvprintw(ln++, 0, catgets(catfd, SET, 2, "     more"));
			break;
		}
		if (Dev_Tbl->d_ent[i] == NULL)  continue;
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[i]);
		class = dev->type & DT_CLASS_MASK;

		if (!(class == DT_TAPE || class == DT_OPTICAL))  continue;

		if (media != 0) {
			if (class != (DT_CLASS_MASK & media))
				continue;
			if ((media & DT_MEDIA_MASK) != 0 && dev->type != media)
				continue;
		}
		if (++devrm_count < devrm_cur) continue;

		rdy = dev->status.b.ready;
		lab = (dev->status.bits & (DVST_LABELED | DVST_STRANGE));
		st = dev->state;
		pct = dev_usage(dev);

		Mvprintw(ln++, 0, "%2s%5d  %s %4d%4d%%  %-7s\t%-32s",
		    device_to_nm(dev->type), i,
		    sam_devstr(dev->status.bits), dev->active, pct,
		    (st == DEV_ON ? (rdy ? "ready" : "notrdy") : dev_state[st]),
		    (rdy ? (lab ? dev->vsn : "nolabel") : " "));
		Clrtoeol();
		if (dev->dis_mes[DIS_MES_CRIT][0] != '\0') {
			Attron(A_BOLD);
			Mvprintw(ln++, 0, "\t%s", dev->dis_mes[DIS_MES_CRIT]);
			Attroff(A_BOLD);
		} else {
			Mvprintw(ln++, 0, "\t%s", dev->dis_mes[DIS_MES_NORM]);
		}
		Clrtoeol();
	}
}


/*
 * Initialize display.
 */
boolean
InitRemove(void)
{
	int m = 0;

	if (Argc > 1) {
		if ((m = sam_atomedia(Argv[1])) == 0) {
			Error(catgets(catfd, SET, 2768,
			    "Unknown media type (%s)"),
			    Argv[1]);
		}
		media = m;
	}
	return (TRUE);
}

/*
 * Keyboard processing.
 */

#define	TLINES MAX(((LINES - 6)/2), 1)
#define	HLINES MAX(((LINES - 6)/4), 1)

boolean
KeyRemove(char key)
{
	switch (key) {

	case KEY_full_fwd:
		devrm_cur += TLINES;
		break;

	case KEY_full_bwd:
		if ((devrm_cur -= TLINES) < 0) devrm_cur = 0;
		break;

	case KEY_half_bwd:
		if ((devrm_cur -= HLINES) < 0) devrm_cur = 0;
		break;

	case KEY_half_fwd:	/* Half page up */
		devrm_cur += HLINES;
		break;

	default:
		return (FALSE);
		/*NOTREACHED*/
		break;
	}
	return (TRUE);
}
