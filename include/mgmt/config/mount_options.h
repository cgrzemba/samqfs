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
#ifndef _CFG_MOUNT_OPTIONS_H
#define	_CFG_MOUNT_OPTIONS_H

#pragma ident	"$Revision: 1.22 $"


/*
 * mount_options.h
 * contains the setfield tables for the mount options structures used for
 * processing samfs.cmd for the api structs.
 */
#include <sys/types.h>

#include "sam/setfield.h"
#include "sam/param.h"

#include "pub/mgmt/filesystem.h"

static struct fieldInt mp_0 = { -1, 0, 1, 0 };
static struct fieldInt mp_1 = { 80, 0, 100, 0 };
static struct fieldInt mp_2 = { 70, 0, 100, 0 };
static struct fieldInt mp_3 = { SAM_DEFRA, SAM_MINRA,
	SAM_MAXRA, 0 };
static struct fieldInt mp_4 = { SAM_DEFWB, SAM_MINWB,
	SAM_MAXWB, 0 };
static struct fieldInt mp_5 = { 0, 0, 8192, 0 };
static struct fieldInt mp_6 = { 0, 0, 8192, 0 };
static struct fieldInt mp_7 = { SAM_DEFPARTIAL, SAM_MINPARTIAL,
	SAM_MAXPARTIAL, 0 };
static struct fieldInt mp_8 = { SAM_DEFPARTIAL, 0,
	SAM_MAXPARTIAL, 0 };
static struct fieldInt mp_9 = { -1, 0, SAM_MAXPARTIAL, 0 };
static struct fieldInt mp_10 = { SAM_DEFSWINDOW, SAM_MINSWINDOW,
	SAM_MAXSWINDOW, 0 };
static struct fieldInt mp_12 = { MAX_STAGE_RETRIES_DEF, 0,
	INT_MAX, 0 };
static struct fieldInt mp_13 = { -1, 0, 255, 0 };

static struct fieldFlag mp_readonly = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_quota = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_noquota = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldInt mp_rd_ino_buf_size = { 16384, 1024,
	16384, 0 };
static struct fieldInt mp_wr_ino_buf_size = { 512, 512,
	16384, 0 };
static struct fieldFlag mp_worm_capable = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_gfsid = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_nogfsid = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldInt mp_14 = { 1, 0, 255, 0 };
static struct fieldInt mp_15 = { SAM_DEFWR, 0, 33554432, 0};
static struct fieldFlag mp_forcenfsasync = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_noforcenfsasync = { (uint32_t)B_TRUE, "off",
	"on", "off" };

static struct fieldInt mp_16 = { SAM_CONS_AUTO, 0, INT_MAX, 0 };
static struct fieldInt mp_17 = { SAM_CONS_AUTO, 0, INT_MAX, 0 };
static struct fieldInt mp_18 = { SAM_MINWF_AUTO, 0, INT_MAX, 0 };
static struct fieldInt mp_19 = { SAM_MINWF_AUTO, 0, INT_MAX, 0 };
static struct fieldInt mp_20 = { SAM_MINIF_AUTO, 0, INT_MAX, 0 };
static struct fieldInt mp_21 = { SAM_MINIF_AUTO, 0, INT_MAX, 0 };
static struct fieldFlag mp_22 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_suid = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_23 = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_24 = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_25 = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_26 = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_arscan = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_noarscan = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_oldarchive = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_newarchive = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_27 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_noforcedirectio = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_29 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_noqwrite = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_30 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_31 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_32 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_33 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_34 = { (uint32_t)B_TRUE, "off",
	"on", "off" };

static struct fieldFlag mp_37 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_nosw_raid = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_38 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_nohwm_archive = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_39 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_nomh_write = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_40 = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldInt mp_42 = { 10000, 0, 20000, 0 };
static struct fieldInt mp_43 = { 0, 0, 60, 0 };
static struct fieldInt mp_44 = { -1, 16, 2097152, 0 };
static struct fieldInt mp_45 = { -1, 16, 4194304, 0 };
static struct fieldInt mp_46 = { DEF_LEASE_TIME, MIN_LEASE_TIME,
	MAX_LEASE_TIME, 0 };
