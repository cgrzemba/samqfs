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

#ifndef _SAMC_FORMDEFS_H
#define	_SAMC_FORMDEFS_H

#pragma ident	"$Revision: 1.11 $"

/* API header files */
#include "devstat.h"
#include "mgmt/types.h"
#include "mgmt/error.h"
#include "mgmt/device.h"
#include "mgmt/filesystem.h"
#include "mgmt/archive.h"
#include "mgmt/diskvols.h"
#include "mgmt/release.h"
#include "mgmt/license.h"
/* other header files */
#include "forms.h"

#define	ANYFS		0
#define	REC_TERM	{ ANYFS, (fieldbuilder) 0, 0, 0, 0, 0, (char *)0 }
#define	MAX_EMAIL_LEN	50
#define	MAXDSKVOLLEN	32
#define	dual_dau	"dual DAU"
#define	large_dau	"large DAU only"


/*
 * The order of elements in each array DOES matter.
 */

char *on_off_enum[] = {
	"on",
	"off",
	NULL
};
char *yes_no_enum[] = {
	"yes",
	"no",
	NULL
};

char *metadata_enum[] = {
	"split",
	"combined",
	NULL
};

char *dau_enum[] = {
	dual_dau,
	large_dau,
	NULL
};

// -------------------- Filesystem-related forms ------------------------------

convinfo fs_cinfo[]  = {
	{ T_STR, offsetof(fs_t, fi_name) },
	{ T_UINT16, offsetof(fs_t, fi_eq) },
	{ T_UINT16, offsetof(fs_t, dau) }
};
FIELD_RECORD addfs_FORM[] = {
	{ ANYFS, mklabel, 0, 0, 3, 0, "Name (required)" },
	{ ANYFS, mkstring, 1, 30, 3, 30, NULL, NULL, &fs_cinfo[0] },

	{ ANYFS, mklabel, 0, 0, 5, 0, "Eq number" },
	{ ANYFS, mkint, 1, 12, 5, 30, NULL, "auto", &fs_cinfo[1] },

	{ ANYFS, mklabel, 0, 0, 12, 0, "DAU Size (kb)" },
	{ ANYFS, mkint, 1, 12, 12, 30, NULL, "16", &fs_cinfo[2] },

	{ QFS, mklabel, 0, 0, 7, 0, "Metadata and data" },
	{ QFS, mkenumtrig, 1, 15, 7, 30,
	    "split", NULL, NULL,  0, 0, 0,
	    metadata_enum },

	{ QFS, mklabel,		0, 0, 8, 0, "Disk Allocation Method" },
	{ QFS, mkenumtrig, 1, 20, 8, 30,
	    large_dau, NULL, NULL,  0, 0, 0,
	    dau_enum },

	{ QFS, mklabel, 0, 0, 9,  0, "Striped groups" },
	{ QFS, mkint, 1, 10, 9, 30, "0", NULL, NULL,
	    1, 1, 128 },

	REC_TERM
};

#define	offsetof_sam_opts offsetof(mount_options_t, sam_opts)
#define	offsetof_io_opts offsetof(mount_options_t, io_opts)
#define	offsetof_qfs_opts offsetof(mount_options_t, qfs_opts)
#define	offsetof_sam_mnt_mask offsetof_sam_opts\
	+ offsetof(sam_mount_options_t, change_flag)
#define	offsetof_io_mnt_mask offsetof_io_opts\
	+ offsetof(io_mount_options_t, change_flag)
#define	offsetof_qfs_mnt_mask offsetof_qfs_opts\
	+ offsetof(qfs_mount_options_t, change_flag)
