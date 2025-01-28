/*
 * dis_sense.c - Display SCSI sense data.
 *
 * Displays the SCSI sense data for the selected device.
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

#pragma ident "$Revision: 1.16 $"


/* SAM-FS headers. */
#include "aml/device.h"
#include "sam/devnm.h"
#include "aml/scsi.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "samu.h"

/* sense_key_msg - SCSI sense key messages. */
static char *sense_key_msg[] = {
	"No Sense",		/* 0 */
	"Recovered Error",	/* 1 */
	"Not Ready",		/* 2 */
	"Medium Error",		/* 3 */
	"Hardware Error",	/* 4 */
	"Illegal Request",	/* 5 */
	"Unit Attention",	/* 6 */
	"Data Protect",		/* 7 */
	"Blank Check",		/* 8 */
	"Reserved (9)",		/* 9 */
	"Reserved (a)",		/* a */
	"Abort Command",	/* b */
	"Equal",		/* c */
	"Volume Overflow",	/* d */
	"Miscompare",		/* e */
	"Reserved (f)"		/* f */
};


void
DisSense()
{
	dev_ent_t *dev;			/* Device entry */
	sam_newext_sense_t *sense;	/* SCSI extended sense data */
	uchar_t *w;			/* Byte pointer */
	int i;

	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[DisEq]);
	Mvprintw(0, 16, "eq: %d", DisEq);
	if (dev->sense == NULL) {
		Mvprintw(2, 0, catgets(catfd, SET, 1790,
		    "No sense data available"));
		return;
	}
	Mvprintw(0, 24, "addr: %.8x", (long)dev->sense);
	Mvprintw(ln, 0, "Sense data:");
	Mvprintw(ln++, 40, catgets(catfd, SET, 370, "Additional sense bytes:"));
	sense = (sam_newext_sense_t *)SHM_ADDR(master_shm, dev->sense);

	w = (uchar_t *)sense;
	Mvprintw(ln++, 0, "%.2x       valid/code:", *w++);
	if (!sense->es_valid)  Printw("     not VALID");
	Mvprintw(ln++, 0, "  %.2x     segment", *w++);
	Mvprintw(ln++, 0, "    %.2x   fmk/eqm/ili/key:", *w++);
	Printw(sense_key_msg[sense->es_key]);
	Mvprintw(ln++, 0, "%.2x%.2x%.2x%.2x info[0-3]", sense->es_info[0],
	    sense->es_info[1], sense->es_info[2], sense->es_info[3]);
	Mvprintw(ln++, 0, "      %.2x add_len", sense->es_add_len);
	Mvprintw(ln++, 0, "%.2x%.2x%.2x%.2x cmd_info[0-3]",
	    sense->es_cmd_info[0],
	    sense->es_cmd_info[1], sense->es_cmd_info[2],
	    sense->es_cmd_info[3]);
	Mvprintw(ln++, 0, "%.2x       ASC", sense->es_asc);
	Mvprintw(ln++, 0, "  %.2x     ASCQ", sense->es_ascq);
	Mvprintw(ln++, 0, "    %.2x   fru", sense->es_fru);
	w = ((uchar_t *)sense) + 15;
	Mvprintw(ln++, 0, "      %.2x sksv/cd/vu/bpv/bitp", *w);
	Mvprintw(ln++, 0, "%.2x%.2x     field pointer[0-1]", sense->es_fp[0],
	    sense->es_fp[1]);
	ln = 4;
	for (i = 0; i < 32 - 18; i += 4) {
		Mvprintw(ln++, 40, "[%d] %.2x%.2x%.2x%.2x", i + 18,
		    sense->es_add_info[i], sense->es_add_info[i + 1],
		    sense->es_add_info[i + 2], sense->es_add_info[i + 3]);
	}
}


/*
 * Display initialization.
 */
boolean
InitSense(void)
{
	if (Argc > 1) {
		DisEq = finddev(Argv[1]);
	}
	return (TRUE);
}


/*
 * Keyboard processing.
 */
boolean
KeySense(char key)
{
	short n;

	switch (key) {

	case KEY_full_fwd:
		n = DisEq;
		while (++n != DisEq) {
			if (n > Max_Devices)  n = 0;
			if (Dev_Tbl->d_ent[n] != NULL)  break;
		}
		DisEq = n;
		break;

	case KEY_full_bwd:
		n = DisEq;
		while (--n != DisEq) {
			if (n < 0)  n = Max_Devices - 1;
			if (Dev_Tbl->d_ent[n] != NULL)  break;
		}
		DisEq = n;
		break;

	default:
		return (FALSE);
		/*NOTREACHED*/
		break;

	}
	return (TRUE);
}
