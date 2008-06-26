/*
 *  dis_mount.c - Display mount table.
 *
 * Display the contents of the mount table.
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

#pragma ident "$Revision: 1.67 $"


/* ANSI headers. */
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <unistd.h>

/* Solaris headers. */
#include <sys/mount.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/devnm.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/syscall.h"

/* Local headers. */
#include "samu.h"

/* Private data */
static int numfs;			/* Number of filesystems */
static struct sam_fs_status *fsarray;	/* Status of configured filesystems */
static struct sam_fs_status *fs;	/* Status of filesystem displayed */
static int details = 0;	/* Detailed display: 1 = show flag names */
static int fs_num;	/* Filesystem displayed */
static int first_part;
static int num_parts;
static int part_line;


/*
 *	bit definitions - see include/sam/mount.h
 */
sam_flagtext_t confTable[] = {
	MT_NOATIME, "NOATIME",
	MT_CDEVID, "CDEVID",
	MT_EMUL_LITE, "EMUL_LITE",
	MT_WORM_EMUL, "WORM_EMUL",
	MT_WORM_LITE, "WORM_LITE",
	MT_CONSISTENT_ATTR, "CONSISTENT_ATTR",
	MT_ZERO_DIO_SPARSE, "ZERO_DIO_SPARSE",
	MT_DMR_DATA, "DMR_DATA",
	MT_ABR_DATA, "ABR_DATA",
	MT_ARCHIVE_SCAN, "ARCHIVE_SCAN",
	MT_REFRESH_EOF, "REFRESH_EOF",
	MT_SHARED_BG, "SHARED_BG",
	MT_SHARED_SOFT, "SHARED_SOFT",
	MT_HWM_ARCHIVE, "HWM_ARCHIVE",
	MT_GFSID, "GFSID",
	MT_QUOTA, "QUOTA",
	MT_OLD_ARCHIVE_FMT, "OLD_ARCHIVE_FMT",
	MT_NFSASYNC, "NFS_ASYNC",
	MT_SYNC_META, "SYNC_META",
	MT_WORM, "WORM",
	MT_SHARED_READER, "SHARED_READER",
	MT_SHARED_WRITER, "SHARED_WRITER",
	MT_SOFTWARE_RAID, "SOFTWARE_RAID",
	MT_DIRECTIO, "DIRECTIO",
	MT_QWRITE, "QWRITE",
	MT_TRACE, "TRACE",
	MT_SAM_ENABLED, "SAM_ENABLED",
	MT_MH_WRITE, "MH_WRITE",
	MT_SHARED_MO, "SHARED_MO"
};
int	confTableSize = sizeof (confTable)/sizeof (sam_flagtext_t);

sam_flagtext_t conf1Table[] = {
	MC_SAM_DB, "SAM_DB",
	MC_OBJECT_FS, "OBJECT_FS",
	MC_CLUSTER_FASTSW, "CLUSTER_FASTSW",
	MC_CLUSTER_MGMT, "CLUSTER_MGMT",
	MC_STRIPE_GROUPS, "STRIPE_GROUPS",
	MC_MD_DEVICES, "MD_DEVICES",
	MC_MR_DEVICES, "MR_DEVICES",
	MC_SMALL_DAUS, "SMALL_DAUS",
	MC_MISMATCHED_GROUPS, "MISMATCHED_GROUPS",
	MC_SHARED_MOUNTING, "SHARED_MOUNTING",
	MC_SHARED_FS, "SHARED_FS",
	MC_LOGGING, "LOGGING"
};
int	conf1TableSize = sizeof (conf1Table)/sizeof (sam_flagtext_t);

/*
 *	bit definitions - see /usr/include/sys/mount.h
 */
static sam_flagtext_t mflagTable[] = {
	MS_RDONLY,	"RDONLY",	/* Read-only */
	MS_FSS,		"FSS",		/* Old (4-argument) */
					/* mount (compatibility) */
	MS_DATA,	"DATA",		/* 6-argument mount */
	MS_NOSUID,	"NOSUID",	/* Setuid programs disallowed */
	MS_REMOUNT,	"REMOUNT",	/* Remount */
	MS_NOTRUNC,	"NOTRUNC",	/* Return ENAMETOOLONG */
					/* for long filenames */
	MS_OVERLAY,	"OVERLAY",	/* Allow overlay mounts */
	MS_OPTIONSTR,	"OPTIONSTR",	/* Data is a an in/out option string */
	MS_GLOBAL,	"GLOBAL",	/* Clustering: */
					/* Mount into global name space */
};
int	mflagTableSize = sizeof (mflagTable)/sizeof (sam_flagtext_t);