convinfo mnt_cinfo[]  = {
	/* basic mount options 0-3 */
	{ T_INT16, offsetof_sam_opts + offsetof(sam_mount_options_t, high),
	    offsetof_sam_mnt_mask, MNT_HIGH },
	{ T_INT16, offsetof_sam_opts + offsetof(sam_mount_options_t, low),
	    offsetof_sam_mnt_mask, MNT_LOW },
	{ T_INT16, offsetof(mount_options_t, stripe),
	    offsetof(mount_options_t, change_flag), MNT_STRIPE },
	{ T_BLN, offsetof(mount_options_t, trace),
	    offsetof(mount_options_t, change_flag), MNT_TRACE },
	/* advanced options 4-21 */
	/* perf. tuning options */
	{ T_LLNG, offsetof_io_opts + offsetof(io_mount_options_t, readahead),
	    offsetof_io_mnt_mask, MNT_READAHEAD },
	{ T_LLNG, offsetof_io_opts + offsetof(io_mount_options_t, writebehind),
	    offsetof_io_mnt_mask, MNT_WRITEBEHIND },
	{ T_LLNG, offsetof_io_opts + offsetof(io_mount_options_t, wr_throttle),
	    offsetof_io_mnt_mask, MNT_WR_THROTTLE },
	{ T_INT16, offsetof_sam_opts + offsetof(mount_options_t, sync_meta),
	    offsetof(mount_options_t, change_flag), MNT_SYNC_META },
	{ T_UINT16,
	    offsetof_qfs_opts + offsetof(qfs_mount_options_t, mm_stripe),
	    offsetof_qfs_mnt_mask, MNT_MM_STRIPE },
	{ T_BLN,
	    offsetof_io_opts + offsetof(io_mount_options_t, forcedirectio),
	    offsetof_io_mnt_mask, MNT_FORCEDIRECTIO },
	/* i/o options 10-15 */
	{ T_INT,
	    offsetof_io_opts + offsetof(io_mount_options_t, dio_rd_consec),
	    offsetof_io_mnt_mask, MNT_DIO_RD_CONSEC },
	{ T_INT,
	    offsetof_io_opts + offsetof(io_mount_options_t, dio_rd_form_min),
	    offsetof_io_mnt_mask, MNT_DIO_RD_FORM_MIN },
	{ T_INT,
	    offsetof_io_opts + offsetof(io_mount_options_t, dio_rd_ill_min),
	    offsetof_io_mnt_mask, MNT_DIO_RD_ILL_MIN },
	{ T_INT,
	    offsetof_io_opts + offsetof(io_mount_options_t, dio_wr_consec),
	    offsetof_io_mnt_mask, MNT_DIO_WR_CONSEC  },
	{ T_INT,
	    offsetof_io_opts + offsetof(io_mount_options_t, dio_wr_form_min),
	    offsetof_io_mnt_mask, MNT_DIO_WR_FORM_MIN },
	{  T_INT,
	    offsetof_io_opts + offsetof(io_mount_options_t, dio_wr_ill_min),
	    offsetof_io_mnt_mask, MNT_DIO_WR_ILL_MIN },
	/* SAM options 16-21 */
	{ T_INT, offsetof_sam_opts + offsetof(sam_mount_options_t, partial),
	    offsetof_sam_mnt_mask, MNT_PARTIAL },
	{ T_INT, offsetof_sam_opts + offsetof(sam_mount_options_t, maxpartial),
	    offsetof_sam_mnt_mask, MNT_MAXPARTIAL },
	{ T_INT,
	    offsetof_sam_opts + offsetof(sam_mount_options_t, partial_stage),
	    offsetof_sam_mnt_mask, MNT_PARTIAL_STAGE },
	{ T_INT,
	    offsetof_sam_opts + offsetof(sam_mount_options_t, stage_retries),
	    offsetof_sam_mnt_mask, MNT_STAGE_RETRIES },
	{ T_INT,
	    offsetof_sam_opts + offsetof(sam_mount_options_t, stage_n_window),
	    offsetof_sam_mnt_mask, MNT_STAGE_N_WINDOW },
	{ T_BLN,
	    offsetof_sam_opts + offsetof(sam_mount_options_t, hwm_archive),
	    offsetof_sam_mnt_mask, MNT_HWM_ARCHIVE }
};
FIELD_RECORD mnt_FORM[] = {

	/* basic mount options 0-3 */
	{ SAMFS, mklabel, 0, 0, 2, 0, "HighWM (%)" },
	{ SAMFS, mklabel, 0, 0, 3, 0, "LowWM (%)" },
	{ ANYFS, mklabel, 0, 0, 4, 0, "Stripe" },
	{ ANYFS, mklabel, 0, 0, 5, 0, "Trace" },
	{ SAMFS, mkint, 1, 12, 2, 12, NULL, NULL, &mnt_cinfo[0], 0, 0, 100 },
	{ SAMFS, mkint, 1, 12, 3, 12, NULL, NULL, &mnt_cinfo[1], 0, 0, 100 },
	{ ANYFS, mkint, 1, 12, 4, 12, NULL, NULL, &mnt_cinfo[2] },
	{ ANYFS, mkenum, 1, 10, 5, 12,
	    "no", "no", &mnt_cinfo[3],  0, 0, 0,
	    yes_no_enum },

	/* second page - advanced options 4-21 */
	{ ANYFS, mknewpage, 1, 1, 1, 1 },

	/* performance tuning 4-9 */
	{ ANYFS, mklabel, 0, 0, 2, 0, "Read ahead (kb)" },
	{ ANYFS, mklabel, 0, 0, 3, 0, "Write behind (kb)" },
	{ ANYFS, mklabel, 0, 0, 4, 0, "Write throttle (kb)" },
	{ ANYFS, mklabel, 0, 0, 7, 0, "Sync metadata" },
	{ QFS, mklabel,   0, 0, 8, 0, "Metadata stripe width" },
	{ ANYFS, mklabel, 0, 0, 10, 0, "Force direct I/O" },
	{ ANYFS, mkint, 1, 13, 2, 20, NULL, NULL, &mnt_cinfo[4] },
	{ ANYFS, mkint, 1, 13, 3, 20, NULL, NULL, &mnt_cinfo[5] },
	{ ANYFS, mkint, 1, 13, 4, 20, NULL, NULL, &mnt_cinfo[6] },
	{ ANYFS, mkint, 1,  8, 7, 22, NULL, NULL, &mnt_cinfo[7] },
	{ QFS,   mkint, 1,  8, 8, 22, NULL, NULL, &mnt_cinfo[8]  },
	{ ANYFS, mkenum, 1, 10, 10, 20, NULL, NULL, &mnt_cinfo[9], 0, 0, 0,
	    on_off_enum  },

	/* direct I/O discovery 10-15 */
	{ ANYFS, mklabel, 0, 0, 12, 0, "Consecutive reads" },
	{ ANYFS, mklabel, 0, 0, 13, 0, "Well-aligned read min (kb)" },
	{ ANYFS, mklabel, 0, 0, 14, 0, "Mis-aligned read min (kb)" },
	{ ANYFS, mklabel, 0, 0, 15, 0, "Consecutive writes" },
	{ ANYFS, mklabel, 0, 0, 16, 0, "Well-aligned write min (kb)" },
	{ ANYFS, mklabel, 0, 0, 17, 0, "Mis-aligned write min (kb)" },
	{ ANYFS, mkint, 1, 15, 12, 30, NULL, NULL, &mnt_cinfo[10] },
	{ ANYFS, mkint, 1, 15, 13, 30, NULL, NULL, &mnt_cinfo[11] },
	{ ANYFS, mkint, 1, 15, 14, 30, NULL, NULL, &mnt_cinfo[12] },
	{ ANYFS, mkint, 1, 15, 15, 30, NULL, NULL, &mnt_cinfo[13] },
	{ ANYFS, mkint, 1, 15, 16, 30, NULL, NULL, &mnt_cinfo[14] },
	{ ANYFS, mkint, 1, 15, 17, 30, NULL, NULL, &mnt_cinfo[15] },

	/* SAM-only options 16-21 */
	{ SAMFS, mklabel, 0, 0, 2, 35, "Default partial release size" },
	{ SAMFS, mklabel, 0, 0, 3, 35, "Default max partial size" },
	{ SAMFS, mklabel, 0, 0, 4, 35, "Partial stage size" },
	{ SAMFS, mklabel, 0, 0, 6, 35, "Stage retries" },
	{ SAMFS, mklabel, 0, 0, 7, 35, "Stage window size" },
	{ SAMFS, mklabel, 0, 0, 8, 35, "Run archiver when HWM reached" },

	{ SAMFS, mkint, 1, 10, 2, 65, NULL, NULL, &mnt_cinfo[16] },
	{ SAMFS, mkint, 1, 10, 3, 65, NULL, NULL, &mnt_cinfo[17] },
	{ SAMFS, mkint, 1, 10, 4, 65, NULL, NULL, &mnt_cinfo[18] },
	{ SAMFS, mkint, 1, 10, 6, 65, NULL, NULL, &mnt_cinfo[19] },
	{ SAMFS, mkint, 1, 10, 7, 65, NULL, NULL, &mnt_cinfo[20] },
	{ SAMFS, mkenum, 1, 10, 8, 65,
	    NULL, NULL, &mnt_cinfo[21],  0, 0, 0,
	    on_off_enum },

	REC_TERM
};