static struct fieldInt mp_47 = { DEF_LEASE_TIME, MIN_LEASE_TIME,
	MAX_LEASE_TIME, 0 };
static struct fieldInt mp_48 = { DEF_LEASE_TIME, MIN_LEASE_TIME,
	MAX_LEASE_TIME, 0 };
static struct fieldInt mp_49 = { DEF_META_TIMEO, MIN_META_TIMEO,
	MAX_META_TIMEO, 0 };
static struct fieldInt mp_lease_timeo = { 0, -1, MIN_LEASE_TIME, 0 };
static struct fieldFlag mp_soft = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldInt mp_50 = { 256, 8, 2048, 0 };
static struct fieldFlag mp_refresh_at_eof =
	{ (uint32_t)B_TRUE, "off", "on", "off" };
static struct fieldFlag mp_norefresh_at_eof =
	{ (uint32_t)B_TRUE, "off", "on", "off" };

static struct fieldInt mp_def_retention = { DEFAULT_RPERIOD,
	MIN_RPERIOD, MAX_RPERIOD, 0 };
static struct fieldFlag mp_abr = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_noabr = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_dmr = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_nodmr = { (uint32_t)B_TRUE, "on",
	"on", "off" };
static struct fieldFlag mp_dio_szero = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_nodio_szero = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_cattr = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_nocattr = { (uint32_t)B_TRUE, "off",
	"on", "off" };

static struct fieldFlag mp_worm_emul = { (uint32_t)B_TRUE,
	"off", "on", "off" };
static struct fieldFlag mp_worm_lite = { (uint32_t)B_TRUE,
	"off", "on", "off" };
static struct fieldFlag mp_emul_lite = { (uint32_t)B_TRUE,
	"off", "on", "off" };
static struct fieldFlag mp_cdevid = { (uint32_t)B_TRUE,
	"on", "on", "off" };
static struct fieldFlag mp_nocdevid = { (uint32_t)B_TRUE,
	"on", "on", "off" };
static struct fieldFlag mp_clustermgmt = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_noclustermgmt = { (uint32_t)B_TRUE, "off",
	"on", "off" };
static struct fieldFlag mp_clusterfastsw = { (uint32_t)B_TRUE,
	"off", "on", "off" };
static struct fieldFlag mp_noclusterfastsw = { (uint32_t)B_TRUE,
	"off", "on", "off" };
static struct fieldFlag mp_noatime = { (uint32_t)B_TRUE,
	"off", "on", "off" };
static struct fieldInt mp_atime = { 0, -1, 1, 0 };
static struct fieldInt mp_min_pool = { 64, 8, 2048, 0 };
static int mount_params_defbits;

