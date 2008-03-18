/*
 *	amld.h - sam-amld definitions for all the SAM-QFS file systems.
 *
 *	Defines the structure of the sam-amld command queue.
 *
 *	Solaris 2.x Sun Storage & Archiving Management File System
 *
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

#ifdef sun
#pragma ident "$Revision: 1.20 $"
#endif


#ifndef	_SAM_FS_AMLD_H
#define	_SAM_FS_AMLD_H

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif


/* File system commands */
/* If a resource record is defined, it must be the first field in the struct */
#define	FS_FIFO_MAGIC		0x4867436d

#define	FS_FIFO_LOAD		1	/* mount a vsn */
#define	FS_FIFO_UNLOAD		2	/* unload a vsn */
#define	FS_FIFO_CANCEL		3	/* cancel a command */
#define	FS_FIFO_RESYNC		4	/* resync fifo (sam-init restart) */
#define	FS_FIFO_WMSTATE		5	/* filesystem water mark state */
#define	FS_FIFO_POSITION	6	/* position removable media file */


/* FS_FIFO_LOAD */

typedef struct {
	sam_resource_t resource;	/* resource record */
} fs_load_t;


/* FS_FIFO_UNLOAD */

typedef struct {
	sam_resource_t resource;	/* resource record */
	SAM_POINTER(void) mt_handle;	/* Generic pointer for daemon */
#if defined(__sparcv9) || defined(__amd64)
	dev32_t rdev;			/* Raw device */
#else /* __sparcv9 || __amd64 */
	dev_t rdev;			/* Raw device */
#endif /* __sparcv9 || __amd64 */
	int io_count;			/* count of rm operations */
} fs_unload_t;


/* FS_FIFO_CANCEL */

typedef struct {
	sam_resource_t resource;	/* resource record */
	int cmd;			/* command to be canceled */
} fs_cancel_t;

/* FS_FIFO_RESYNC */

#if defined(__sparcv9) || defined(__amd64)

typedef struct {
	time_t seq;
} fs_resync_t;

#else /* __sparcv9 || __amd64 */

typedef struct {
	int pad;
	time_t seq;
} fs_resync_t;

#endif /* __sparcv9 || __amd64 */


/*
 * FS_FIFO_WMSTATE
 * no response back to the file system.
 */
typedef enum {
	FS_LWM = 0,			/* below LWM */
	FS_LHWM,			/* between LWM & HWM coming up */
	FS_HLWM,			/* between LWM & HWM going down */
	FS_HWM,				/* above HWM */
	FS_WMSTATE_MAX			/* max number of fs wm states */
} fs_wmstate_e;


#define	FS_WMSTATE_MIN_SECS 60

typedef struct {
	equ_t fseq;
	fs_wmstate_e wmstate;
} fs_wmstate_t;

/*
 * FS_FIFO_POSITION
 * position removable media file
 */
typedef struct {
	sam_resource_t	resource;	/* resource record */
#if defined(__sparcv9) || defined(__amd64)
	dev32_t			rdev;	/* raw device */
#else /* __sparcv9 || __amd64 */
	dev_t			rdev;
#endif /* __sparcv9 || __amd64 */
	u_longlong_t	setpos;		/* requested position */
} fs_position_t;

/* sam_handle_t - Handle for daemon requests from file sysgem. */

typedef struct sam_handle {
	sam_id_t id;			/* File identification */
	pid_t pid;			/* pid number for requestor */
	uid_t uid;			/* real uid number for requestor */
	gid_t gid;			/* read gid number for requestor */
	equ_t fseq;			/* family set equipment number */
	union {
		struct {
			ushort_t
			cs_val:1,	/* valid checksum: 1=yes, 0=no */
			cs_use:1,	/* checksum attribute: 1=on, 0=off */
			eagain:1,	/* Issued again because EAGAIN error */
			arstage:1,	/* Stage started by archiver */

			stage_wait:1,	/* Stage with wait */
			unused:11;
		} b;
	ushort_t bits;
	} flags;
	/* Generic ptr for daemon */
	SAM_POINTER(struct sam_fs_fifo) fifo_cmd;
	uchar_t cs_algo;		/* checksum algorithm indicator */
	offset_t stage_off;
	offset_t stage_len;
	ushort_t seqno;			/* removable media sequence number */
} sam_handle_t;