// -------------------- Archiving-related forms -------------------------------
#define	offsetof_arfs_mask offsetof(ar_fs_directive_t, change_flag)
convinfo arfs_cinfo[] = {
	{ T_STR, offsetof(ar_fs_directive_t, log_path),
	    offsetof_arfs_mask, AR_FS_log_path },
	{ T_TIME, offsetof(ar_fs_directive_t, fs_interval),
	    offsetof_arfs_mask, AR_FS_fs_interval },
	{ T_BLN, offsetof(ar_fs_directive_t, wait),
	    offsetof_arfs_mask, AR_FS_wait }
};
FIELD_RECORD arfsdir_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 3, 0, "Log path" },
	{ SAMFS, mklabel, 0, 0, 4, 0, "Interval" },
	{ SAMFS, mklabel, 0, 0, 5, 0, "Wait" },
	{ SAMFS, mkstring, MAX_PATH_LENGTH, 40, 3, 10, NULL, NULL,
	    &arfs_cinfo[0] },
	{ SAMFS, mktimein, 1, 15, 4, 10, NULL, NULL, &arfs_cinfo[1] },
	{ SAMFS, mkenum, 1, 10, 5, 10, NULL, NULL, &arfs_cinfo[2], 0, 0, 0,
	    on_off_enum }
};

