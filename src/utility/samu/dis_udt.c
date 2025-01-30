/*
 * dis_udt.c - Display SAM-FS device table entry.
 *
 * Displays the contents of the device table entry for the
 * selected device.
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

#pragma ident "$Revision: 1.25 $"


/* SAM-FS headers. */
#include "aml/device.h"
#include "sam/devnm.h"
#include "sam/nl_samfs.h"
#include "pub/lib.h"

/* Local headers. */
#include "samu.h"


void
DisUdt()
{
	equ_t eq;		/* Device equipment ordinal */
	int fl;
	dev_ent_t *dev;		/* Device entry */
	int *w;			/* Word ptr in IOB device table */
	long long *llw;
	int date_line;
	int low_line;

	eq = DisEq;
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);
	Mvprintw(0, 0, catgets(catfd, SET, 887, "Device table: eq: %d"), eq);
	Mvprintw(0, 24, "addr: %.8x", (long)Dev_Tbl->d_ent[eq]);
	if (dev->dis_mes[DIS_MES_CRIT][0] != '\0') {
		Attron(A_BOLD);
		Mvprintw(ln++, 0, "message: %s", dev->dis_mes[DIS_MES_CRIT]);
		Attroff(A_BOLD);
	} else {
		Mvprintw(ln++, 0, "message: %s", dev->dis_mes[DIS_MES_NORM]);
	}

	if (is_scsi(dev)) {
	if (dev->serial[0])
		Mvprintw(ln++, 0, "Inquiry: %s %s rev: %s serial: %s",
		    dev->vendor_id, dev->product_id, dev->revision,
		    dev->serial);
	else
		Mvprintw(ln++, 0, "Inquiry: %s %s rev: %s",
		    dev->vendor_id, dev->product_id, dev->revision);
	}

	ln++;
	fl = ln;
	Clrtoeol();

	w = (int *)&dev;
	llw = (long long *)&dev->mutex;
	Mvprintw(ln++, 0, "%.16llx %.16llx ", *llw, *(llw+1));
	Mvprintw(ln++, 0, "%.16llx mutex", *(llw+2));
	Mvprintw(ln++, 0, "%.8x next", dev->next);
	/* LINTED pointer cast may result in improper alignment */
	w = (int *)dev->set;
	Mvprintw(ln++, 0, "%.8x set:  %s", *w++, string(dev->set));
	Mvprintw(ln++, 0, "%.8x ", *w++);
	Mvprintw(ln++, 0, "%.8x ", *w++);
	Mvprintw(ln++, 0, "%.8x ", *w++);
	Mvprintw(ln++, 0, "%.4x%.4x eq/fseq", dev->eq, dev->fseq);
	Mvprintw(ln++, 0, "%.4x%.4x type/equ_type", dev->type, dev->equ_type);
	Mvprintw(ln++, 0, "%.4x     state", dev->state);
	Mvprintw(ln++, 0, "%.8x st_rdev", dev->st_rdev);
	Mvprintw(ln++, 0, "%.4x%.4x ord/model", dev->ord, dev->model);
	Mvprintw(ln++, 0, "%.8x mode_sense", dev->mode_sense);
	Mvprintw(ln++, 0, "%.8x sense", dev->sense);
	Mvprintw(ln++, 0, "%.8x space", dev->space);
	Mvprintw(ln++, 0, "%.8x capacity", dev->capacity);
	Mvprintw(ln++, 0, "%.8x active", dev->active);
	Mvprintw(ln++, 0, "%.8x open", dev->open_count);
	Mvprintw(ln++, 0, "%.8x sector_size", dev->sector_size);
	Mvprintw(ln++, 0, "%.8x label_address", dev->label_address);
	/* LINTED pointer cast may result in improper alignment */
	w = (int *)dev->vsn;
	Mvprintw(ln++, 0, "%.8x vsn:  %s", *w++, string(dev->vsn));
	date_line = ln;
	Mvprintw(ln++, 0, "%.8x ", *w++);
	Mvprintw(ln++, 0, "%.8x ", *w++);
	Mvprintw(ln++, 0, "%.8x ", *w++);
	Mvprintw(ln++, 0, "%.8x status: %s", dev->status.bits,
	    sam_devstr(dev->status.bits));
	low_line = ln;

	ln = fl;

	Mvprintw(ln++, 40, "%.8x delay", dev->delay);
	Mvprintw(ln++, 40, "%.8x unload_delay", dev->unload_delay);

	switch (dev->type & DT_CLASS_MASK) {
	case DT_OPTICAL:
		Mvprintw(ln++, 40, "%.8x mtime", dev->mtime);
		Mvprintw(ln++, 40, "%.8x scan_tid", dev->scan_tid);
		Mvprintw(ln++, 40, "%.8x slot", dev->slot);
		Mvprintw(ln++, 40, "%.8x next_file_fwa",
		    dev->dt.od.next_file_fwa);
		Mvprintw(ln++, 40, "%.8x ptoc_fwa", dev->dt.od.ptoc_fwa);
		Mvprintw(ln++, 40, "%.8x ptoc_lwa", dev->dt.od.ptoc_lwa);
		Mvprintw(ln++, 40, "%.2x medium", dev->dt.od.medium_type);
		Mvprintw(date_line, 10, "%s", ctime(&dev->label_time));
		break;
	case DT_TAPE:
		Mvprintw(ln++, 40, "%.8x mtime", dev->mtime);
		Mvprintw(ln++, 40, "%.8x scan_tid", dev->scan_tid);
		Mvprintw(ln++, 40, "%.8x slot", dev->slot);
		Mvprintw(ln++, 40, "%.8x eod_position", dev->dt.tp.position);
		Mvprintw(ln++, 40, "%.8x stage position",
		    dev->dt.tp.stage_pos);
		Mvprintw(ln++, 40, "%.8x next_read", dev->dt.tp.next_read);
		Mvprintw(ln++, 40, "%.8x def blk-size",
		    dev->dt.tp.default_blocksize);
		Mvprintw(ln++, 40, "%.8x pos timeout",
		    dev->dt.tp.position_timeout);
		Mvprintw(ln++, 40, "%.8x max blksize",
		    dev->dt.tp.max_blocksize);
		Mvprintw(ln++, 40, "%.8x dflt cap",
		    dev->dt.tp.default_capacity);
		Mvprintw(ln++, 40, "%.8x drvblksz", dev->dt.tp.driver_blksize);
		Mvprintw(ln++, 40, "%.8x fsn", dev->dt.tp.fsn);
		Mvprintw(ln++, 40, "%.8x mask", dev->dt.tp.mask);
		Mvprintw(ln++, 40, "%.4x drv indx", dev->dt.tp.drive_index);
		Mvprintw(ln++, 40, "%.2x medium", dev->dt.tp.medium_type);
		/* LINTED pointer cast may result in improper alignment */
		w = (int *)dev->dt.tp.samst_name;
		Mvprintw(low_line++, 0,
		    "%.8x samnm: %s", *w++, string(dev->dt.tp.samst_name));
		Mvprintw(date_line, 10, "%s", ctime(&dev->label_time));
		break;
	default:
		switch (dev->type) {
		case DT_PSEUDO_SS:
			Mvprintw(ln++, 40, "%.8x message", dev->dt.ss.message);
			Mvprintw(ln++, 40, "%.8x clients", dev->dt.ss.clients);
			Mvprintw(ln++, 40, "%.8x private", dev->dt.ss.private);
			Mvprintw(ln++, 40, "%.8x ordinal", dev->dt.ss.ordinal);
			Mvprintw(ln++, 40, "%.8x serv_port %d",
			    dev->dt.ss.serv_port, dev->dt.ss.serv_port);
			break;
		case DT_PSEUDO_RD:
			Mvprintw(ln++, 40, "%.8x message", dev->dt.sp.message);
			Mvprintw(ln++, 40, "%.8x private", dev->dt.sp.private);
			break;
		case DT_PSEUDO_SC:
			Mvprintw(ln++, 40, "%.8x message", dev->dt.sc.message);
			Mvprintw(ln++, 40, "%.8x private", dev->dt.sc.private);
			Mvprintw(ln++, 40, "%.8x process", dev->dt.sc.process);
			Mvprintw(ln++, 40, "%.8x server", dev->dt.sc.server);
			Mvprintw(ln++, 40, "%.8x data_port %d",
			    dev->dt.sc.data_port, dev->dt.sc.data_port);
			break;
		default:
			if (IS_ROBOT(dev)) {
				Mvprintw(ln++, 40, "%.8x message",
				    dev->dt.rb.message);
				Mvprintw(ln++, 40, "%.8x private",
				    dev->dt.rb.private);
				Mvprintw(ln++, 40, "%.8x process",
				    dev->dt.rb.process);
		/* LINTED pointer cast may result in improper alignment */
				w = (int *)dev->dt.rb.name;
				Mvprintw(low_line++, 0, "%.8x cat. name: %s",
				    *w++, string(dev->dt.rb.name));
			} else {
				w = (int *)&dev->dt;
				Mvprintw(low_line++, 0, "%.8x dt", *w++);
			}
			break;
		}
		break;
	}

	ln = low_line;

	/* LINTED pointer cast may result in improper alignment */
	w = (int *)dev->name;
	Mvprintw(ln++, 0, "%.8x name: %s", *w++, string(dev->name));
}


/*
 * Display initialization.
 */
boolean
InitUdt(void)
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
KeyUdt(char key)
{
	int n;

	switch (key) {

	case KEY_full_fwd:
		n = DisEq;
		while (++n != DisEq) {
			if (n > Max_Devices)  n = 0;
			if (Dev_Tbl->d_ent[n] != NULL)  break;
		}
		DisEq = (equ_t)n;
		break;

	case KEY_full_bwd:
		n = DisEq;
		while (--n != DisEq) {
			if (n < 0)  n = Max_Devices - 1;
			if (Dev_Tbl->d_ent[n] != NULL)  break;
		}
		DisEq = (equ_t)n;
		break;

	default:
		return (FALSE);

	}
	return (TRUE);
}