/*
 *	bit definitions - see include/sam/mount.h
 */
sam_flagtext_t statusTable[] = {
/* N.B. Bad indentation to meet cstyle requirements */
FS_MOUNTED,		"MOUNTED",	/* Filesystem is mounted */
FS_MOUNTING,		"MOUNTING",	/* Filesystem is currently mounting */
FS_UMOUNT_IN_PROGRESS,	"UMOUNT_IN_PROGRESS",
					/* Filesystem is currently umounting */
FS_SERVER,		"SERVER",	/* Host is now metadata server */
FS_CLIENT,		"CLIENT",	/* Host is not metadata server */
FS_NODEVS,		"NODEVS",	/* Host can't be metadata server */
FS_SAM,			"SAM",		/* Metadata server is running SAM */
FS_LOCK_WRITE,		"LOCK_WRITE",	/* Lock write operations */
FS_LOCK_NAME,		"LOCK_NAME",	/* Lock name operations */
FS_LOCK_RM_NAME,	"LOCK_RM_NAME", /* Lock remove name operations */
FS_LOCK_HARD,		"LOCK_HARD",	/* Lock all operations */
FS_SRVR_DOWN,		"SRVR_DOWN",	/* Server is not responding */
FS_SRVR_BYTEREV,	"SRVR_BYTEREV",	/* Server has rev byte ordering */
FS_SRVR_DONE,		"SRVR_DONE",	/* Server finished failover */
FS_CLNT_DONE,		"CLNT_DONE",	/* Client finished resetting leases */
FS_FREEZING,		"FREEZING",	/* Host is failing over */
FS_FROZEN,		"FROZEN",	/* Host is frozen */
FS_THAWING,		"THAWING",	/* Host is thawing */
FS_RESYNCING,		"RESYNCING",	/* Server is resyncing */
FS_RELEASING,		"RELEASING",	/* releasing is active on this fs */
FS_STAGING,		"STAGING",	/* staging is active on this fs */
FS_ARCHIVING,		"ARCHIVING",	/* archiving is active on this fs */
};
int statusTableSize = sizeof (statusTable)/sizeof (sam_flagtext_t);

/* Private functions */
static void DisFsParts(struct sam_fs_info *fi);


/* Public functions */
void PrintFlags(uint32_t field, sam_flagtext_t *table, int ntable, char *label);

/*
 * Display initialization.
 */
boolean
InitMount(
void)
{
	if (FsInitialized != 0) {
		return (TRUE);
	}
	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		Error("GetFsStatus failed");
		/*NOTREACHED*/
	}

	fs_num = 0;
	fs = fsarray + fs_num;
	File_System = fs->fs_name;
	first_part = 0;
	num_parts = 100;

	if (Argc > 1) {
		File_System = Argv[1];
	}
	FsInitialized = 1;
	return (TRUE);
}