convinfo crit_cinfo[]  = {
	{ T_STR, offsetof(ar_set_criteria_t, path),
	    offsetof(ar_set_criteria_t, change_flag), AR_ST_path },
	{ T_STR, offsetof(ar_set_criteria_t, name),
	    offsetof(ar_set_criteria_t, change_flag), AR_ST_name },
	{ T_SIZE, offsetof(ar_set_criteria_t, minsize),
	    offsetof(ar_set_criteria_t, change_flag), AR_ST_minsize },
	{ T_SIZE, offsetof(ar_set_criteria_t, maxsize),
	    offsetof(ar_set_criteria_t, change_flag), AR_ST_maxsize },
	{ T_STR, offsetof(ar_set_criteria_t, user),
	    offsetof(ar_set_criteria_t, change_flag), AR_ST_user },
	{ T_STR, offsetof(ar_set_criteria_t, group),
	    offsetof(ar_set_criteria_t, change_flag), AR_ST_group }
};
FIELD_RECORD archcrit_FORM[] = {

	{ SAMFS, mklabel, 0, 0, 3, 0, "Starting directory (required)" },
	{ SAMFS, mklabel, 0, 0, 5, 0, "Name (regexp)" },
	{ SAMFS, mklabel, 0, 0, 6, 0, "Minsize" },
	{ SAMFS, mklabel, 0, 0, 7, 0, "Maxsize" },
	{ SAMFS, mklabel, 0, 0, 8, 0, "User" },
	{ SAMFS, mklabel, 0, 0, 9, 0, "Group" },

	{ SAMFS, mkstring, 1, 30, 3, 30, NULL, NULL, &crit_cinfo[0] },
	{ SAMFS, mkstring, 1, 20, 5, 20, NULL, NULL, &crit_cinfo[1] },
	{ SAMFS, mksize, 1, 20, 6, 20, NULL, NULL, &crit_cinfo[2] },
	{ SAMFS, mksize, 1, 20, 7, 20, NULL, NULL, &crit_cinfo[3] },
	{ SAMFS, mkstring, 1, 20, 8, 20, NULL, NULL, &crit_cinfo[4] },
	{ SAMFS, mkstring, 1, 20, 9, 20, NULL, NULL, &crit_cinfo[5] },
	REC_TERM
};

/* fields 4 and 5 are set dynamically by set_arcopies_convinfo() in samc.c */
convinfo cpdir_cinfo[] = {
	{ T_BLN, 0, 0, 0 },
	{ T_TIME, 0, 0, AR_CP_ar_age },
	{ T_TIME, 0, 0, AR_CP_un_ar_age },
	{ T_BLN, 0, 0, 0 },
	{ T_TIME, 0, 0, AR_CP_ar_age },
	{ T_TIME, 0, 0, AR_CP_un_ar_age },
	{ T_BLN, 0, 0, 0 },
	{ T_TIME, 0, 0, AR_CP_ar_age },
	{ T_TIME, 0, 0, AR_CP_un_ar_age },
	{ T_BLN, 0, 0, 0 },
	{ T_TIME, 0, 0, AR_CP_ar_age },
	{ T_TIME, 0, 0, AR_CP_un_ar_age },
};
/* archive copy directives form */
FIELD_RECORD archcopydir_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 2, 0, "Copy 1" },
	{ SAMFS, mkenum, 1, 15, 2, 15, NULL, NULL, &cpdir_cinfo[0],
	    0, 0, 0, on_off_enum },
	{ SAMFS, mklabel, 0, 0, 4, 0, "Age (required)" },
	{ SAMFS, mktimein, 1, 15, 4, 15, NULL, NULL, &cpdir_cinfo[1] },
	{ SAMFS, mklabel, 0, 0, 5, 0, "Unarchive age" },
	{ SAMFS, mktimein, 1, 15, 5, 15, NULL, NULL, &cpdir_cinfo[2] },

	{ SAMFS, mklabel, 0, 0, 2, 40, "Copy 2" },
	{ SAMFS, mkenum, 1, 15, 2, 55, NULL, NULL, &cpdir_cinfo[3],
	    0, 0, 0, on_off_enum },
	{ SAMFS, mklabel, 0, 0, 4, 40, "Age (required)" },
	{ SAMFS, mktimein, 1, 15, 4, 55, NULL, NULL, &cpdir_cinfo[4] },
	{ SAMFS, mklabel, 0, 0, 5, 40, "Unarchive age" },
	{ SAMFS, mktimein, 1, 15, 5, 55, NULL, NULL, &cpdir_cinfo[5] },

	{ SAMFS, mklabel, 0, 0, 8, 0, "Copy 3" },
	{ SAMFS, mkenum, 1, 15, 8, 15, NULL, NULL, &cpdir_cinfo[6],
	    0, 0, 0, on_off_enum },
	{ SAMFS, mklabel, 0, 0, 10, 0, "Age (required)" },
	{ SAMFS, mktimein, 1, 15, 10, 15, NULL, NULL, &cpdir_cinfo[7] },
	{ SAMFS, mklabel, 0, 0, 11, 0, "Unarchive age" },
	{ SAMFS, mktimein, 1, 15, 11, 15, NULL, NULL, &cpdir_cinfo[8] },

	{ SAMFS, mklabel, 0, 0, 8, 40, "Copy 4" },
	{ SAMFS, mkenum, 1, 15, 8, 55, NULL, NULL, &cpdir_cinfo[9],
	    0, 0, 0, on_off_enum },
	{ SAMFS, mklabel, 0, 0, 10, 40, "Age (required)" },
	{ SAMFS, mktimein, 1, 15, 10, 55, NULL, NULL, &cpdir_cinfo[10] },
	{ SAMFS, mklabel, 0, 0, 11, 40, "Unarchive age" },
	{ SAMFS, mktimein, 1, 15, 11, 55, NULL, NULL, &cpdir_cinfo[11] },

	REC_TERM
};

