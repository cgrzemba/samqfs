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
#pragma ident "$Revision: 1.21 $"

#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include "pub/mgmt/error.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/util.h"

#include "pub/mgmt/filesystem.h"
#include "sam/setfield.h"
#include "sam/sam_trace.h"
#include "sam/mount.h"
#include "sam/lib.h"

#define	numof(a) (sizeof (a)/sizeof (*(a))) /* Number of elements in array a */

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/* Error return message. Empty ("") if no error. */
static char *
CmdSetFsConfig(char *fsname, char *param, char *value);

typedef enum { SAM_NONE = 0, QFS_STANDALONE, SAM_DISK, SAM_REMOVABLE }
sam_level;

static int set_live_mount_options(ctx_t *ctx, uname_t fsname,
    mount_options_t *options, sqm_lst_t **failed_options);

/* Private data. */
static struct {
	char *name;		/* Command name */
	uint16_t FvType;	/* Data type in field */
	int	FvLoc;		/* Offset of field in structure */
	int	FvFlagLoc;	/* Offset of field change_flag in structure */
	sam_level sam_lvl;	/* lowest level that this command is valid */
	char *(*cmd)(upath_t, upath_t, upath_t);	/* Command processor */
	int	mount_option;
	failed_mount_option_t	failed_mount_option;
} cmd_t[] = {
	/* general option from 1 - 6 */
	{ "", DEFBITS, offsetof(struct mount_options, change_flag),
		offsetof(struct mount_options, change_flag),
		QFS_STANDALONE, NULL, 0, 0},
	{ "suid", SETFLAG, offsetof(struct mount_options, no_suid),
		offsetof(struct mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_SUID, FAILED_MNT_SUID},
	{ "nosuid", SETFLAG, offsetof(struct mount_options, no_suid),
		offsetof(struct mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NOSUID, FAILED_MNT_NOSUID},
	{ "sync_meta", INT16, offsetof(struct mount_options, sync_meta),
		offsetof(struct mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_SYNC_META, FAILED_MNT_SYNC_META},
	{ "trace", SETFLAG, offsetof(struct mount_options, trace),
		offsetof(struct mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_TRACE, FAILED_MNT_TRACE},
	{ "notrace", SETFLAG, offsetof(struct mount_options, trace),
		offsetof(struct mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NOTRACE, FAILED_MNT_NOTRACE},
	{ "stripe", INT16, offsetof(struct mount_options, stripe),
		offsetof(struct mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_STRIPE, FAILED_MNT_STRIPE},


	/* io option from 7 - 20 */
	{ "dio_rd_consec", INT, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, dio_rd_consec),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_DIO_RD_CONSEC, FAILED_MNT_DIO_RD_CONSEC},
	{ "dio_rd_form_min", INT, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, dio_rd_form_min),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_DIO_RD_FORM_MIN, FAILED_MNT_DIO_RD_FORM_MIN},
	{ "dio_rd_ill_min", INT, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, dio_rd_ill_min),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_DIO_RD_ILL_MIN, FAILED_MNT_DIO_RD_ILL_MIN},
	{ "dio_wr_consec", INT, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, dio_wr_consec),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE,  SetFsParam,
		MNT_DIO_WR_CONSEC, FAILED_MNT_DIO_WR_CONSEC},
	{ "dio_wr_form_min", INT, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, dio_wr_form_min),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_DIO_WR_FORM_MIN, FAILED_MNT_DIO_WR_FORM_MIN},
	{ "dio_wr_ill_min", INT, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, dio_wr_ill_min),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_DIO_WR_ILL_MIN, FAILED_MNT_DIO_WR_ILL_MIN},
	{ "forcedirectio", SETFLAG, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, forcedirectio),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_FORCEDIRECTIO, FAILED_MNT_FORCEDIRECTIO},
	{ "noforcedirectio", SETFLAG, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, forcedirectio),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NOFORCEDIRECTIO, FAILED_MNT_NOFORCEDIRECTIO},
	{ "sw_raid", SETFLAG, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, sw_raid),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_SW_RAID, FAILED_MNT_SW_RAID},
	{ "nosw_raid", SETFLAG, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, sw_raid),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NOSW_RAID, FAILED_MNT_NOSW_RAID},
	{ "flush_behind", MUL8, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, flush_behind),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_FLUSH_BEHIND, FAILED_MNT_FLUSH_BEHIND},
	{ "readahead", MULL8, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, readahead),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_READAHEAD, FAILED_MNT_READAHEAD},
	{ "writebehind", MULL8, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, writebehind),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_WRITEBEHIND, FAILED_MNT_WRITEBEHIND},
	{ "wr_throttle", INT64, offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, wr_throttle),
		offsetof(struct mount_options, io_opts) +
		offsetof(struct io_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_WR_THROTTLE, FAILED_MNT_WR_THROTTLE},

	/* New options after rel4.2 */
	{ "abr", SETFLAG, offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, abr),
		offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_ABR, FAILED_MNT_ABR},
	{ "noabr", SETFLAG, offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, abr),
		offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NOABR, FAILED_MNT_NOABR},

	{ "dmr", SETFLAG, offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, dmr),
		offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_DMR, FAILED_MNT_DMR},
	{ "nodmr", SETFLAG, offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, dmr),
		offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NODMR, FAILED_MNT_NODMR},

	{ "dio_szero", SETFLAG, offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, dio_szero),
		offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_DIO_SZERO, FAILED_MNT_DIO_SZERO},
	{ "nodio_szero", SETFLAG, offsetof(struct mount_options,
	    post_4_2_opts) +
		offsetof(struct post_4_2_options, dio_szero),
		offsetof(struct mount_options, post_4_2_opts) +
		offsetof(struct post_4_2_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NODIO_SZERO, FAILED_MNT_NODIO_SZERO},


	/* sam option from 21 - 30 */
	{ "high", INT16,  offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, high),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_HIGH, FAILED_MNT_HIGH},
	{ "low", INT16,  offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, low),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_LOW, FAILED_MNT_LOW},
	{ "partial", MUL8,  offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, partial),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		SAM_DISK, SetFsParam,
		MNT_PARTIAL, FAILED_MNT_PARTIAL},
	{ "maxpartial", MUL8,   offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, maxpartial),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		SAM_DISK, SetFsParam,
		MNT_MAXPARTIAL, FAILED_MNT_MAXPARTIAL},
	{ "partial_stage", MUL8,  offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, partial_stage),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		SAM_DISK, SetFsParam,
		MNT_PARTIAL_STAGE, FAILED_MNT_PARTIAL_STAGE},
	{ "stage_n_window", MUL8,  offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, stage_n_window),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		SAM_DISK, SetFsParam,
		MNT_STAGE_N_WINDOW, FAILED_MNT_STAGE_N_WINDOW},
	{ "stage_retries", INT,  offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, stage_retries),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		SAM_DISK, SetFsParam,
		MNT_STAGE_RETRIES, FAILED_MNT_STAGE_RETRIES},
	{ "stage_flush_behind", MUL8, offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, stage_flush_behind),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		SAM_DISK, SetFsParam,
		MNT_STAGE_FLUSH_BEHIND, FAILED_MNT_STAGE_FLUSH_BEHIND},
	{ "hwm_archive", SETFLAG,  offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, hwm_archive),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		SAM_DISK, CmdSetFsConfig,
		MNT_HWM_ARCHIVE, FAILED_MNT_HWM_ARCHIVE},
	{ "nohwm_archive", SETFLAG,  offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, hwm_archive),
		offsetof(struct mount_options, sam_opts) +
		offsetof(struct sam_mount_options, change_flag),
		SAM_DISK, CmdSetFsConfig,
		MNT_NOHWM_ARCHIVE, FAILED_MNT_NOHWM_ARCHIVE},


	/* shared fs option from 31 - 38 */
	{ "minallocsz", INT64, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, minallocsz),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_MINALLOCSZ, FAILED_MNT_MINALLOCSZ},
	{ "maxallocsz", INT64, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, maxallocsz),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_MAXALLOCSZ, FAILED_MNT_MAXALLOCSZ},
	{ "rdlease", INT, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, rdlease),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_RDLEASE, FAILED_MNT_RDLEASE},
	{ "wrlease", INT, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, wrlease),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_WRLEASE, FAILED_MNT_WRLEASE},
	{ "aplease", INT, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, aplease),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_APLEASE, FAILED_MNT_APLEASE},
	{ "mh_write", SETFLAG, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, mh_write),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_MH_WRITE, FAILED_MNT_MH_WRITE},
	{ "nomh_write", SETFLAG, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, mh_write),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NOMH_WRITE, FAILED_MNT_NOMH_WRITE},
	{ "meta_timeo", INT, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, meta_timeo),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_META_TIMEO, FAILED_MNT_META_TIMEO},

	/* leased_timeo, added at rel 4.5 */
	{ "lease_timeo", INT, offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, lease_timeo),
		offsetof(struct mount_options, sharedfs_opts) +
		offsetof(struct sharedfs_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_LEASE_TIMEO, FAILED_MNT_LEASE_TIMEO},

	/* multireader fs option from 39 - 39 */
	{ "invalid", INT, offsetof(struct mount_options, multireader_opts) +
		offsetof(struct multireader_mount_options, invalid),
		offsetof(struct mount_options, multireader_opts) +
		offsetof(struct multireader_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_INVALID, FAILED_MNT_INVALID},

	/* refresh at eof: newly added at 4.5 */
	{ "refresh_at_eof", SETFLAG, offsetof(struct mount_options,
	    multireader_opts) +
		offsetof(struct multireader_mount_options, refresh_at_eof),
		offsetof(struct mount_options, multireader_opts) +
		offsetof(struct multireader_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_REFRESH_AT_EOF, FAILED_MNT_REFRESH_AT_EOF},

	{ "norefresh_at_eof", SETFLAG, offsetof(struct mount_options,
	    multireader_opts) +
		offsetof(struct multireader_mount_options, refresh_at_eof),
		offsetof(struct mount_options, multireader_opts) +
		offsetof(struct multireader_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NOREFRESH_AT_EOF, FAILED_MNT_NOREFRESH_AT_EOF},

	/* qfs fs option from 40 - 42 */
	{ "qwrite", SETFLAG, offsetof(struct mount_options, qfs_opts) +
		offsetof(struct qfs_mount_options, qwrite),
		offsetof(struct mount_options, qfs_opts) +
		offsetof(struct qfs_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_QWRITE, FAILED_MNT_QWRITE},
	{ "noqwrite", SETFLAG, offsetof(struct mount_options, qfs_opts) +
		offsetof(struct qfs_mount_options, qwrite),
		offsetof(struct mount_options, qfs_opts) +
		offsetof(struct qfs_mount_options, change_flag),
		QFS_STANDALONE, CmdSetFsConfig,
		MNT_NOQWRITE, FAILED_MNT_NOQWRITE},
	{ "mm_stripe", INT16, offsetof(struct mount_options, qfs_opts) +
		offsetof(struct qfs_mount_options, mm_stripe),
		offsetof(struct mount_options, qfs_opts) +
		offsetof(struct qfs_mount_options, change_flag),
		QFS_STANDALONE, SetFsParam,
		MNT_MM_STRIPE, FAILED_MNT_MM_STRIPE}


};


/*
 * This function changes mount options for a mounted filesystem.
 * Not all of the options are setable on a mounted filesystem. Any
 * which are not will be ignored.
 * It does check CLR flags to know if the user wants to reset
 * mount options to default.
 * failed_options is a list of type failed_mount_option_t.
 * If all mount options are set, success will be returned and
 * failed_options is a empty list.  If all mount options are not
 * set, error will be returned and failed_options includes all
 * failed options. If some mount options are not set, warning
 * will be returned and failed_options includes a list of failed
 * mount options.  All successful mount options will be recorded
 * in action log. All failed mount option will be recorded in
 * error log with detaild reasons.
 */
int
change_live_mount_options(
ctx_t *ctx,			/* ARGSUSED */
uname_t fsname,			/* name of fs for which to change options */
mount_options_t *options,	/* options to set */
sqm_lst_t **failed_options)	/* a list of failed_mount_option_t */
{
	int clear_flag = 0;
	fs_t *fs;
	mount_options_t *mo;
	int ret;
	boolean_t multi_r;
	sqm_lst_t *failed_list;

	Trace(TR_MISC,
	    "changeing live mount options for %s", fsname);
	if (ISNULL(fsname, options)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	if (ISNULL(failed_options)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	if (get_fs(NULL, fsname, &fs) != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	if (fs->mount_options->multireader_opts.reader == B_TRUE ||
	    fs->mount_options->multireader_opts.writer == B_TRUE) {
		multi_r = B_TRUE;
	} else {
		multi_r = B_FALSE;
	}

	if ((fs->striped_group_list)->length == 0) {
		if (get_default_mount_options(ctx, fs->equ_type, (int)fs->dau,
		    B_FALSE, fs->mount_options->sharedfs_opts.shared,
		    multi_r, &mo) != 0) {
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
	} else {
		if (get_default_mount_options(ctx, fs->equ_type, (int)fs->dau,
		    B_TRUE, fs->mount_options->sharedfs_opts.shared,
		    multi_r, &mo) != 0) {
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
	}

	if ((options->change_flag & CLR_SYNC_META) == CLR_SYNC_META) {
		clear_flag ++;
		options->sync_meta = mo->sync_meta;
		options->change_flag = options->change_flag | MNT_SYNC_META;
	}

	if ((options->change_flag & CLR_NOSUID) == CLR_NOSUID) {
		clear_flag ++;
		options->no_suid = mo->no_suid;
		options->change_flag = options->change_flag | MNT_NOSUID;
	}

	if ((options->change_flag & CLR_SUID) == CLR_SUID) {
		clear_flag ++;
		options->no_suid = mo->no_suid;
		options->change_flag = options->change_flag | MNT_SUID;
	}

	if ((options->change_flag & CLR_STRIPE) == CLR_STRIPE) {
		clear_flag ++;
		options->stripe = mo->stripe;
		options->change_flag = options->change_flag | MNT_STRIPE;
	}

	if ((options->change_flag & CLR_TRACE)
	    == CLR_TRACE) {
		clear_flag ++;
		options->trace = mo->trace;
		options->change_flag = options->change_flag | MNT_TRACE;
	}

	if ((options->change_flag & CLR_NOTRACE) == CLR_NOTRACE) {
		clear_flag ++;
		options->trace = mo->trace;
		options->change_flag = options->change_flag | MNT_NOTRACE;
	}

	if ((options->io_opts.change_flag & CLR_DIO_RD_CONSEC) ==
	    CLR_DIO_RD_CONSEC) {
		clear_flag ++;
		options->io_opts.dio_rd_consec = mo->io_opts.dio_rd_consec;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_DIO_RD_CONSEC;
	}
	if ((options->io_opts.change_flag & CLR_DIO_RD_FORM_MIN) ==
	    CLR_DIO_RD_FORM_MIN) {
		clear_flag ++;
		options->io_opts.dio_rd_form_min = mo->io_opts.dio_rd_form_min;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_DIO_RD_FORM_MIN;
	}
	if ((options->io_opts.change_flag & CLR_DIO_RD_ILL_MIN) ==
	    CLR_DIO_RD_ILL_MIN) {
		clear_flag ++;
		options->io_opts.dio_rd_ill_min = mo->io_opts.dio_rd_ill_min;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_DIO_RD_ILL_MIN;
	}
	if ((options->io_opts.change_flag & CLR_DIO_WR_CONSEC) ==
	    CLR_DIO_WR_CONSEC) {
		clear_flag ++;
		options->io_opts.dio_wr_consec = mo->io_opts.dio_wr_consec;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_DIO_WR_CONSEC;
	}
	if ((options->io_opts.change_flag & CLR_DIO_WR_FORM_MIN)
	    == CLR_DIO_WR_FORM_MIN) {
		clear_flag ++;
		options->io_opts.dio_wr_form_min = mo->io_opts.dio_wr_form_min;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_DIO_WR_FORM_MIN;
	}

	if ((options->io_opts.change_flag & CLR_DIO_WR_ILL_MIN)
	    == CLR_DIO_WR_ILL_MIN) {
		clear_flag ++;
		options->io_opts.dio_wr_ill_min = mo->io_opts.dio_wr_ill_min;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_DIO_WR_ILL_MIN;
	}

	if ((options->io_opts.change_flag & CLR_FORCEDIRECTIO)
	    == CLR_FORCEDIRECTIO) {
		clear_flag ++;
		options->io_opts.forcedirectio = mo->io_opts.forcedirectio;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_FORCEDIRECTIO;
	}

	if ((options->io_opts.change_flag & CLR_NOFORCEDIRECTIO)
	    == CLR_NOFORCEDIRECTIO) {
		clear_flag ++;
		options->io_opts.forcedirectio = mo->io_opts.forcedirectio;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_NOFORCEDIRECTIO;
	}

	if ((options->io_opts.change_flag & CLR_SW_RAID)
	    == CLR_SW_RAID) {
		clear_flag ++;
		options->io_opts.sw_raid = mo->io_opts.sw_raid;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_SW_RAID;
	}

	if ((options->io_opts.change_flag & CLR_NOSW_RAID)
	    == CLR_NOSW_RAID) {
		clear_flag ++;
		options->io_opts.sw_raid = mo->io_opts.sw_raid;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_NOSW_RAID;
	}

	if ((options->io_opts.change_flag & CLR_FLUSH_BEHIND)
	    == CLR_FLUSH_BEHIND) {
		clear_flag ++;
		options->io_opts.flush_behind = mo->io_opts.flush_behind;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_FLUSH_BEHIND;
	}

	if ((options->io_opts.change_flag & CLR_READAHEAD)
	    == CLR_READAHEAD) {
		clear_flag ++;
		options->io_opts.readahead = mo->io_opts.readahead;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_READAHEAD;
	}

	if ((options->io_opts.change_flag & CLR_WRITEBEHIND)
	    == CLR_WRITEBEHIND) {
		clear_flag ++;
		options->io_opts.writebehind = mo->io_opts.writebehind;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_WRITEBEHIND;
	}

	if ((options->io_opts.change_flag & CLR_WR_THROTTLE)
	    == CLR_WR_THROTTLE) {
		clear_flag ++;
		options->io_opts.wr_throttle = mo->io_opts.wr_throttle;
		options->io_opts.change_flag =
		    options->io_opts.change_flag | MNT_WR_THROTTLE;
	}

	/* new options after rel 4.2, checked in at 4.5 */
	if ((options->post_4_2_opts.change_flag & CLR_ABR)
	    == CLR_ABR) {
		clear_flag ++;
		options->post_4_2_opts.abr = mo->post_4_2_opts.abr;
		options->post_4_2_opts.change_flag =
		    options->post_4_2_opts.change_flag | MNT_ABR;
	}

	if ((options->post_4_2_opts.change_flag & CLR_NOABR)
	    == CLR_NOABR) {
		clear_flag ++;
		options->post_4_2_opts.abr = mo->post_4_2_opts.abr;
		options->post_4_2_opts.change_flag =
		    options->post_4_2_opts.change_flag | MNT_NOABR;
	}

	if ((options->post_4_2_opts.change_flag & CLR_DMR)
	    == CLR_DMR) {
		clear_flag ++;
		options->post_4_2_opts.dmr = mo->post_4_2_opts.dmr;
		options->post_4_2_opts.change_flag =
		    options->post_4_2_opts.change_flag | MNT_DMR;
	}

	if ((options->post_4_2_opts.change_flag & CLR_NODMR)
	    == CLR_NODMR) {
		clear_flag ++;
		options->post_4_2_opts.dmr = mo->post_4_2_opts.dmr;
		options->post_4_2_opts.change_flag =
		    options->post_4_2_opts.change_flag | MNT_NODMR;
	}


	if ((options->post_4_2_opts.change_flag & CLR_DIO_SZERO)
	    == CLR_DIO_SZERO) {
		clear_flag ++;
		options->post_4_2_opts.dio_szero =
		    mo->post_4_2_opts.dio_szero;
		options->post_4_2_opts.change_flag =
		    options->post_4_2_opts.change_flag | MNT_DIO_SZERO;
	}

	if ((options->post_4_2_opts.change_flag & CLR_NODIO_SZERO)
	    == CLR_NODIO_SZERO) {
		clear_flag ++;
		options->post_4_2_opts.dio_szero =
		    mo->post_4_2_opts.dio_szero;
		options->post_4_2_opts.change_flag =
		    options->post_4_2_opts.change_flag | MNT_NODIO_SZERO;
	}

	if ((options->sam_opts.change_flag & CLR_HIGH) == CLR_HIGH) {
		clear_flag ++;
		options->sam_opts.high = mo->sam_opts.high;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_HIGH;
	}

	if ((options->sam_opts.change_flag & CLR_LOW) == CLR_LOW) {
		clear_flag ++;
		options->sam_opts.low = mo->sam_opts.low;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_LOW;
	}

	if ((options->sam_opts.change_flag & CLR_PARTIAL)
	    == CLR_PARTIAL) {
		clear_flag ++;
		options->sam_opts.partial = mo->sam_opts.partial;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_PARTIAL;
	}

	if ((options->sam_opts.change_flag & CLR_MAXPARTIAL)
	    == CLR_MAXPARTIAL) {
		clear_flag ++;
		options->sam_opts.maxpartial = mo->sam_opts.maxpartial;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_MAXPARTIAL;
	}

	if ((options->sam_opts.change_flag & CLR_PARTIAL_STAGE)
	    == CLR_PARTIAL_STAGE) {
		clear_flag ++;
		options->sam_opts.partial_stage = mo->sam_opts.partial_stage;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_PARTIAL_STAGE;
	}

	if ((options->sam_opts.change_flag & CLR_STAGE_N_WINDOW)
	    == CLR_STAGE_N_WINDOW) {
		clear_flag ++;
		options->sam_opts.stage_n_window = mo->sam_opts.stage_n_window;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_STAGE_N_WINDOW;
	}

	if ((options->sam_opts.change_flag & CLR_STAGE_RETRIES)
	    == CLR_STAGE_RETRIES) {
		clear_flag ++;
		options->sam_opts.stage_retries = mo->sam_opts.stage_retries;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_STAGE_RETRIES;
	}

	if ((options->sam_opts.change_flag & CLR_STAGE_FLUSH_BEHIND)
	    == CLR_STAGE_FLUSH_BEHIND) {
		clear_flag ++;
		options->sam_opts.stage_flush_behind =
		    mo->sam_opts.stage_flush_behind;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_STAGE_FLUSH_BEHIND;
	}

	if ((options->sam_opts.change_flag & CLR_HWM_ARCHIVE)
	    == CLR_HWM_ARCHIVE) {
		clear_flag ++;
		options->sam_opts.hwm_archive = mo->sam_opts.hwm_archive;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_HWM_ARCHIVE;
	}
	if ((options->sam_opts.change_flag & CLR_NOHWM_ARCHIVE)
	    == CLR_NOHWM_ARCHIVE) {
		clear_flag ++;
		options->sam_opts.hwm_archive = mo->sam_opts.hwm_archive;
		options->sam_opts.change_flag =
		    options->sam_opts.change_flag | MNT_NOHWM_ARCHIVE;
	}

	if ((options->sharedfs_opts.change_flag & CLR_MINALLOCSZ)
	    == CLR_MINALLOCSZ) {
		clear_flag ++;
		options->sharedfs_opts.minallocsz =
		    mo->sharedfs_opts.minallocsz;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_MINALLOCSZ;
	}

	if ((options->sharedfs_opts.change_flag & CLR_MAXALLOCSZ)
	    == CLR_MAXALLOCSZ) {
		clear_flag ++;
		options->sharedfs_opts.maxallocsz =
		    mo->sharedfs_opts.maxallocsz;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_MAXALLOCSZ;
	}

	if ((options->sharedfs_opts.change_flag & CLR_RDLEASE)
	    == CLR_RDLEASE) {
		clear_flag ++;
		options->sharedfs_opts.rdlease = mo->sharedfs_opts.rdlease;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_RDLEASE;
	}

	if ((options->sharedfs_opts.change_flag & CLR_WRLEASE)
	    == CLR_WRLEASE) {
		clear_flag ++;
		options->sharedfs_opts.wrlease = mo->sharedfs_opts.wrlease;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_WRLEASE;
	}

	if ((options->sharedfs_opts.change_flag & CLR_APLEASE)
	    == CLR_APLEASE) {
		clear_flag ++;
		options->sharedfs_opts.aplease = mo->sharedfs_opts.aplease;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_APLEASE;
	}

	if ((options->sharedfs_opts.change_flag & CLR_MH_WRITE)
	    == CLR_MH_WRITE) {
		clear_flag ++;
		options->sharedfs_opts.mh_write = mo->sharedfs_opts.mh_write;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_MH_WRITE;
	}

	if ((options->sharedfs_opts.change_flag & CLR_NOMH_WRITE)
	    == CLR_NOMH_WRITE) {
		clear_flag ++;
		options->sharedfs_opts.mh_write = mo->sharedfs_opts.mh_write;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_NOMH_WRITE;
	}

	if ((options->sharedfs_opts.change_flag & CLR_META_TIMEO)
	    == CLR_META_TIMEO) {
		clear_flag ++;
		options->sharedfs_opts.meta_timeo =
		    mo->sharedfs_opts.meta_timeo;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_META_TIMEO;
	}

	/*
	 *	lease_timeo will be checked in at rel 4.5.
	 */
	if ((options->sharedfs_opts.change_flag & CLR_LEASE_TIMEO)
	    == CLR_LEASE_TIMEO) {
		clear_flag ++;
		options->sharedfs_opts.lease_timeo =
		    mo->sharedfs_opts.lease_timeo;
		options->sharedfs_opts.change_flag =
		    options->sharedfs_opts.change_flag | MNT_LEASE_TIMEO;
	}

	if ((options->multireader_opts.change_flag & CLR_INVALID)
	    == CLR_INVALID) {
		clear_flag ++;
		options->multireader_opts.invalid =
		    mo->multireader_opts.invalid;
		options->multireader_opts.change_flag =
		    options->multireader_opts.change_flag | MNT_INVALID;
	}

	if ((options->multireader_opts.change_flag & CLR_REFRESH_AT_EOF)
	    == CLR_REFRESH_AT_EOF) {
		clear_flag ++;
		options->multireader_opts.refresh_at_eof =
		    mo->multireader_opts.refresh_at_eof;
		options->multireader_opts.change_flag =
		    options->multireader_opts.change_flag | MNT_REFRESH_AT_EOF;
	}

	if ((options->multireader_opts.change_flag & CLR_NOREFRESH_AT_EOF)
	    == CLR_NOREFRESH_AT_EOF) {
		clear_flag ++;
		options->multireader_opts.refresh_at_eof =
		    mo->multireader_opts.refresh_at_eof;
		options->multireader_opts.change_flag =
		    options->multireader_opts.change_flag
		    | MNT_NOREFRESH_AT_EOF;
	}

	if ((options->qfs_opts.change_flag & CLR_QWRITE)
	    == CLR_QWRITE) {
		clear_flag ++;
		options->qfs_opts.qwrite = mo->qfs_opts.qwrite;
		options->qfs_opts.change_flag =
		    options->qfs_opts.change_flag | MNT_QWRITE;
	}

	if ((options->qfs_opts.change_flag & CLR_NOQWRITE)
	    == CLR_NOQWRITE) {
		clear_flag ++;
		options->qfs_opts.qwrite = mo->qfs_opts.qwrite;
		options->qfs_opts.change_flag =
		    options->qfs_opts.change_flag | MNT_NOQWRITE;
	}

	if ((options->qfs_opts.change_flag & CLR_MM_STRIPE)
	    == CLR_MM_STRIPE) {
		clear_flag ++;
		options->qfs_opts.mm_stripe = mo->qfs_opts.mm_stripe;
		options->qfs_opts.change_flag =
		    options->qfs_opts.change_flag | MNT_MM_STRIPE;
	}

	ret = set_live_mount_options(ctx, fsname, options,
	    &failed_list);
	*failed_options = failed_list;

	if (fs) {
		free_fs(fs);
	}
	if (mo) {
		free(mo);
	}
	Trace(TR_MISC,
	    "finished changeing live mount options for %s", fsname);
	return (ret);
}


/*
 * This function changes mount options for a mounted filesystem.
 * Not all of the options are setable on a mounted filesystem. Any
 * which are not will be ignored.
 * failed_options is a list of type failed_mount_option_t.
 * If all mount options are set, success will be returned and
 * failed_options is a empty list.  If all mount options are not
 * set, error will be returned and failed_options includes all
 * failed options. If some mount options are not set, warning
 * will be returned and failed_options includes a list of failed
 * mount options.  All successful mount options will be recorded
 * in action log. All failed mount option will be recorded in
 * error log with detaild reasons.
 */
static int
set_live_mount_options(
ctx_t *ctx,			/* ARGSUSED */
uname_t fsname,			/* name of fs for which to change options */
mount_options_t *options,	/* options to set */
sqm_lst_t **failed_options)	/* a list of failed_mount_option_t */
{
	char *msg;
	upath_t value;
	failed_mount_option_t *failed_option;
	int total_flag = 0;
	int error_flag = 0;
	int cmd_n;
	uint32_t *fldDef;
	void *v;

	Trace(TR_MISC, "setting fs mount options live for %s", fsname);
	if (ISNULL(fsname, options)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	if (ISNULL(failed_options)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	*failed_options = lst_create();
	if (*failed_options == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	for (cmd_n = 1; cmd_n < numof(cmd_t); cmd_n++) {
		if (*cmd_t[cmd_n].name == '\0')  continue;
		fldDef = (uint32_t *)(void *)((char *)options +
		    cmd_t[cmd_n].FvFlagLoc);
		if (*fldDef & cmd_t[cmd_n].mount_option) {
			total_flag ++;
			v = ((char *)options + cmd_t[cmd_n].FvLoc);
			switch (cmd_t[cmd_n].FvType) {
				case SETFLAG:
					msg = cmd_t[cmd_n].cmd(fsname,
					    cmd_t[cmd_n].name, NULL);
					break;
				case PWR2:
				case MUL8:
				case INT:
					snprintf(value,
					    sizeof (value),
					    "%d",
					    *(int *)v);
					msg = cmd_t[cmd_n].cmd(fsname,
					    cmd_t[cmd_n].name, value);
					break;
				case INT16:
					snprintf(value,
					    sizeof (value),
					    "%d",
					    *(int16_t *)v);
					msg = cmd_t[cmd_n].cmd(fsname,
					    cmd_t[cmd_n].name, value);
					break;
				case MULL8:
				case INT64:
					snprintf(value,
					    sizeof (value),
					    "%llu",
					    *(int64_t *)v);
					msg = cmd_t[cmd_n].cmd(fsname,
					    cmd_t[cmd_n].name, value);
					break;
				default:
					samerrno = SE_NOT_DEFINED_MNT_OPTION;
					snprintf(samerrmsg, MAX_MSG_LEN,
					    GetCustMsg(samerrno),
					    cmd_t[cmd_n].mount_option);
					Trace(TR_ERR, "%s", samerrmsg);
					return (-1);
			}
			if (*msg != '\0') {
				error_flag ++;
				Trace(TR_CUST,
				    "mount option %s is not set, error: %s\n",
				    cmd_t[cmd_n].name, msg);
				failed_option = (failed_mount_option_t *)
				    mallocer(sizeof (failed_mount_option_t));
				if (failed_option == NULL) {
					free_list_of_fs(*failed_options);
					*failed_options = NULL;
					Trace(TR_ERR, "%s", samerrmsg);
					return (-1);
				}
				*failed_option =
				    cmd_t[cmd_n].failed_mount_option;
				if (lst_append(*failed_options, failed_option)
				    != 0) {
					free_list_of_fs(*failed_options);
					*failed_options = NULL;
					free(failed_option);
					Trace(TR_ERR, "%s", samerrmsg);
					return (-1);
				}
			} else {
				Trace(TR_CUST,
				    "mount option %s is set\n",
				    cmd_t[cmd_n].name);
			}
		}
	}

	if (error_flag < total_flag && error_flag != 0) {
		samerrno = SE_SET_LIVE_MOUNT_OPTIONS_PARTIAL_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_ERR, "%s", samerrmsg);
		return (-2);
	} else if (error_flag == total_flag && error_flag != 0) {
		samerrno = SE_SET_LIVE_MOUNT_OPTIONS_ALL_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "finished setting fs mount options live for %s",
	    fsname);
	return (0);
}


static char *
CmdSetFsConfig(
upath_t fsname,		/* file system name */
upath_t option_name,	/* fs parameter name */
upath_t option_value	/* ARGSUSED */
)
{
	char    *msg;

	msg = SetFsConfig(fsname, option_name);
	return (msg);
}