static struct fieldVals cfg_mount_params[] = {

/* base mnt opts */
{ "", DEFBITS, offsetof(struct mount_options, change_flag),
	&mount_params_defbits, },
{ "sync_meta", INT16, offsetof(struct mount_options, sync_meta),
	&mp_0, MNT_SYNC_META },
{ "nosuid", SETFLAG, offsetof(struct mount_options, no_suid),
	&mp_22, MNT_NOSUID },
{ "suid", CLEARFLAG, offsetof(struct mount_options, no_suid),
	&mp_suid, MNT_SUID },
{ "trace", SETFLAG, offsetof(struct mount_options, trace),
	&mp_23, MNT_TRACE },
{ "notrace", CLEARFLAG, offsetof(struct mount_options, trace),
	&mp_24, MNT_NOTRACE },
{ "stripe", INT16, offsetof(struct mount_options, stripe),
	&mp_13, MNT_STRIPE },
{ "ro",	SETFLAG, offsetof(struct mount_options, readonly),
	&mp_readonly, MNT_READONLY },
{ "quota", SETFLAG, offsetof(struct mount_options, quota),
	&mp_quota, MNT_QUOTA },
{ "noquota", CLEARFLAG, offsetof(struct mount_options, quota),
	&mp_noquota, MNT_NOQUOTA },
{ "rd_ino_buf_size", INT, offsetof(struct mount_options, rd_ino_buf_size),
	&mp_rd_ino_buf_size, MNT_RD_INO_BUF_SIZE },
{ "wr_ino_buf_size", INT, offsetof(struct mount_options, wr_ino_buf_size),
	&mp_wr_ino_buf_size, MNT_WR_INO_BUF_SIZE },
{ "worm_capable", SETFLAG, offsetof(struct mount_options, worm_capable),
	&mp_worm_capable, MNT_WORM_CAPABLE },
{ "gfsid", SETFLAG, offsetof(struct mount_options, gfsid), &mp_gfsid,
	MNT_GFSID },
{ "nogfsid", CLEARFLAG, offsetof(struct mount_options, gfsid), &mp_nogfsid,
	MNT_NOGFSID },


/* io defbits line */
{ "", DEFBITS, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, change_flag),
	&mount_params_defbits, },
{ "dio_rd_consec", INT, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, dio_rd_consec), &mp_16,
	MNT_DIO_RD_CONSEC },
{ "dio_rd_form_min", INT, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, dio_rd_form_min),
	&mp_18, MNT_DIO_RD_FORM_MIN },
{ "dio_rd_ill_min", INT, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, dio_rd_ill_min), &mp_20,
	MNT_DIO_RD_ILL_MIN },
{ "dio_wr_consec", INT, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, dio_wr_consec), &mp_17,
	MNT_DIO_WR_CONSEC },
{ "dio_wr_form_min", INT, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, dio_wr_form_min),
	&mp_19, MNT_DIO_WR_FORM_MIN },
{ "dio_wr_ill_min", INT, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, dio_wr_ill_min), &mp_21,
	MNT_DIO_WR_ILL_MIN },
{ "forcedirectio", SETFLAG, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, forcedirectio), &mp_27,
	MNT_FORCEDIRECTIO },
{ "noforcedirectio", CLEARFLAG, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, forcedirectio), &mp_noforcedirectio,
	MNT_NOFORCEDIRECTIO },
{ "sw_raid", SETFLAG, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, sw_raid), &mp_37,
	MNT_SW_RAID },
{ "nosw_raid", CLEARFLAG, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, sw_raid), &mp_nosw_raid,
	MNT_NOSW_RAID },
{ "flush_behind", MUL8, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, flush_behind), &mp_5,
	MNT_FLUSH_BEHIND },
{ "readahead", MULL8, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, readahead), &mp_3,
	MNT_READAHEAD },
{ "writebehind", MULL8, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, writebehind), &mp_4,
	MNT_WRITEBEHIND },
{ "wr_throttle", INT64, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, wr_throttle), &mp_15,
	MNT_WR_THROTTLE },
{ "force_nfs_async", SETFLAG, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, forcenfsasync), &mp_forcenfsasync,
	MNT_FORCENFSASYNC },
{ "noforce_nfs_async", CLEARFLAG, offsetof(struct mount_options, io_opts) +
	offsetof(struct io_mount_options, forcenfsasync), &mp_noforcenfsasync,
	MNT_NOFORCENFSASYNC },


/* sam defbits line */
{ "", DEFBITS, offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, change_flag),
	&mount_params_defbits, },
{ "high", INT16,  offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, high), &mp_1,
	MNT_HIGH },
{ "low", INT16,  offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, low), &mp_2,
	MNT_LOW },
{ "partial", MUL8,  offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, partial), &mp_7,
	MNT_PARTIAL },
{ "maxpartial", MUL8,	offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, maxpartial), &mp_8,
	MNT_MAXPARTIAL },
{ "partial_stage", MUL8,  offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, partial_stage), &mp_9,
	MNT_PARTIAL_STAGE },
{ "stage_n_window", MUL8,  offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, stage_n_window),
	&mp_10, MNT_STAGE_N_WINDOW },
{ "stage_retries", INT,  offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, stage_retries),
	&mp_12, MNT_STAGE_RETRIES },
{ "stage_flush_behind", MUL8,	offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, stage_flush_behind),
	&mp_6, MNT_STAGE_FLUSH_BEHIND },
{ "hwm_archive", SETFLAG,  offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, hwm_archive), &mp_38,
	MNT_HWM_ARCHIVE },
{ "nohwm_archive", CLEARFLAG,  offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, hwm_archive), &mp_nohwm_archive,
	MNT_NOHWM_ARCHIVE },
{ "sam", SETFLAG, offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, archive), &mp_25,
	MNT_ARCHIVE },
{ "nosam", CLEARFLAG, offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, archive), &mp_26,
	MNT_NOARCHIVE },
{ "arscan", SETFLAG, offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, arscan), &mp_arscan,
	MNT_ARSCAN },
{ "noarscan", CLEARFLAG, offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, arscan), &mp_noarscan,
	MNT_NOARSCAN },
{ "oldarchive", SETFLAG, offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, oldarchive), &mp_oldarchive,
	MNT_OLDARCHIVE },
{ "newarchive", CLEARFLAG, offsetof(struct mount_options, sam_opts) +
	offsetof(struct sam_mount_options, oldarchive), &mp_newarchive,
	MNT_NEWARCHIVE },

