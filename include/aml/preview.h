/*
 * preview.h - Optical disk/tape request preview tables.
 *
 * Contains definitions for the optical disk & tape preview
 * tables.
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

#ifndef _AML_PREVIEW_H
#define	_AML_PREVIEW_H

#pragma ident "$Revision: 1.15 $"

#ifdef linux
#include <pthread.h>
#else
#include <synch.h>
#endif /* linux */
#include "sam/resource.h"
#include "sam/names.h"
#include "aml/types.h"
#include "sam/fs/amld.h"


/* Definition for preview scheduling parameters filename */
#define	PREVIEW_CMD	SAM_CONFIG_PATH"/preview.cmd"

/*
 * Macros for access "pointers" in the segment to other places in the
 * segment.  Since having a pointer to somewhere in the segment makes
 * no sense (since every process has a different base address) the
 * pointers in the segment are actually offsets from the beginning of
 * the segment.
 */

/*
 * find the address pointed to by x
 */
#define	PRE_REF_ADDR(x)	(((x) == 0) ? NULL : \
	((void *) ((char *)preview_shm.shared_memory + (int)(x))))

/*
 * get address as an offset from the segment
 */
#define	PRE_GET_OFFS(x)		Ptrdiff((x), preview_shm.shared_memory)


/*
 * remove_preview_ent has a flags parameter. In all cases the entry is
 * removed.  All notify flags involve only the file system since it is
 * the only thing that can really be waiting.
 */
#define	PREVIEW_NOTHING		1		/* just remove the entry */
#define	PREVIEW_NOTIFY		2		/* notify those waiting */


/*
 * function call flags
 */
#define	PREVIEW_SET_FOUND	1	/* Search and call function */
#define	PREVIEW_HISTORIAN	2	/* identify as historian to preview */

#define	MAX_PREVIEW		100	/* Default preview buffer len */
#define	MAX_STAGES		1000	/* Default max stages */


/* Defaults for the priority factors of the preview requests */

/*
 * Bump each archive request if high water mark reached for filesystem.
 * NEED more work here as stage request maybe originated by archiver
 * and needs to get bump here as well.
 * Also may need to differentiate request for the other filesystem
 * states: between lwm and hwm, and below lwm.
 */

#define	PRV_LWM_FACTOR_DEFAULT		0
#define	PRV_LHWM_FACTOR_DEFAULT		0
#define	PRV_HLWM_FACTOR_DEFAULT		0
#define	PRV_HWM_FACTOR_DEFAULT		0

/*
 * Default priority for selected VSNs
 * For backward compatibility catalog contains only bit about set of
 * VSNs with high priority. Define here numeric value for the "high".
 * In future VSN entry should contain this value for each VSN, requires
 * change in the catalog structure.
 */
#define	PRV_VSN_FACTOR_DEFAULT		1000


/*
 * Default age priority.
 * Value of 1 means that each second will add 1 to the total request
 * priority.
 */
#define	PRV_AGE_FACTOR_DEFAULT		1


/*
 * Default original priorities based on originator
 */
#define	PRV_FS_PRIO_DEFAULT	0		/* request from filesystem */
#define	PRV_CMD_PRIO_DEFAULT	0		/* request from command line */
#define	PRV_REM_PRIO_DEFAULT	0		/* request from remote sam */
#define	PRV_MIG_PRIO_DEFAULT	0		/* request from migkit */


typedef struct preview {
#ifdef linux
	pthread_mutex_t	p_mutex;
#else
	mutex_t		p_mutex;	/* Mutex for this entry */
#endif /* linux */
	int		prv_id;		/* Preview id (formerly slot) */
	sam_handle_t	handle;		/* fs handle information */
	int		stages;		/* Offset to additionl stage reqs. */
	uint_t		sequence;	/* sequence number for this entry */
	ushort_t
		busy		:1,	/* Entry is busy don't touch */
		in_use		:1,	/* Entry in use (occupied) */
		p_error		:1,	/* Clear vsn requested */
		write		:1,	/* Write access request flag */
		fs_req		:1,	/* File system requested */
		block_io	:1,	/* Use block I/O for optical/tape IO */
		stage		:1,	/* Stage request flag */
		flip_side	:1;	/* If flipside already mounted */
	ushort_t	count;		/* Number of requests for this media */
	ushort_t	element;	/* Element address */
	equ_t		robot_equ;	/* Equipment if robot */
	equ_t		remote_equ;	/* Equipment if remote */
	uint_t		slot;		/* Slot number if robot */
	enum callback callback;		/* Function to complete processing */
	sam_resource_t resource;	/* Resource record */
	time_t		ptime;		/* Time of request */
	float		stat_priority;	/* Static priority of request */
	float		fswm_priority;	/* Fs water mark priority of request */
	float		priority;	/* Total priority of request */
} preview_t;


/*
 * Stage_preview is used to hold stage requests. Only
 * the first stage request for a specific media is stored in the
 * preview entry.  Any additional stage requests for the same
 * media are stored in the list.  The count field in the preview
 * table is all active entries.  If the count field goes 0 (zero)
 * then the preview entry can be removed.
 */

typedef struct stage_preview {
	sam_handle_t	handle;		/* FS handle information */
	uint_t		next;		/* Offset for next entry */
	time_t		ar_time;	/* Time of archive (from resource) */
	sam_arch_rminfo_t rm_info;	/* rm_info for the stage request */
} stage_preview_t;


/*
 * Each filesystem may modify archive vs. stage requests priorities
 * when high water mark reached. prv_fs_ent_t contains 4 factors
 * corresponding to 4 states filesystem could be in regarding
 * LWM and HWM. prv_wm_state indicates filesystem state.
 * Factors are set at initialization from preview.cmd .
 * Priority of the archive requests will be modified using the factors as
 *		priority = current_priority + prv_wm_factor
 */

typedef struct {
	equ_t		prv_fseq;		/* Filesystem eq */
	uname_t		prv_fsname;		/* Filesystem name */
	fs_wmstate_e prv_fswm_state;		/* Water mark state */
	float		prv_fswm_factor[FS_WMSTATE_MAX]; /* Water mark prio */
} prv_fs_ent_t;


/*
 * preview_tbl_t contains factors used to calculate request's
 * dynamic priorities. prv_age_factor by default 1, which adds
 * to priority number of seconds since the request was issued.
 * prv_fs_table is a pointer (offset) to prv_fs_ent_t table in
 * the shared memory.
 * (see man page for preview.cmd).
 */

typedef struct {
#ifdef linux
	pthread_mutex_t	ptbl_mutex;
#else
	mutex_t 	ptbl_mutex;	/* Mutex for locking preview table */
#endif /* linux */
	int		stages;		/* Offset to the first free stage */
	int		avail;		/* Number of available entries */
	int		ptbl_count;	/* Number of entries used */
	uint_t		sequence;	/* Next sequence number */
	float		prv_age_factor;	/* Request age factor for priority */
	float		prv_vsn_factor;	/* Vsn priority factor */
	uint_t		prv_fs_table;	/* Offset to preview fs table */
	int		fs_count;	/* Number of filesystems */
	preview_t	p[1];		/* Preview entries table */
} preview_tbl_t;

void prv_fswm_priority(equ_t fseq, fs_wmstate_e fswm_state);

#endif /* _AML_PREVIEW_H */
