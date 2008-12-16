/*
 * dis_device.c - Display device status information.
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
static int dev_cur;

static void DisDevsth(void);
static void DisOdh(void);
static void DisTapeh(void);

/*
 * Display device status.
 */
void
DisDevst(
void)
{
	int headln;

	Mvprintw(ln++, 0, catgets(catfd, SET, 7302,
	    "ty     eq state   device_name\t\t\t     fs status"));
	headln = ln;
	DisDevsth();
	if (headln == ln) {
		dev_cur = 0;
		DisDevsth();
	}
}

/*
 * Display device status helper.
 */
void
DisDevsth(
	void)
{
	dstate_t st;	/* Device state */
	dtype_t ty;		/* Device type */
	dev_ent_t *dev;	/* Device entry */
	int i;
	int dev_count;

	for (i = 0, dev_count = 0; i <= Max_Devices; i++) {
		if (ln > LINES - 3) {
			Mvprintw(ln++, 0,
			    catgets(catfd, SET, 2, "     more"));
			break;
		}
		if (Dev_Tbl->d_ent[i] == NULL)  continue;
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[i]);

		st = dev->state;
		ty = dev->type;

		if (IS_DISK(dev) || IS_FS(dev))
			continue;  /* don't show fs equip. */
		if (++dev_count < dev_cur) continue;

		Mvprintw(ln++, 0, "%-4s%5d %-7s %-32s",
		    device_to_nm(ty), i, dev_state[st], dev->name);
		if (dev->fseq != 0)  Printw("%5d", dev->fseq);
		else  Printw("     ");
		Printw(" %s", sam_devstr(dev->status.bits));
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
 * Display optical status information.
 */
void
DisOd()
{
	int headln;

	Mvprintw(ln++, 0, catgets(catfd, SET, 7303,
	    "ty   eq  status      act  use  state \tvsn"));
	headln = ln;
	DisOdh();
	if (headln == ln) {
		dev_cur = 0;
		DisOdh();
	}
}

/*
 * Display optical status helper.
 */
void
DisOdh()
{
	dstate_t st;	/* Device state */
	dev_ent_t *dev;	/* Device entry */
	int rdy;		/* Device ready status */
	int lab;		/* Device label status */
	int pct;		/* Space used percentage */
	int i;
	int dev_count;

	for (i = 0, dev_count = 0; i <= Max_Devices; i++) {
		if (ln > LINES - 3) {
			Mvprintw(ln++, 0, catgets(catfd, SET, 2, "     more"));
			break;
		}
		if (Dev_Tbl->d_ent[i] == NULL)  continue;
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[i]);

		if ((dev->type & DT_CLASS_MASK) != DT_OPTICAL)  continue;
		if (++dev_count < dev_cur) continue;
		rdy = dev->status.b.ready;
		lab = (dev->status.bits & (DVST_LABELED | DVST_STRANGE));
		st = dev->state;
		pct = dev_usage(dev);

		Mvprintw(ln++, 0, "%2s%5d  %s %4d%4d%%  %-7s\t%-32s",
		    device_to_nm(dev->type), i,
		    sam_devstr(dev->status.bits), dev->active, pct,
		    (st == DEV_ON) ? ((rdy) ? "ready" : "notrdy") :
		    dev_state[st],
		    (rdy) ? ((lab) ? dev->vsn : "nolabel") : " ");
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
 * Display tape status information.
 */
void
DisTape(
	void)
{
	int headln;

	Mvprintw(ln++, 0, catgets(catfd, SET, 7303,
	    "ty   eq  status      act  use  state \tvsn"));
	headln = ln;
	DisTapeh();
	if (headln == ln) {
		dev_cur = 0;
		DisTapeh();
	}
}

/*
 * Display tape status helper.
 */
void
DisTapeh(
	void)
{
	dstate_t st;	/* Device state */
	dev_ent_t *dev;	/* Device entry */
	int rdy;		/* Device ready status */
	int lab;		/* Device label status */
	int pct;		/* Space used percentage */
	int i;
	int dev_count;

	for (i = 0, dev_count = 0; i <= Max_Devices; i++) {
		if (ln > LINES - 3) {
			Mvprintw(ln++, 0, catgets(catfd, SET, 2, "     more"));
			break;
		}
		if (Dev_Tbl->d_ent[i] == NULL)  continue;
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[i]);
		if ((dev->type & DT_CLASS_MASK) != DT_TAPE)  continue;
		if (++dev_count < dev_cur) continue;

		rdy = dev->status.b.ready;
		lab = (dev->status.bits & (DVST_LABELED | DVST_STRANGE));
		st = dev->state;
		pct = dev_usage(dev);

		Mvprintw(ln++, 0, "%2s%5d  %s %4d%4d%%  %-7s\t%-32s",
		    device_to_nm(dev->type), i,
		    sam_devstr(dev->status.bits), dev->active, pct,
		    (st == DEV_ON) ? ((rdy) ? "ready" : "notrdy") :
		    dev_state[st],
		    (rdy) ? ((lab) ? dev->vsn : "nolabel") : " ");
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
 * Keyboard processing.
 */

#define	TLINES MAX(((LINES - 6)/2), 1)
#define	HLINES MAX(((LINES - 6)/4), 1)

boolean
KeyDevst(
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