/* shared fs defbits line */
{ "", DEFBITS, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, change_flag),
	&mount_params_defbits, },
{ "shared", SETFLAG, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, shared), &mp_30,
	MNT_SHARED },
{ "bg", SETFLAG, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, bg), &mp_40,
	MNT_BG },
{ "retry", INT16, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, retry), &mp_42,
	MNT_RETRY },
{ "minallocsz", INT64, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, minallocsz),
	&mp_44, MNT_MINALLOCSZ	},
{ "maxallocsz", INT64, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, maxallocsz),
	&mp_45, MNT_MAXALLOCSZ },
{ "rdlease", INT, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, rdlease), &mp_46,
	MNT_RDLEASE },
{ "wrlease", INT, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, wrlease), &mp_47,
	MNT_WRLEASE },
{ "aplease", INT, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, aplease), &mp_48,
	MNT_APLEASE },
{ "mh_write", SETFLAG, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, mh_write), &mp_39,
	MNT_MH_WRITE },
{ "nomh_write", CLEARFLAG, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, mh_write), &mp_nomh_write,
	MNT_NOMH_WRITE },
{ "nstreams", INT, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, nstreams), &mp_50,
	MNT_NSTREAMS },
{ "meta_timeo", INT, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, meta_timeo),
	&mp_49, MNT_META_TIMEO },
{ "lease_timeo", INT, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, lease_timeo),
	&mp_lease_timeo, MNT_LEASE_TIMEO },
{ "soft", SETFLAG, offsetof(struct mount_options, sharedfs_opts) +
	offsetof(struct sharedfs_mount_options, soft), &mp_soft,
	MNT_SOFT },


/* multireader_mount_options defbits line */
{ "", DEFBITS, offsetof(struct mount_options, multireader_opts) +
	offsetof(struct multireader_mount_options, change_flag),
	&mount_params_defbits, },
{ "writer", SETFLAG, offsetof(struct mount_options, multireader_opts) +
	offsetof(struct multireader_mount_options, writer),
	&mp_32, MNT_WRITER },
{ "shared_writer", SETFLAG, offsetof(struct mount_options, multireader_opts) +
	offsetof(struct multireader_mount_options, writer),
	&mp_31, MNT_SHARED_WRITER },
{ "reader", SETFLAG, offsetof(struct mount_options, multireader_opts) +
	offsetof(struct multireader_mount_options, reader),
	&mp_34, MNT_READER },
{ "shared_reader", SETFLAG, offsetof(struct mount_options, multireader_opts) +
	offsetof(struct multireader_mount_options, reader),
	&mp_33, MNT_SHARED_READER },
{ "invalid", INT, offsetof(struct mount_options, multireader_opts) +
	offsetof(struct multireader_mount_options, invalid),
	&mp_43, MNT_INVALID },
{ "refresh_at_eof", SETFLAG, offsetof(struct mount_options, multireader_opts) +
	offsetof(struct multireader_mount_options, refresh_at_eof),
	&mp_refresh_at_eof, MNT_REFRESH_AT_EOF },
{ "norefresh_at_eof", CLEARFLAG,
	offsetof(struct mount_options, multireader_opts) +
	offsetof(struct multireader_mount_options, refresh_at_eof),
	&mp_norefresh_at_eof, MNT_NOREFRESH_AT_EOF },

