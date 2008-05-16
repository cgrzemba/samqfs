/*
 * types.h - SAM-FS types.
 *
 * Description:
 *    Defines SAM-FS types.
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

#if !defined(_AML_TYPES_H)
#define	_AML_TYPES_H

#pragma ident "$Revision: 1.14 $"

#include "sam/types.h"

/* the following used in the SC_fsreleaser system call */
#define	RELEASER_STARTED 1
#define	RELEASER_FINISHED 0

#define	DF_THR_STK    (0)
#define	SM_THR_STK    DF_THR_STK
#define	MD_THR_STK    DF_THR_STK
#define	LG_THR_STK    DF_THR_STK
#define	HG_THR_STK    DF_THR_STK

#define	DIS_MES_TYPS    2		/* Number of display message types */
#define	DIS_MES_LEN    128		/* Length of display message area */
#define	DIS_MES_NORM    0		/* normal message */
#define	DIS_MES_CRIT    1		/* critical message */

/* Define debug bits for "on the fly" debug and tracing */

typedef uint32_t sam_debug_t;

/* Debug flag definitions. */

#define	SAM_DBG_LOGGING	0x80000000	/* sam-amld fifo and syscall logging */
#define	SAM_DBG_DEBUG	0x40000000	/* General debug stuff */
#define	SAM_DBG_TMOVE	0x20000000	/* Trace robot movement information */
#define	SAM_DBG_TMESG	0x10000000	/* Trace robot message passing */

#define	SAM_DBG_TIME	0x08000000	/* various timing */
#define	SAM_DBG_ODRNG	0x04000000	/* od range */
#define	SAM_DBG_LBL	0x02000000	/* labeling info */
#define	SAM_DBG_CANCEL	0x01000000	/* canceled */

#define	SAM_DBG_DISSCSI	0x00800000	/* display scsi */
#define	SAM_DBG_EVENT	0x00400000	/* event tracing */
#define	SAM_DBG_LOAD	0x00200000	/* load tracing */
#define	SAM_DBG_OPEN	0x00100000	/* open tracing */

#define	SAM_DBG_TSCSI	0x00080000	/* trace scsi */
#define	SAM_DBG_RBDELAY	0x00040000	/* robot delay start */
#define	SAM_DBG_STGALL	0x00020000	/* trace stageall */

#define	SAM_DBG_MIGKIT	0x00004000	/* trace remote sam */


#define	SAM_DBG_NONE	0x00000000	/* No trace or logging */
#define	SAM_DBG_ALL	0xfff3ffff	/* All but trace_scsi and robot_delay */
#define	SAM_DBG_DEFAULT SAM_DBG_LOGGING	/* Logging is always on */

/* A macro for testing if the debug bit(s) are set */
#define	DBG_LVL(l)  ((master_shm.shared_memory != NULL) && \
	(((shm_ptr_tbl_t *)master_shm.shared_memory)->debug & (l)))
/* A macro for testing if all the debug bit(s) are set */
#define	DBG_LVL_EQ(l)  ((((shm_ptr_tbl_t *)master_shm.shared_memory)->debug & \
	(l)) == (l))

#define	SAM_DBG_OPER_SET  0
#define	SAM_DBG_ROOT_SET  1

/* String to bits for debug */

typedef struct {
	char	*string;
	uint_t	bits;
	int	who;
} sam_dbg_strings_t;