void
DisMount()
{
	struct sam_fs_info fi;
	int		col;
	int		next_line;
	char	sbuf[20];

	if (!FsInitialized) {
		(void) InitMount();
	}
	if (GetFsInfo(File_System, &fi) == -1) {
		Error("Filesystem %s not configured", File_System);
		/*NOTREACHED*/
	}

	ln = 2;
	col = 0;
	Mvprintw(ln++, col, "mount_point    : %s", fi.fi_mnt_point);
	Mvprintw(ln++, col, "server         : %s", fi.fi_server);
	Mvprintw(ln++, col, "filesystem name: %s", fi.fi_name);
	Mvprintw(ln++, col, "eq  type       : %d %s",
	    fi.fi_eq, device_to_nm(fi.fi_type));
	Mvprintw(ln++, col, "state version  : %4d %4d",
	    fi.fi_state, fi.fi_version);
	Mvprintw(ln++, col, "(fs,mm)_count  : %4d %4d",
	    fi.fs_count, fi.mm_count);
	snprintf(sbuf, 20, "%d", fi.fi_sync_meta);
	Mvprintw(ln++, col, "sync_meta      : %s",
	    (fi.fi_sync_meta < 0) ? "default" : sbuf);
	snprintf(sbuf, 20, "%d", fi.fi_atime);
	Mvprintw(ln++, col, "atime          : %s",
	    (fi.fi_atime == 0) ? "default" : sbuf);
	snprintf(sbuf, 20, "%d", fi.fi_stripe[0]);
	Mvprintw(ln++, col, "stripe         : %s",
	    (fi.fi_stripe[0] < 0) ? "default" : sbuf);
	Mvprintw(ln++, col, "mm_stripe      : %d", fi.fi_stripe[1]);
	Mvprintw(ln++, col, "high  low      : %3d%% %3d%%",
	    fi.fi_high, fi.fi_low);
	Mvprintw(ln++, col, "readahead      : %lld", fi.fi_readahead);
	Mvprintw(ln++, col, "writebehind    : %lld", fi.fi_writebehind);
	Mvprintw(ln++, col, "wr_throttle    : %lld", fi.fi_wr_throttle);
	Mvprintw(ln++, col, "rd_ino_buf_size: %d", fi.fi_rd_ino_buf_size);
	Mvprintw(ln++, col, "wr_ino_buf_size: %d", fi.fi_wr_ino_buf_size);
	next_line = ln;

	/* Filesystem parameters in second column */

	ln = 2;
	col = 40;
	Mvprintw(ln++, col, "partial        : %dk", fi.fi_partial);
	Mvprintw(ln++, col, "maxpartial     : %dk", fi.fi_maxpartial);
	Mvprintw(ln++, col, "partial_stage  : %u", fi.fi_partial_stage);
	Mvprintw(ln++, col, "flush_behind   : %d", fi.fi_flush_behind);
	Mvprintw(ln++, col, "stage_flush_beh: %d", fi.fi_stage_flush_behind);
	Mvprintw(ln++, col, "stage_n_window : %u", fi.fi_stage_n_window);
	Mvprintw(ln++, col, "stage_retries  : %d", fi.fi_stage_retries);
	Mvprintw(ln++, col, "stage timeout  : %d", fi.fi_timeout);
	Mvprintw(ln++, col, "dio_consec r,w : %4d %4d",
	    fi.fi_dio_rd_consec, fi.fi_dio_wr_consec);
	Mvprintw(ln++, col, "dio_frm_min r,w: %4d %4d",
	    fi.fi_dio_rd_form_min, fi.fi_dio_wr_form_min);
	Mvprintw(ln++, col, "dio_ill_min r,w: %4d %4d",
	    fi.fi_dio_rd_ill_min, fi.fi_dio_wr_ill_min);
	Mvprintw(ln++, col, "ext_bsize      : %ld", fi.fi_ext_bsize);
	Mvprintw(ln++, col, "def_retention  : %d", fi.fi_def_retention);
	if (fi.fi_config1 & MC_SHARED_FS) {
		snprintf(sbuf, 20, "%lld", fi.fi_minallocsz);
		Mvprintw(ln++, col, "minallocsz     : %s",
		    (fi.fi_minallocsz < 0LL) ? "default" : sbuf);
		snprintf(sbuf, 20, "%lld", fi.fi_maxallocsz);
		Mvprintw(ln++, col, "maxallocsz     : %s",
		    (fi.fi_maxallocsz < 0LL) ? "default" : sbuf);
		Mvprintw(ln++, col, "min_pool       : %d", fi.fi_min_pool);
		Mvprintw(ln++, col, "meta_timeo     : %d", fi.fi_meta_timeo);
		Mvprintw(ln++, col, "lease_timeo    : %d", fi.fi_lease_timeo);
		Mvprintw(ln++, col, "(rd,wr,ap)lease: %4d %4d %4d",
		    fi.fi_lease[RD_LEASE], fi.fi_lease[WR_LEASE],
		    fi.fi_lease[AP_LEASE]);
	}
	if (fi.fi_config & (MT_SHARED_WRITER|MT_SHARED_READER)) {
		Mvprintw(ln++, col, "invalid        : %d", fi.fi_invalid);
	}
	if (ln < next_line) {
		ln = next_line;
	}
	Mvprintw(ln,    0, "config         : 0x%08x  ", fi.fi_config);
	Mvprintw(ln++, 40, "config1        : 0x%08x", fi.fi_config1);
	Mvprintw(ln,  0,   "status         : 0x%08x  ", fi.fi_status);
	Mvprintw(ln++, 40, "mflag          : 0x%08x", fi.fi_mflag);
	if (details == 0) {
		if (fi.fi_config & MT_WORM) {
			Mvprintw(ln, 40, "               : WORM mode");
		} else if (fi.fi_config & MT_WORM_LITE) {
			Mvprintw(ln, 40, "               : WORM Lite mode");
		} else if (fi.fi_config & MT_WORM_EMUL) {
			Mvprintw(ln, 40, "               : "
			"WORM Emulation mode");
		} else if (fi.fi_config & MT_EMUL_LITE) {
			Mvprintw(ln, 40, "               : "
			"WORM Emulation Lite mode");
		}
	}
	ln++;
	if (details == 1 || ScreenMode == FALSE) {
		PrintFlags(fi.fi_config, confTable, confTableSize, "config");
		PrintFlags(fi.fi_config1, conf1Table, conf1TableSize,
		    "config1");
		PrintFlags(fi.fi_mflag, mflagTable, mflagTableSize, "mflag");
		PrintFlags(fi.fi_status, statusTable, statusTableSize,
		    "status");
	}
	if (details == 0 || ScreenMode == FALSE) {
		ln++;
		DisFsParts(&fi);
	}
}