/* qwrite defbits line */
{ "", DEFBITS, offsetof(struct mount_options, qfs_opts) +
	offsetof(struct qfs_mount_options, change_flag),
	&mount_params_defbits, },
{ "qwrite", SETFLAG, offsetof(struct mount_options, qfs_opts) +
	offsetof(struct qfs_mount_options, qwrite), &mp_29,
	MNT_QWRITE },
{ "noqwrite", CLEARFLAG, offsetof(struct mount_options, qfs_opts) +
	offsetof(struct qfs_mount_options, qwrite), &mp_noqwrite,
	MNT_NOQWRITE },
{ "mm_stripe", INT16, offsetof(struct mount_options, qfs_opts) +
	offsetof(struct qfs_mount_options, mm_stripe), &mp_14,
	MNT_MM_STRIPE },

/* post 4.3 defbits line */
{ "", DEFBITS, offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, change_flag),
	&mount_params_defbits, },
{ "def_retention", INT, offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, def_retention), &mp_def_retention,
	MNT_DEF_RETENTION},

{ "abr", SETFLAG, offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, abr), &mp_abr, MNT_ABR },
{ "noabr", CLEARFLAG, offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, abr), &mp_noabr, MNT_NOABR },
{ "dmr", SETFLAG, offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, dmr), &mp_dmr, MNT_DMR },
{ "nodmr", CLEARFLAG, offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, dmr), &mp_nodmr, MNT_NODMR },
{ "dio_szero", SETFLAG,  offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, dio_szero), &mp_dio_szero,
	MNT_DIO_SZERO },

{ "nodio_szero", CLEARFLAG,  offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, dio_szero), &mp_nodio_szero,
	MNT_NODIO_SZERO },
{ "cattr", SETFLAG,  offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, cattr), &mp_cattr,
	MNT_CATTR },
{ "nocattr", CLEARFLAG,  offsetof(struct mount_options, post_4_2_opts) +
	offsetof(struct post_4_2_options, cattr), &mp_nocattr,
	MNT_NOCATTR },

/* 4.6 opts defbits line */
{ "", DEFBITS, offsetof(struct mount_options, rel_4_6_opts) +
	offsetof(struct rel_4_6_options, change_flag),
	&mount_params_defbits, },
{ "worm_emul", SETFLAG, offsetof(struct mount_options, rel_4_6_opts) +
	offsetof(struct rel_4_6_options, worm_emul), &mp_worm_emul,
	MNT_WORM_EMUL },
{ "worm_lite", SETFLAG, offsetof(struct mount_options, rel_4_6_opts) +
	offsetof(struct rel_4_6_options, worm_lite), &mp_worm_lite,
	MNT_WORM_LITE },
{ "emul_lite", SETFLAG, offsetof(struct mount_options, rel_4_6_opts) +
	offsetof(struct rel_4_6_options, emul_lite), &mp_emul_lite,
	MNT_WORM_EMUL_LITE },
{ "cdevid", SETFLAG, offsetof(struct mount_options, rel_4_6_opts) +
	offsetof(struct rel_4_6_options, cdevid), &mp_cdevid,
	MNT_CDEVID },
{ "nocdevid", CLEARFLAG, offsetof(struct mount_options,
	rel_4_6_opts) + offsetof(struct rel_4_6_options, cdevid),
	&mp_nocdevid, MNT_NOCDEVID },
{ "clustermgmt", SETFLAG, offsetof(struct mount_options,
	rel_4_6_opts) + offsetof(struct rel_4_6_options, clustermgmt),
	&mp_clustermgmt, MNT_CLMGMT },
{ "noclustermgmt", CLEARFLAG, offsetof(struct mount_options,
	rel_4_6_opts) + offsetof(struct rel_4_6_options, clustermgmt),
	&mp_noclustermgmt, MNT_NOCLMGMT },
{ "clusterfastsw", SETFLAG, offsetof(struct mount_options,
	rel_4_6_opts) + offsetof(struct rel_4_6_options, clusterfastsw),
	&mp_clusterfastsw, MNT_CLFASTSW },
{ "noclusterfastsw", CLEARFLAG, offsetof(struct mount_options,
	rel_4_6_opts) + offsetof(struct rel_4_6_options, clusterfastsw),
	&mp_noclusterfastsw, MNT_NOCLFASTSW },
{ "noatime", SETFLAG, offsetof(struct mount_options, rel_4_6_opts) +
	offsetof(struct rel_4_6_options, noatime), &mp_noatime,
	MNT_NOATIME },
{ "atime", INT16, offsetof(struct mount_options, rel_4_6_opts) +
	offsetof(struct rel_4_6_options, atime), &mp_atime,
	MNT_ATIME },
{ "min_pool", INT, offsetof(struct mount_options, rel_4_6_opts) +
	offsetof(struct rel_4_6_options, min_pool), &mp_min_pool,
	MNT_MIN_POOL},
{ NULL }
};