/* archive copy parameters form */

char *join_enum[] = {
	"none",
	"path",
	NULL
};
char *sort_enum[] = {
	"none",
	"age",
	"path",
	"priority",
	"size",
	NULL
};
char *offlinecp_enum[] = {
	"none",
	"direct",
	"stageahead",
	"stageall",
	NULL
};
/* char *resrv_enum[] = { */
/* 	"none", */
/* 	"set", */
/* 	"dir" */
/* 	"user", */
/* 	"group", */
/* 	"fs", */
/* 	NULL */
/* }; */
#define	offsetof_cparams_mask offsetof(ar_set_copy_params_t, change_flag)
convinfo cparam_cinfo[] = {
	{ T_SIZE, offsetof(ar_set_copy_params_t, archmax),
	    offsetof_cparams_mask, AR_PARAM_archmax },
	{ T_INT, offsetof(ar_set_copy_params_t, bufsize),
	    offsetof_cparams_mask, AR_PARAM_bufsize },
	{ T_BLN, offsetof(ar_set_copy_params_t, buflock),
	    offsetof_cparams_mask, AR_PARAM_buflock },
	{ T_INT, offsetof(ar_set_copy_params_t, drives),
	    offsetof_cparams_mask, AR_PARAM_drives },
	{ T_SIZE, offsetof(ar_set_copy_params_t, drivemin),
	    offsetof_cparams_mask, AR_PARAM_drivemin },
	{ T_SIZE, offsetof(ar_set_copy_params_t, ovflmin),
	    offsetof_cparams_mask, AR_PARAM_ovflmin },
	{ T_JOIN, offsetof(ar_set_copy_params_t, join),
	    offsetof_cparams_mask, AR_PARAM_join },
	{ T_SORT, offsetof(ar_set_copy_params_t, sort),
	    offsetof_cparams_mask, AR_PARAM_sort },
	{ T_OFCP, offsetof(ar_set_copy_params_t, offline_copy),
	    offsetof_cparams_mask, AR_PARAM_offline_copy },
	{ T_BLN, offsetof(ar_set_copy_params_t, fillvsns),
	    offsetof_cparams_mask, AR_PARAM_fillvsns },
};
FIELD_RECORD archcopyparam_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 2, 0, "ArchMax" },
	{ SAMFS, mklabel, 0, 0, 3, 0, "Buffer size" },
	{ SAMFS, mklabel, 0, 0, 4, 0, "Buflock" },
	{ SAMFS, mklabel, 0, 0, 5, 0, "Drives" },
	{ SAMFS, mklabel, 0, 0, 6, 0, "Drive min" },
	{ SAMFS, mklabel, 0, 0, 7, 0, "Overflow min" },
	{ SAMFS, mklabel, 0, 0, 8, 0, "Join" },
	{ SAMFS, mklabel, 0, 0, 9, 0, "Sort" },
	{ SAMFS, mklabel, 0, 0, 10, 0, "Offline copy" },
	{ SAMFS, mklabel, 0, 0, 11, 0, "Fill VSNs" },

	{ SAMFS, mksize, 1, 20, 2, 20, NULL, NULL, &cparam_cinfo[0] },
	{ SAMFS, mkint, 1, 20, 3, 20, NULL, NULL, &cparam_cinfo[1] },
	{ SAMFS, mkenum, 1, 20, 4, 20, NULL, NULL, &cparam_cinfo[2], 0, 0, 0,
	    on_off_enum },
	{ SAMFS, mkint, 1, 20, 5, 20, NULL, NULL, &cparam_cinfo[3] },
	{ SAMFS, mksize, 1, 20, 6, 20, NULL, NULL, &cparam_cinfo[4] },
	{ SAMFS, mksize, 1, 20, 7, 20, NULL, NULL, &cparam_cinfo[5] },
	{ SAMFS, mkenum, 1, 20, 8, 20, NULL, NULL, &cparam_cinfo[6], 0, 0, 0,
	    join_enum },
	{ SAMFS, mkenum, 1, 20, 9, 20, NULL, NULL, &cparam_cinfo[7], 0, 0, 0,
	    sort_enum },
	{ SAMFS, mkenum, 1, 20, 10, 20, NULL, NULL, &cparam_cinfo[8], 0, 0, 0,
	    offlinecp_enum },
	{ SAMFS, mkenum, 1, 20, 11, 20, NULL, NULL, &cparam_cinfo[9], 0, 0, 0,
	    on_off_enum },
	REC_TERM
};
// ----------------------------- Disk archiving ---------------------------
#define	offsetof_dskvol_mask offsetof(disk_vol_t, set_flags)
convinfo dskvol_cinfo[] = {
	{ T_STR, offsetof(disk_vol_t, vol_name), offsetof_dskvol_mask, 0 },
	{ T_STR, offsetof(disk_vol_t, host), offsetof_dskvol_mask, 0 },
	{ T_STR, offsetof(disk_vol_t, path), offsetof_dskvol_mask, 0 }
};
// TO DO: replace 0 with the actual flags (not defined by the API yet)
FIELD_RECORD dskvol_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 5, 0, "Disk volume name" },
	{ SAMFS, mklabel, 0, 0, 7, 0, "Host name (optional)" },
	{ SAMFS, mklabel, 0, 0, 9, 0, "Path" },

	{ SAMFS, mkstring, MAXDSKVOLLEN, 20, 5, 23, NULL, NULL,
	    &dskvol_cinfo[0] },
	{ SAMFS, mkstring, 1, 20, 7, 23, NULL, NULL, &dskvol_cinfo[1] },
	{ SAMFS, mkstring, MAX_PATH_LENGTH, 20, 9, 23, NULL, NULL,
	    &dskvol_cinfo[2] },
	REC_TERM
};
FIELD_RECORD diskar_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 0, 7, "Archive policy" },
	{ SAMFS, mklabel, 0, 0, 0, 20 }, // name

	{ SAMFS, mklabel, 0, 0, 2, 0, "Copy number" },
	{ SAMFS, mklabel, 0, 0, 2, 30, "Archive volume name" },
	{ SAMFS, mklabel, 0, 0, 4, 5, "1" },
	{ SAMFS, mklabel, 0, 0, 6, 5, "2" },
	{ SAMFS, mklabel, 0, 0, 8, 5, "3" },
	{ SAMFS, mklabel, 0, 0, 10, 5, "4" },
	{ SAMFS, mkstring, 1, 20, 4, 30, "DISK01", "" },
	{ SAMFS, mkstring, 1, 20, 6, 30, "-", "" },
	{ SAMFS, mkstring, 1, 20, 8, 30, "DISK07", "" },
	{ SAMFS, mkstring, 1, 20, 10, 30, "-", "" },
	REC_TERM
};