#if defined(MAIN) && !defined(lint)
sam_dbg_strings_t sam_dbg_strings[] = {
	"logging", SAM_DBG_LOGGING, SAM_DBG_OPER_SET,
	"debug", SAM_DBG_DEBUG, SAM_DBG_OPER_SET,
	"moves", SAM_DBG_TMOVE, SAM_DBG_OPER_SET,
	"messages", SAM_DBG_TMESG, SAM_DBG_OPER_SET,
	"timing", SAM_DBG_TIME, SAM_DBG_OPER_SET,
	"od_range", SAM_DBG_ODRNG, SAM_DBG_OPER_SET,
	"labeling", SAM_DBG_LBL, SAM_DBG_OPER_SET,
	"canceled", SAM_DBG_CANCEL, SAM_DBG_OPER_SET,
	"disp_scsi", SAM_DBG_DISSCSI, SAM_DBG_OPER_SET,
	"events", SAM_DBG_EVENT, SAM_DBG_OPER_SET,
	"loads", SAM_DBG_LOAD, SAM_DBG_OPER_SET,
	"opens", SAM_DBG_OPEN, SAM_DBG_OPER_SET,
	"trace_scsi", SAM_DBG_TSCSI, SAM_DBG_ROOT_SET,
	"robot_delay", SAM_DBG_RBDELAY, SAM_DBG_OPER_SET,
	"stageall", SAM_DBG_STGALL, SAM_DBG_OPER_SET,
	"migkit", SAM_DBG_MIGKIT, SAM_DBG_OPER_SET,
	"all", SAM_DBG_ALL, SAM_DBG_OPER_SET,
	"none", SAM_DBG_NONE, SAM_DBG_OPER_SET,
	NULL, 0
};
#endif

extern sam_dbg_strings_t sam_dbg_strings[];

/* Callback functions */

enum callback {
	CB_NO_CALLBACK,			/* no callback */
	CB_NOTIFY_FS_LOAD,		/* load request */
	CB_NOTIFY_FS_ULOAD,		/* unload request */
	CB_NOTIFY_TP_LOAD,		/* third party load */
	CB_POSITION_RMEDIA		/* position removable media file */
};

/* Misc */
/* todo request sub commands */

typedef enum todo_sub {
	TODO_ADD,			/* add to list */
	TODO_CANCEL,			/* cancel the request */
	TODO_UNLOAD			/* unload the resource file */
} todo_sub_t;


/* Operator settings */

#define	SAM_ROOT 0			/* root authority */
#define	SAM_OPER 1			/* operator authority */
#define	SAM_USER 2			/* normal user */

/*
 * Must declare a operator_t variable(x) for these macros, along with
 * defining the master_shm value
 */
#define	SET_SAM_OPER_LEVEL(x) \
	x = GetDefaults()->operator; \
	x.gid = geteuid() == 0 ? SAM_ROOT : \
	(getegid() == x.gid ? SAM_OPER : SAM_USER);

#define	SAM_ROOT_LEVEL(x) (x.gid == SAM_ROOT)
#define	SAM_OPER_LEVEL(x) (x.gid < SAM_USER)

#define	SAM_OPER_LABEL(x) (x.gid == 0 || x.gid == SAM_OPER && x.priv.label)
#define	SAM_OPER_SLOT(x) (x.gid == 0 || x.gid == SAM_OPER && x.priv.slot)
#define	SAM_OPER_FULLAUDIT(x) (x.gid == 0 || \
	x.gid == SAM_OPER && x.priv.fullaudit)
#define	SAM_OPER_STATE(x) (x.gid == 0 || x.gid == SAM_OPER && x.priv.state)
#define	SAM_OPER_CLEAR(x) (x.gid == 0 || x.gid == SAM_OPER && x.priv.clear)
#define	SAM_OPER_OPTIONS(x) (x.gid == 0 || x.gid == SAM_OPER && x.priv.options)
#define	SAM_OPER_REFRESH(x) (x.gid == 0 || x.gid == SAM_OPER && x.priv.refresh)

typedef struct {
	gid_t gid;
	struct {
		uint32_t
			label	: 1,		/* labeling */
			slot	: 1,		/* slot changes */
			fullaudit:1,		/* full audits */
			state	: 1,		/* robot/device state changes */

			clear	: 1,		/* clear request option */
			options	: 1,		/* display options */
			refresh	: 1,		/* refresh settings */

			unused	:25;
	} priv;
} operator_t;


/* Misc defines that need to be here */

/*
 * A debug for malloc and mutexs.  Only works if the debug library
 * has been linked in.
 */
#if defined(MAIN)
void debug_init(void);
#pragma weak debug_init
#if !defined(lint)
void (*debug_dummy) (void) = debug_init; /* force the so library if used */
#endif
#endif

#endif /* !defined(_AML_TYPES_H) */
