/*
 * dis_shm.c - Display shared memory tables.
 *
 * Displays the contents of the shared memory pointer table.
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

#pragma ident "$Revision: 1.20 $"


/* ANSI headers. */
#include <ctype.h>

/* Solaris headers. */
#include <sys/ipc.h>

/* Local headers. */
#include "samu.h"


void
DisShm(void)
{
	shm_ptr_tbl_t *shm;		/* Address of shm pointer table */
	sam_defaults_t *def;		/* SAM defaults */
	prv_fs_ent_t *prv_fs_ent;
	char *fifo;			/* FIFO path */
	int fl;
	int ll;
	int i;

	fl = ln;
	shm = (shm_ptr_tbl_t *)SHM_ADDR(master_shm, 0);
	fifo = (char *)SHM_ADDR(master_shm, shm->fifo_path);
	Mvprintw(ln++, 0, "shm ptr tbl:");
	Mvprintw(ln++, 0, "size            %x (%d)", shm->shm_block.size,
	    shm->shm_block.size);
	Mvprintw(ln++, 0, "left            %x (%d)", shm->shm_block.left,
	    shm->shm_block.left);
	Mvprintw(ln++, 0, "scanner pid     %d", shm->scanner);
	Mvprintw(ln++, 0, "fifo path       %.4x %s", shm->fifo_path, fifo);
	Mvprintw(ln++, 0, "dev_table       %.4x", shm->dev_table);
	Mvprintw(ln++, 0, "first_dev       %.4x", shm->first_dev);
	Mvprintw(ln++, 0, "scan_mess       %.4x", shm->scan_mess);
	Mvprintw(ln++, 0, "preview_shmid   %d", shm->preview_shmid);
	Mvprintw(ln++, 0, "flags           0x%x", shm->flags.bits);
	ll = ln;

	def = GetDefaults();
	ln = fl;
	Mvprintw(ln++, 40, "defaults:");
	Mvprintw(ln++, 40, "optical         %s", device_to_nm(def->optical));
	Mvprintw(ln++, 40, "tape            %s", device_to_nm(def->tape));
	Mvprintw(ln++, 40, "timeout         %d", def->timeout);
	Mvprintw(ln++, 40, "previews        %d", def->previews);
	Mvprintw(ln++, 40, "stages          %d", def->stages);
	Mvprintw(ln++, 40, "log_facility    %d", def->log_facility);
	Mvprintw(ln++, 40, "label barcode   %s",
	    (def->flags & DF_LABEL_BARCODE) ? "TRUE":"FALSE");
	Mvprintw(ln++, 40, "barcodes low    %s",
	    (def->flags & DF_BARCODE_LOW) ? "TRUE":"FALSE");
	Mvprintw(ln++, 40, "export unavail  %s",
	    (def->flags & DF_EXPORT_UNAVAIL) ? "TRUE":"FALSE");
	Mvprintw(ln++, 40, "attended        %s",
	    (def->flags & DF_ATTENDED) ? "TRUE":"FALSE");
	Mvprintw(ln++, 40, "start rpc       %s",
	    (def->flags & DF_START_RPC) ? "TRUE":"FALSE");
	ln = ll;
	Mvprintw(ln++, 0, "preview stages  %d", Preview_Tbl->stages);
	Mvprintw(ln++, 0, "preview avail   %d", Preview_Tbl->avail);
	Mvprintw(ln++, 0, "preview count   %d", Preview_Tbl->ptbl_count);
	Mvprintw(ln++, 0, "preview sequence  %d", Preview_Tbl->sequence);
	Mvprintw(ln,   0, "age factor  %8.0f", Preview_Tbl->prv_age_factor);
	Mvprintw(ln++, 40, "vsn factor  %8.0f", Preview_Tbl->prv_vsn_factor);
	Mvprintw(ln,   0, "fs tbl ptr 0x%x", Preview_Tbl->prv_fs_table);
	Mvprintw(ln++, 40, "fs count  %d", Preview_Tbl->fs_count);
	prv_fs_ent = (prv_fs_ent_t *)PRE_REF_ADDR(Preview_Tbl->prv_fs_table);
	for (i = 0; i < Preview_Tbl->fs_count; i++, prv_fs_ent++) {
		Mvprintw(ln++, 0,
		    "fseq  %d %s state %d %6.0f %6.0f %6.0f %6.0f",
		    prv_fs_ent->prv_fseq, prv_fs_ent->prv_fsname,
		    prv_fs_ent->prv_fswm_state,
		    prv_fs_ent->prv_fswm_factor[0],
		    prv_fs_ent->prv_fswm_factor[1],
		    prv_fs_ent->prv_fswm_factor[2],
		    prv_fs_ent->prv_fswm_factor[3]);
	}
}