// -------------------------- VSN pools -----------------------------------
convinfo pool_cinfo[] = {
	{ T_STR }, /* dataptr fields set at runtime by addpool() */
	{ T_STR },
	{ T_STR }
};
FIELD_RECORD pool_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 2, 0, "Pool name" },
	{ SAMFS, mklabel, 0, 0, 4, 0, "Media type" },
	{ SAMFS, mklabel, 0, 0, 6, 0, "VSN expressions" },

	{ SAMFS, mkstring, 1, 20, 2, 30, NULL, NULL, &pool_cinfo[0] },
	/*
	 * 'keywords' field for this enum initialized at runtime
	 * based on license info
	 */
	{ SAMFS, mkenum, 1, 20, 4, 30, NULL, NULL, &pool_cinfo[1] },
	{ SAMFS, mkstrbla, 1, 20, 6, 30, NULL, NULL, &pool_cinfo[2] },
	REC_TERM
};

// -------------------------- VSN associations ------------------------------
convinfo assoc_cinfo[] = {
	{ T_STR }, /* dataptr fields set at runtime by addassoc() */
	{ T_STR },
	{ T_STR },
	{ T_BLN }
};
FIELD_RECORD assoc_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 2, 0, "Archive copy" },
	{ SAMFS, mklabel, 0, 0, 4, 0, "Media type" },
	{ SAMFS, mklabel, 0, 0, 6, 0, "VSN expression" },
	{ SAMFS, mklabel, 0, 0, 8, 0, "Use VSN pools" },

	{ SAMFS, mkstring, 1, 20, 2, 30, NULL, NULL, &assoc_cinfo[0] },
	/* val field for the REC above set by modifas() and addassoc() */
	{ SAMFS, mkenum, 1, 20, 4, 30, NULL, NULL, &assoc_cinfo[1] },
	{ SAMFS, mkstring, 1, 20, 6, 30, NULL, NULL, &assoc_cinfo[2] },
	{ SAMFS, mkenum, 1, 10, 8, 30, "no", NULL, &assoc_cinfo[3],
	    0, 0, 0, yes_no_enum },
	REC_TERM
};