void
PrintFlags(
uint32_t field,
sam_flagtext_t *table,
int ntable,
char *label)
{
	int i, j;
	sam_flagtext_t *tabent;

	j = 0;
	Mvprintw(ln++, 6, "%-8s :  ", label);
	for (i = 0; i < ntable; i++) {
		tabent = table + i;
		if (field & tabent->flagmask) {
			if ((j != 0) && (j % 4 == 0)) {
				Mvprintw(ln++, 6, "\"        :  ");
			}
			j++;
			Printw("%s\t", tabent->flagtext);
		}
	}
}

/*
 * Display configuration - device display.
 */
static void
DisFsParts(
struct sam_fs_info *fi)
{
	struct sam_fs_part *ptarray;
	int		i;

	/* "Filesystem configuration:" */
	Mvprintw(ln++, 0, GetCustMsg(868));
	/* "ty   eq state   device_name\t\t\t   fs family_set" */
	Mvprintw(ln++, 0, GetCustMsg(7301));
	part_line = ln;

	/*
	 * Make partition array and get the partitions data.
	 */
	ptarray = (struct sam_fs_part *)malloc(
	    fi->fs_count * sizeof (struct sam_fs_part));
	if (ptarray == NULL) {
		Error("no memory for partitions");
		/*NOTREACHED*/
	}
	if (GetFsParts(fi->fi_name, fi->fs_count, ptarray) == -1) {
		free(ptarray);
		Error("GetFsParts(%s) failed.", fi->fi_name);
		/*NOTREACHED*/
	}
	num_parts = fi->fs_count;
	for (i = first_part; i < num_parts; i++) {
		struct sam_fs_part *pt;

		if (ln > LINES - 3) {
			Mvprintw(ln++, 0, GetCustMsg(2)); /* "     more" */
			break;
		}
		pt = ptarray + i;

		Mvprintw(ln++, 0, "%2s%5d %-7s %-32s",
		    device_to_nm(pt->pt_type),
		    pt->pt_eq,
		    dev_state[pt->pt_state],
		    pt->pt_name);
		Printw("%5d", fi->fi_eq);
		Printw(" %s", fi->fi_name);
		Clrtoeol();
	}
	free(ptarray);
}

/*
 * Keyboard processing.
 */


boolean
KeyMount(
char key)
{
#define	DLINES (LINES - part_line - 3)

	switch (key) {

	case KEY_full_fwd:
		fs_num++;
		if (fs_num >= numfs) {
			fs_num = 0;
		}
		fs = fsarray + fs_num;
		File_System = fs->fs_name;
		first_part = 0;
		num_parts = 0;
		break;

	case KEY_full_bwd:
		fs_num--;
		if (fs_num < 0) {
			fs_num = numfs - 1;
		}
		fs = fsarray + fs_num;
		File_System = fs->fs_name;
		first_part = 0;
		num_parts = 0;
		break;

	case KEY_half_fwd:
		if (first_part + DLINES < num_parts) {
			first_part += DLINES;
		}
		break;

	case KEY_half_bwd:
		if (first_part - DLINES > 0) {
			first_part -= DLINES;
		} else {
			first_part = 0;
		}
		break;

	case KEY_details:
		if (++details > 1) details = 0;
		break;

	default:
		return (FALSE);
	}
	return (TRUE);
}
