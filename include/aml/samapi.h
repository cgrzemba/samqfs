/*
 * samapi.h - SAM-FS API library functions.
 *
 * Definitions for SAM-FS API library functions.
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

#if !defined(_AML_SAMAPI_H)
#define	_AML_SAMAPI_H

#pragma ident "$Revision: 1.17 $"

#include <sys/types.h>
#include "pub/devstat.h"
#include "aml/types.h"
#include "sam/defaults.h"
#include "aml/catalog.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Defintions for sam_fsdata() API
 */

/*
 * Default values entry structure for sam_settings() API
 */

struct sam_def_values {
	/*
	 * Fields as defined in structure sam_defaults_t
	 */
	media_t		optical;	/* Default optical disk type */
	media_t		tape;		/* Default tape type */
	operator_t	operator;	/* Operator settings */
	sam_debug_t	debug;		/* Default debug mode */
	int		timeout;	/* Timeout for direct access */
	int		previews;	/* Max number of preview entries */
	int		stages;		/* Max number of stage requests */
	int		inodes;		/* Max number of archive inodes */
	int		tapemode;	/* Reset value for tape drives mode */
	int		log_facility;	/* Syslog facility */
	int		stale_time;	/* Stale preview time */
	int		idle_unload;	/* Idle time to force media unload */
	int		shared_unload;	/* Idle time to unload shared drive */
	uint32_t	flags;
	char		dis_mes[DIS_MES_TYPS][DIS_MES_LEN]; /* System msg */
};


/*
 * File Set disk entry structure for sam_fsdisk() API
 */

struct sam_fs_disk {
	int		eq;		/* Equipment ordinal of disk in */
					/* family set */
	long long	space;		/* Space left on this disk */
	long long	capacity;	/* Capacity available on this disk */
};

/*
 * Bit defines used to set flags for sam_chmed() API
 */

#define	CMD_CATALOG_NEEDS_AUDIT	0x000001
#define	CMD_CATALOG_SLOT_INUSE	0x000002
#define	CMD_CATALOG_LABELED	0x000004
#define	CMD_CATALOG_BAD_MEDIA	0x000008
#define	CMD_CATALOG_SLOT_OCCUPIED 0x000010
#define	CMD_CATALOG_CLEANING	0x000020
#define	CMD_CATALOG_BAR_CODE	0x000040
#define	CMD_CATALOG_WRITE_PROTECT 0x000080
#define	CMD_CATALOG_READ_ONLY	0x000100
#define	CMD_CATALOG_RECYCLE	0x000200
#define	CMD_CATALOG_UNAVAIL	0x000400
#define	CMD_CATALOG_EXPORT_SLOT 0x000800
#define	CMD_CATALOG_VSN		0x001000
#define	CMD_CATALOG_STRANGE	0x002000
#define	CMD_CATALOG_INFO	0x004000

/*
 * Miscellaneous API defintions
 */

#ifndef	ROBOT_NO_SLOT
#define	ROBOT_NO_SLOT  (0xffffffff)	/* Used to flag no slot specified */
#endif


/*
 * API function calls
 */

/* Audit media in a robot API */
int sam_audit(ushort_t eq_ord, uint_t ea, int wait_response);

/* Change flags in media catalog API */
int sam_chmed(ushort_t eq_ord, uint_t ea, int modifier, char *media, char *vsn,
	int flags, int on_off, int wait_response);

/* Change values in media catalog API */
int sam_chmed_value(struct CatalogEntry *ce, int flags, long long value);

void sam_chmed_doit(struct CatalogEntry *ce, uint_t flags, int value);

/* Clear request from specified removable media mount request ea */
int sam_clear_request(uint_t src_slot, int wait_response);

/* Export media from a robot API */
int SamExportCartridge(char *volspec, int WaitResponse, int OneStep,
	void (*MsgFunc)(int code, char *msg));
int sam_export(ushort_t eq_ord, char *vsn, int ea, int wait_response,
	int OneStep);

/* Import media to a robot API */
int sam_import(ushort_t eq_ord, char *vsn, char *media_nm, int audit_eod,
	int wait_response);

/* Load media into a robot drive API */
int SamMoveCartridge(char *volspec, int DestSlot, int WaitResponse,
	void (*MsgFunc)(int code, char *msg));
int sam_load(ushort_t eq_ord, char *vsn, char *media, int ea, int modifier,
	int wait_response);

/* Move media from a source ea to a destination ea in a robot API */
int sam_move(ushort_t eq_ord, int src_ea, int dest_ea,
	int wait_response);

/* Write a optical disk label API */
int sam_odlabel(ushort_t eq_ord, char *new_vsn, char *old_vsn, uint_t ea,
	int modifier, char *use_info, int erase, int wait_response);

/* Reserve volume for archiving. */
int SamReserveVolume(char *volspec, time_t rtime, char *reserve,
	void (*MsgFunc)(int code, char *msg));

/* Set file system maximum contiguous blocks */
int sam_set_fs_contig(ushort_t eq_ord, int type, int contig, int wait_response);

/* Set file system threshold minimum and maximum values */
int sam_set_fs_thresh(ushort_t eq_ord, int min_threshold, int max_threshold,
	int wait_response);

/* Set operational state of device API */
int sam_set_state(ushort_t eq_ord, dstate_t new_state, int wait_response);

/* Obtain default settings and current system messages API */
int sam_settings(struct sam_def_values *defaults, int size);

/* Set priority in preview queue  API */
int SamSetPreviewPri(uint_t  pid, float newpri);

/* Write a tape label API */
int sam_tplabel(ushort_t eq_ord, char *new_vsn, char *old_vsn, uint_t ea,
	int modifier, int block_size, int erase, int wait_response);

/* Unload the media from a device API */
int sam_unload(ushort_t eq_ord, int wait_response);

/* Unreserve volume for archiving. */
int SamUnreserveVolume(char *volspec, void (*MsgFunc)(int code, char *msg));

/* Interpret error number and return error string API */
char *sam_errno(int err);


#ifdef  __cplusplus
}
#endif

#endif /* !defined(_AML_SAMAPI_H) */