FIELD_RECORD drives_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 1, 0, "library name"},
	{ SAMFS, mklabel, 0, 0, 3, 0, "library1" },
	{ SAMFS, mklabel, 0, 0, 4, 0, "lib2" },
	{ SAMFS, mklabel, 0, 0, 5, 0, "library3" },

	{ SAMFS, mklabel, 0, 0, 1, 20, "# of staging drives"},
	{ SAMFS, mkint, 1, 5, 3, 20, "1" },
	{ SAMFS, mkint, 1, 5, 4, 20, "10" },
	{ SAMFS, mkint, 1, 5, 5, 20, "2" },

	REC_TERM
};

#define	offsetof_rel_mask offsetof(rl_fs_directive_t, change_flag)
#define	offsetof_ageprio offsetof(rl_fs_directive_t, age_priority)
convinfo rel_cinfo[] = {
	/* fields 0-5 */
	{ T_BLN,
	    offsetof(rl_fs_directive_t, no_release),
	    offsetof_rel_mask, RL_no_release },
	{ T_BLN,
	    offsetof(rl_fs_directive_t, rearch_no_release),
	    offsetof_rel_mask, RL_rearch_no_release },
	{ T_STR,
	    offsetof(rl_fs_directive_t, releaser_log),
	    offsetof_rel_mask, RL_releaser_log },
	{ T_BLN,
	    offsetof(rl_fs_directive_t, display_all_candidates),
	    offsetof_rel_mask, RL_display_all_candidates },
	{ T_LNG,
	    offsetof(rl_fs_directive_t, min_residence_age),
	    offsetof_rel_mask, RL_min_residence_age },

	/* weight fields  (5-10) */
	{ T_FLT, offsetof(rl_fs_directive_t, size_priority),
	    offsetof_rel_mask, RL_size_priority },
	{ T_AGE, offsetof(rl_fs_directive_t, type),
	    offsetof_rel_mask, RL_type },
	{ T_FLT, offsetof(rl_fs_directive_t, age_priority),
	    offsetof_rel_mask, RL_simple },
	{ T_FLT,
	    offsetof_ageprio + offsetof(rl_age_priority_t, access_weight),
	    offsetof_rel_mask, RL_access_weight },
	{ T_FLT,
	    offsetof_ageprio + offsetof(rl_age_priority_t, modify_weight),
	    offsetof_rel_mask, RL_modify_weight },
	{ T_FLT,
	    offsetof_ageprio + offsetof(rl_age_priority_t, residence_weight),
	    offsetof_rel_mask, RL_residence_weight }
};

FIELD_RECORD rel_FORM[] = {

	{ SAMFS, mklabel, 0, 0, 2, 40, "Miscellaneous directives" },

	{ SAMFS, mklabel, 0, 0, 4, 42, "no_release" },
	{ SAMFS, mklabel, 0, 0, 5, 42, "rearch_no_release" },
	{ SAMFS, mklabel, 0, 0, 7, 42, "logfile" },
	{ SAMFS, mklabel, 0, 0, 8, 42, "display_all_candidates" },
	{ SAMFS, mklabel, 0, 0, 10, 42, "min_residence_age (sec)" },

	{ SAMFS, mkenum, 1, 11, 4, 60,
	    NULL, "off", &rel_cinfo[0],  0, 0, 0,
	    on_off_enum },
	{ SAMFS, mkenum, 1, 11, 5, 60,
	    NULL, "off", &rel_cinfo[1],  0, 0, 0,
	    on_off_enum },
	{ SAMFS, mkstring, MAX_PATH_LENGTH, 25, 7, 50, NULL, NULL,
	    &rel_cinfo[2] },
	{ SAMFS, mkenum, 1, 11, 8, 66,
	    NULL, NULL, &rel_cinfo[3],  0, 0, 0,
	    on_off_enum },
	{ SAMFS, mkint, 1, 11, 10, 66, NULL, NULL, &rel_cinfo[4] },

	{ SAMFS, mklabel, 0, 0, 2, 0, "Weight directives" },

	{ SAMFS, mklabel, 0, 0, 4, 2, "weight_size" },
	{ SAMFS, mkdbl, 1, 15, 4, 24, NULL, NULL, &rel_cinfo[5],
	    4, 0, 1 },

	{ SAMFS, mklabel, 0, 0, 6, 2, "weight_age" },
	{ SAMFS, mkdbl, 1, 15, 6, 24, NULL, NULL, &rel_cinfo[7],
	    4, 0, 1 },  // index = 15 used in samc.c
	{ SAMFS, mklabel, 0, 0, 8, 2, "Detailed age directives" },
	{ SAMFS, mkenumtrig, 1, 10, 8, 28, NULL, "off", &rel_cinfo[6],
	    0, 0, 0, on_off_enum },  // index = 17 used in samc.c
	{ SAMFS, mklabel, 0, 0,  9, 2, "weight_age_access" },
	{ SAMFS, mklabel, 0, 0, 10, 2, "weight_age_modify" },
	{ SAMFS, mklabel, 0, 0, 11, 2, "weight_age_residence" },
	{ SAMFS, mkdbl, 1, 15,  9, 24, NULL, NULL, &rel_cinfo[8],
	    4, 0, 1 },  // index = 21 used in samc.c
	{ SAMFS, mkdbl, 1, 15, 10, 24, NULL, NULL, &rel_cinfo[9],
	    4, 0, 1 },  // index = 22 used in samc.c
	{ SAMFS, mkdbl, 1, 15, 11, 24, NULL, NULL, &rel_cinfo[10],
	    4, 0, 1 },    // index = 23 samc.c

	REC_TERM
};

