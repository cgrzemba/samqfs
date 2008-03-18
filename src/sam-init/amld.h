/*
 *  amld.h - sam-amld program definitions.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#if !defined(_AML_H)
#define	_AML_H

#pragma ident "$Revision: 1.14 $"

#include "stdio.h"
#include "sam/defaults.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "aml/shm.h"

#define	AML_DIR SAM_VARIABLE_PATH"/amld" /* sam-amld base directory */
#define	NUM_FIFOS 2

/* align w to t alignment return increment to l */
#define	  ALIGN(w, t, l) w += (l = (sizeof (t) - (w & (sizeof (t) -1))))

#define	  SAM_FS_FIFO_THREAD	0
#define	  SAM_CMD_FIFO_THREAD	1

/*
 * identifers for  the children of sam-amld.  All children are passed
 * the path to the main FIFO as argv[1], master shared memory segment
 * id as argv[2] and the preview shared memory segment id as argv[3].
 */

#define	  CHILD_SCANNER		1
#define	  SCANNER_CMD		"sam-scannerd"
#define	  CHILD_ROBOTS		2
#define	  ROBOTS_CMD		"sam-robotsd"
#define	  CHILD_SAMRPC		3
#define	  SAMRPC_CMD		"sam-rpcd"
#define	  CHILD_CATSERVER	4
#define	  CATSERVER_CMD		"sam-catserverd"

#define	  NUMBER_OF_CHILDREN	4

#define	  DEFAULT_RESTART_DELAY	30    /* time to wait to restart */
#define	  RESTART		0
#define	  NO_RESTART		1
#define	  NO_AUTOSTART		2

#define	  STD_ARGS		0	/* master shmid and preview shmid */
#define	  SPECIAL_ARGS		1	/*  */

/* some structures for sam-amld */

/* To keep track of the children, children are so hard to keep track of */
typedef struct {
	int	pid;			  /* pid of the child */
	int	who;			  /* who is it */
	int	status;			/* exit status */
	int	restart;			/* restart/start flags */
	int	arguments;		    /* arguments flag */
	char    *cmd;			 /* what command */
	char    **args;			/* speical arguments */
	time_t  start_time;		   /* when did it start */
	time_t  restart_time;		 /* time to restart */
}child_pids_t;

typedef struct sam_si_fs_fifo {
	struct sam_fs_fifo	command;
	struct sam_si_fs_fifo  *next;
}sam_si_fs_fifo_t;

struct DeviceParams *DeviceParams;

#if defined(DEC_INIT)

#define	SET_PID_ENTRY(x, z, y, w)   0, x, 0, z, w, y, NULL, 0, 0
child_pids_t pids[NUMBER_OF_CHILDREN] = {
	SET_PID_ENTRY(CHILD_SCANNER, RESTART, SCANNER_CMD, STD_ARGS),
	SET_PID_ENTRY(CHILD_ROBOTS, RESTART, ROBOTS_CMD, STD_ARGS),
	SET_PID_ENTRY(CHILD_SAMRPC, NO_AUTOSTART | NO_RESTART, SAMRPC_CMD,
	    STD_ARGS),
	SET_PID_ENTRY(CHILD_CATSERVER, RESTART | NO_AUTOSTART, CATSERVER_CMD,
	    STD_ARGS),
};
#else
extern child_pids_t pids[];
#endif

/* Public data declaration/initialization macros. */
#undef DCL
#undef IVAL
#if defined(DEC_INIT)
#define	DCL
#define	IVAL(v) = v
#else /* defined(DEC_INIT) */
#define	DCL extern
#define	IVAL(v) /* v */
#endif /* defined(DEC_INIT) */

DCL shm_alloc_t master_shm;
DCL shm_alloc_t preview_shm;

DCL boolean_t Daemon IVAL(TRUE);
DCL boolean_t LicenseExpired IVAL(FALSE);
DCL boolean_t SlotsUsedUp IVAL(FALSE);

/* Public function. */
void *add_catalog_cmd(void *);
void *cancel_for_fs(void *);
void *clean_drive(void *);
void *fifo_cmd_export_media(void *);
void *fifo_cmd_import_media(void *);
void *fifo_cmd_tapealert(void *);
void *fifo_cmd_sef(void *);
void *init_for_fs(void *);
void *fifo_cmd_load_unavail(void *);
void *log_for_fs(void *);
void makeShm(void);
void *manage_cmd_fifo(void *);
void *manage_fs_fifo(void *);
void *manage_license(void *);
void *move_slots(void *);
void *load_for_fs(void *);
void *mount_slot(void *);
void *position_rmedia_for_fs(void *);
void *robot_label(void *);
void *robot_unload(void *);
void *rmt_server_thr(void *);
void *scanner_label(void *);
void *set_state(void *);
void *stage_for_fs(void *);
void *fifo_cmd_start_audit(void *);
void *unload_cmd(void *);
void *unload_for_fs(void *);
void *wmstate_for_fs(void *vcmd);
void alloc_shm_seg(void);
void check_children(child_pids_t []);
void kill_off_threads(void);
void rel_shm_seg(void);
void start_a_process(child_pids_t *, char **);
void start_children(child_pids_t []);
void reap_child(child_pids_t []);
void kill_children(child_pids_t []);
void set_media_to_default(media_t *, sam_defaults_t *);
void get_defaults(sam_defaults_t *defaults);
void ReadPreviewCmd(preview_tbl_t *preview, prv_fs_ent_t *preview_fs_table);
int will_use_up_slots(dtype_t rtype);

#if defined(lint)
#include "sam/lint.h"
#define	cond_init (void)cond_init
#define	cond_signal (void)cond_signal
#define	cond_wait (void)cond_wait
#define	sema_init (void)sema_init
#define	sema_wait (void)sema_wait
#define	sema_post (void)sema_post
#endif /* defined(lint) */

#endif  /* _AML_H */