/* sam_fs_fifo.  This struct is used by the file system. */

typedef struct sam_fs_fifo {
	uint_t magic;			/* command start bit sequence */
	pid_t sam_amld_pid;		/* sam-amld's pid for validation */
	int cmd;			/* command */
	sam_handle_t handle;		/* handle returned in response */
	union {
		fs_load_t fs_load;		/* load a vsn */
		fs_unload_t fs_unload;		/* unload a vsn */
		fs_cancel_t fs_cancel;		/* cancel a command */
		fs_resync_t fs_resync;		/* resync the fifo */
		fs_wmstate_t fs_wmstate;	/* fs water mark state */
		/* pos'n removable media file */
		fs_position_t fs_position;
	} param;
} sam_fs_fifo_t;


/*
 * sam_fs_fifo_ctl_t - provides means for returning errno to waiting thread.
 */

typedef struct {
	sam_fs_fifo_t fifo;
	int ret_err;
	char fifo_name[32];
} sam_fs_fifo_ctl_t;

/* Number of sam-amld cmd buffers */

#define	SAM_AMLD_CMD_BUF	(100)


/*
 * A cmd queue entry:
 *	cmd_cv: the cv that signals when the cmd has been picked up
 *		by sam-amld.
 *	cmd_mutex: the lock for the above cv.
 *	forward: the forward link.
 *	back:	the backward link.
 *	qcmd_error:	the error code when sam-amld doesn't pick up the cmd.
 *	cmd:	the cmd itself.
 */

typedef struct samamld_cmd_queue {
#ifndef	linux
	kcondvar_t	cmd_cv;
	char		pad[4];	/* for amd64 64bit alignment of cmd_mutex */
	kmutex_t	cmd_mutex;
#endif	/* linux */
	struct samamld_cmd_queue *forward;
	struct samamld_cmd_queue *back;
	int qcmd_error;
	int qcmd_wait;
	int blk_flag;
	sam_fs_fifo_t	cmd;
} samamld_cmd_queue_t;


/*
 * The cmd queue header:
 *	front:			the first cmd in the queue.
 *	end:			the last cmd in the queue.
 *	cmd_queue_mutex:	lock for the cmd queue.
 *	cmd_lockout_mutex:	lock for the cmd lock out logic
 *	cmd_lockout_cv:		cv to wait on when the cmd queue is locked.
 *	cmd_queue_cv:		cv for sam-amld to wait on when there are no
 *					cmds in the queue.
 *	cmd_lock_flag:		cmd lock out flag.
 *	cmd_queue_timeout:	timeout value to determine if sam-amld has
 *					timed out.
 */

typedef struct {
	samamld_cmd_queue_t *front;
	samamld_cmd_queue_t *end;
#ifndef	linux
	kmutex_t cmd_queue_mutex;
	kmutex_t cmd_lockout_mutex;
	kcondvar_t cmd_lockout_cv;
	kcondvar_t  cmd_queue_cv;
#endif	/* linux */
	int cmd_lock_flag;
	clock_t cmd_queue_timeout;
} samamld_cmd_queue_hdr_t;


/*
 * The command table:
 *	cmd_buffers:	a kmem_alloc'ed array of cmd buffers.
 *	cmd_buffer_free_list:	pointer to the first buffer on the free list.
 *	samamld_cmd_queue_hdr:	The cmd queue header.
 *	queue_mutex:	Free list mutex.
 *	cmd_buffer_free_list_count:	Count of free list entries.
 */

typedef struct samamld_cmd_table {
	samamld_cmd_queue_t *cmd_buffers; /* sam-amld command buffers */
	/* free list of sam-amld cmds */
	samamld_cmd_queue_t *cmd_buffer_free_list;
	kcondvar_t free_list_cond;
	samamld_cmd_queue_hdr_t samamld_cmd_queue_hdr;
#ifndef	linux
	char	pad[4];	/* needed for amd64 64bit alignment of queue_mutex */
	kmutex_t queue_mutex;
#endif	/* linux */
	int cmd_buffer_free_list_count;
} samamld_cmd_table_t;


/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#endif  /* _SAM_FS_AMLD_H */