// -------------------------- Recycling -----------------------------------

#define	offsetof_rbrec_mask offsetof(rc_param_t, change_flag)
convinfo rbrec_cinfo[] = {
	{ T_SIZE, offsetof(rc_param_t, data_quantity), offsetof_rbrec_mask,
	    RC_data_quantity },
	{ T_INT, offsetof(rc_param_t, hwm), offsetof_rbrec_mask,
	    RC_hwm },
	{ T_BLN, offsetof(rc_param_t, ignore), offsetof_rbrec_mask,
	    RC_ignore },
	{ T_STR, offsetof(rc_param_t, email_addr), offsetof_rbrec_mask,
	    RC_email_addr },
	{ T_INT, offsetof(rc_param_t, mingain), offsetof_rbrec_mask,
	    RC_mingain },
	{ T_INT, offsetof(rc_param_t, vsncount), offsetof_rbrec_mask,
	    RC_vsncount }
};
FIELD_RECORD rbrecycle_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 4, 0, "Dataquantity" },
	{ SAMFS, mklabel, 0, 0, 5, 0, "High watermark (%)" },
	{ SAMFS, mklabel, 0, 0, 6, 0, "Ignore" },
	{ SAMFS, mklabel, 0, 0, 7, 0, "Email address" },
	{ SAMFS, mklabel, 0, 0, 8, 0, "Mingain (%)" },
	{ SAMFS, mklabel, 0, 0, 9, 0, "VSNcount" },

	{ SAMFS, mksize, 1, 15, 4, 20, NULL, NULL, &rbrec_cinfo[0] },
	{ SAMFS, mkint,    1, 15, 5, 20, NULL, NULL, &rbrec_cinfo[1] },
	{ SAMFS, mkenum,   1, 10, 6, 20, NULL, NULL, &rbrec_cinfo[2],
	    0, 0, 0, on_off_enum },
	{ SAMFS, mkstring, MAX_EMAIL_LEN, 30, 7, 20, NULL, NULL,
	    &rbrec_cinfo[3] },
	{ SAMFS, mkint,    1, 15, 8, 20, NULL, NULL, &rbrec_cinfo[4] },
	{ SAMFS, mkint,    1, 15, 9, 20, NULL, NULL, &rbrec_cinfo[5] },

	REC_TERM
};

// -------------------------- Removable Media --------------------------------

convinfo media_cinfo[]  = {
	{ T_STR, offsetof(base_dev_t, set) },
	{ T_UINT16, offsetof(base_dev_t, eq) },
	{ T_STR, offsetof(base_dev_t, equ_type) },
	{ T_STR, offsetof(base_dev_t, additional_params) }
};

FIELD_RECORD lib_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 2, 0, "Name (required)" },
	{ SAMFS, mklabel, 0, 0, 3, 0, "Eq number" },
	{ SAMFS, mklabel, 0, 0, 4, 0, "Eq type" },
	{ SAMFS, mklabel, 0, 0, 5, 0, "Parameters file" },

	{ SAMFS, mkstring, 1, 15, 2, 20, NULL, NULL, &media_cinfo[0] },
	{ SAMFS, mkint,    1, 15, 3, 20, NULL, "auto", &media_cinfo[1] },
	{ SAMFS, mkstring, 1, 15, 4, 20, NULL, NULL, &media_cinfo[2] },
	/* eq type value set by addlib() */
	{ SAMFS, mkstring, MAX_PATH_LENGTH, 20, 5, 20, NULL, NULL,
	    &media_cinfo[3] },

	REC_TERM
};

FIELD_RECORD sdrive_FORM[] = {
	{ SAMFS, mklabel, 0, 0, 2, 0, "Name (required)" },
	{ SAMFS, mklabel, 0, 0, 3, 0, "Eq number" },
	{ SAMFS, mklabel, 0, 0, 4, 0, "Eq type" },

	{ SAMFS, mkstring, 1, 15, 2, 20, NULL, NULL, &media_cinfo[0] },
	{ SAMFS, mkint,    1, 15, 3, 20, NULL, "auto", &media_cinfo[1] },
	{ SAMFS, mkstring, 1, 15, 4, 20, NULL, NULL, &media_cinfo[2] },
	/* eq type set by addsdrv() */

	REC_TERM
};

#endif /* _SAMC_FORMDEFS_H */