/*
 * This array must contain a mapping between the boolean entries and
 * their unset values. For example at the location in this array, that
 * corresponds to the location in cfg_mount_params which contains the trace
 * entry, the flag for no trace should appear and vice versa. This pairing
 * is important for the implementation of vfstab mount options and is used
 * to determine if either the flag or its pair has been set in the vfstab.
 */
static int32_t flag_pairs[98] = {
	/* general paired flags */
	0, 0, MNT_SUID, MNT_NOSUID, MNT_NOTRACE,
	MNT_TRACE, 0, 0, MNT_NOQUOTA, MNT_QUOTA,
	0, 0, 0, MNT_NOGFSID, MNT_GFSID,

	/* io paired flags */
	0, 0, 0, 0, 0,
	0, 0, MNT_NOFORCEDIRECTIO, MNT_FORCEDIRECTIO, MNT_NOSW_RAID,
	MNT_SW_RAID, 0, 0, 0, 0,
	MNT_NOFORCENFSASYNC, MNT_FORCENFSASYNC,

	/* sam paired flags */
	0, 0, 0, 0, 0,
	0, 0, 0, 0, MNT_NOHWM_ARCHIVE,
	MNT_HWM_ARCHIVE, MNT_NOARCHIVE, MNT_ARCHIVE, MNT_NOARSCAN, MNT_ARSCAN,
	MNT_NEWARCHIVE, MNT_OLDARCHIVE,

	/* sharedfs paired flags */
	0, 0, 0, 0, 0,
	0, 0, 0, 0, MNT_NOMH_WRITE,
	MNT_MH_WRITE, 0, 0, 0, 0,

	/* multireader paired flags */
	0, 0, 0, 0, 0,
	0, MNT_NOREFRESH_AT_EOF, MNT_REFRESH_AT_EOF,

	/* qfs paired flags */
	0, MNT_NOQWRITE, MNT_QWRITE, 0,

	/* Post 4.2 Mount Options */
	0, 0, MNT_NOABR, MNT_ABR, MNT_NODMR,
	MNT_DMR, MNT_NODIO_SZERO, MNT_DIO_SZERO, MNT_NOCATTR, MNT_CATTR,

	/* rel 4.6 Mount Options */
	0, 0, 0, MNT_NOCDEVID, MNT_CDEVID,
	MNT_NOCLMGMT, MNT_CLMGMT, MNT_NOCLFASTSW, MNT_CLFASTSW, 0,
	0, 0

};


/*
 * This array contains the names of mount_options that should not be written
 * to the samfs.cmd file. Some of the entries don't show up in the man page
 * with their paired options for example suid is not listed with nosuid. The
 * absence of nosuid implies suid so it can be left out.
 *
 * Some of them are outdated names for current mount_options. For example
 * shared_reader is the old name for the reader mount option. These are
 * omitted in favor of the inclusion of the new name in the file.
 */
static char non_printing_mount_options[][20] = {
	"suid", "shared_reader", "shared_writer", "noforcedirectio",
	"nosw_raid", "noforce_nfs_async", "nohwm_archive", "nomh_write",
	"norefresh_at_eof", "noqwrite", '\0'};

#endif /* _CFG_MOUNT_OPTIONS_H */
