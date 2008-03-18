/*
 * message.h - structures for message passing.
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


#if !defined(_AML_MESSAGE_H)
#define	_AML_MESSAGE_H

#pragma ident "$Revision: 1.17 $"

#include "sam/types.h"
#include "sam/resource.h"
#include "aml/exit_fifo.h"

#include "aml/remote.h"

/* Structs and defines for messages */

/* Message types */

enum sam_mess_type {
	MESS_MT_SHUTDOWN = 0,	/* shutdown message */
	MESS_MT_CRITICAL,	/* critical */
	MESS_MT_URGENT,		/* urgent */
	MESS_MT_APIHELP,	/* robot helper response */
	MESS_MT_HISTORIAN,	/* historian */
	MESS_MT_RS_SERVER,	/* remote sam server */
	MESS_MT_NORMAL = 10,	/* normal delivery */
	MESS_MT_VOID = 0xff	/* no message present */
};

/*
 * The STKACS response will have a message body that is the union of all the
 * types of responses that can be returned..  This is defined in
 * src/robots/stk/stk.h since it involves most all of the stk include files,
 * which we don't really want to include here.  It is up to the code in the
 * stk robot to verify that the message area is large enough to include the
 * entire response message and make the message union below large enough to
 * hold it.
 *
 * The GRAUAPI response has the same problem as described above.  See
 * src/robots/grau/grau.h etal.
 *
 */

/* Messages commands */

enum sam_message {
	MESS_CMD_SHUTDOWN,	/* shutdown request */
	MESS_CMD_STATE,		/* device changed state */
	MESS_CMD_LABEL,		/* request to label media */
	MESS_CMD_MOUNT,		/* request to mount media */
	MESS_CMD_AUDIT,		/* request to audit MESS/slot */
	MESS_CMD_PREVIEW,	/* preview table added entry */
	MESS_CMD_UNLOAD,	/* unload equipment */
	MESS_CMD_IMPORT,	/* import cartridge */
	MESS_CMD_EXPORT,	/* export cartridge */
	MESS_CMD_TODO,		/* todo request */
	MESS_CMD_CLEAN,		/* clean drive request */
	MESS_CMD_MOVE,		/* move media in catalog request */
	MESS_CMD_ADD,		/* add vsn to  catalog request */
	MESS_CMD_LOAD_UNAVAIL,	/* load media unavail drive */
	MESS_CMD_HIST_UPDATE,	/* update info (historian only) */
	MESS_CMD_HIST_GET,	/* return info (historian only) */
	MESS_CMD_HIST_INFO,	/* returned info (from historian) */
				/* message_type == MESS_MT_HISTORIAN */
	MESS_CMD_TP_MOUNT,	/* Third party mount request  */
	MESS_CMD_ACK,		/* Wake up network attached daemon  */
	/* no-op */
	MESS_CMD_TAPEALERT,	/* tapealert request */
	MESS_CMD_SEF,		/* sef request */
	MESS_CMD_max
};

/*
 * All messages less than or equal to ACCEPT_DOWN, will be accepted if the
 * robot is off or down.  All others will be silently ignored.
 */
#define	  ACCEPT_DOWN    MESS_CMD_STATE
#define	  MESSAGE_MAGIC    0xa9c2d628
#define	  REQUEST_NOT_COMPLETE  -100

/* structures used in the message area */

/* label_request */

typedef struct label_request {
	int		block_size;	/* requested blocksize */
	uint_t		slot;		/* slot number to label */
	uint_t		part;		/* partition to label */
	uint_t		flags;		/* flags */
	media_t		media;
	int		partition;	/* partition id */
	char		vsn[32];	/* vsn to write */
	char		old_vsn[32];	/* former vsn (to be checked) */
	char		info[128];	/* info */
} label_request_t;

/* mount_request */

typedef struct mount_request {
	uint_t		slot;		/* slot number to mount */
	uint_t		part;		/* partition to mount */
	uint_t		flags;		/* flags */
	media_t		media;
	char		vsn[32];	/* vsn to mount */
} mount_request_t;

/* state change */

typedef struct state_change {
	equ_t		eq;		/* equipment changing state */
	uint_t		flags;
	int		state;		/* new state */
	int		old_state;	/* old state */
} state_change_t;

typedef struct audit_request {
	equ_t		eq;		/* equipment to audit */
	uint_t		slot;		/* optional slot number */
	uint_t		flags;
} audit_request_t;

typedef struct unload_request {
	equ_t		eq;		/* equipment to unload */
	uint_t		slot;		/* optional slot for robots */
	uint_t		flags;
} unload_request_t;

typedef struct import_request {
	equ_t		eq;		/* equipment to import into */
	uint_t		flags;
	media_t		media;
} import_request_t;

typedef struct todo_request {
	enum todo_sub	sub_cmd;	/* sub command */
	equ_t		eq;		/* equipment to work on */
	void		*mt_handle;	/* handle, used on unload */
	enum callback	callback;
	sam_handle_t	handle;
	sam_resource_t	resource;
} todo_request_t;

typedef struct export_request {
	equ_t		eq;		/* equipment to export from */
	uint_t		slot;		/* slot number */
	uint_t		flags;
	media_t		media;
	int		element;
	char		vsn[32];	/* vsn to export */
} export_request_t;

typedef struct catchg_request {
	uint_t		flags;		/* which flags to change in catalog */
	uint_t		slot;		/* slot to change */
	longlong_t	value;		/* what value to set them to */
	media_t		media;
	char		vsn[32];	/* vsn to change/set */
} catchg_request_t;

typedef struct cmdmove_request {
	uint_t		slot;		/* source slot */
	uint_t		d_slot;		/* destination slot */
	uint_t		flags;
} cmdmove_request_t;

typedef struct addcat_request {
	equ_t		eq;		/* robot (must be a grau) */
	uint_t		flags;
	media_t		media;
	char		vsn[32];	/* vsn to add */
	uchar_t		bar_code[BARCODE_LEN + 1];	/* barcode is flag */
							/* is set */
} addcat_request_t;

typedef struct clean_request {
	equ_t		eq;		/* drive to clean */
	uint_t		flags;
} clean_request_t;

/* load_unavail_request */
typedef struct load_u_request {
	uint_t		slot;		/* slot number to mount */
	uint_t		part;		/* partition to mount */
	uint_t		flags;		/* flags */
	media_t		media;		/* media type */
	equ_t		eq;		/* drive to mount on */
	char		vsn[32];	/* vsn to mount */
} load_u_request_t;

/* Notify third party of mount */
typedef struct tp_notify_mount {
	uint_t		errflag;	/* return error from mount */
	void		*event;		/* address of waiting thread */
	equ_t		eq;		/* eq where media is mounted */
} tp_notify_mount_t;

typedef struct tapealert_request {
	equ_t		eq;		/* drive for tapealert activity */
	uint_t		flags;		/* action on/off etc */
} tapealert_request_t;

typedef struct sef_request {
	equ_t		eq;		/* drive for sef activity */
	uint_t		flags;		/* action on/off etc */
	int		interval;	/* timeout interval */
} sef_request_t;

/*
 * This must match QU_DRV_STATUS in
 * src/robots/stk/stk_includes/api/structs_api.h
 */
typedef struct {
	char		drive_id[4];
	char		vol_id[7];
	char		drive_type;
	int		state;
	int		status;
} qu_drv_status_t;

typedef struct query_all_drvs {
	char		stk_resp_header[30];	/* Space for stk_resp_headr_t */
	int		status;
	unsigned short	count;
	qu_drv_status_t drvstat[49];
} query_all_drvs_t;

typedef struct {
	char		vol_id[7];
	int		status;
	unsigned short	count;
	qu_drv_status_t drvstat[165];
} qu_mnt_status_t;

typedef struct query_mnt_stat {
	char		stk_resp_header[30];
	int		status;
	unsigned short	count;
	qu_mnt_status_t mntstat[1];
} query_mnt_stat_t;


/* Flags for export media */

#define	EXPORT_FLAG_MASK	0xf0000
#define	EXPORT_BY_VSN		0x10000
#define	EXPORT_BY_SLOT		0x20000
#define	EXPORT_BY_EQ		0x40000

/* Flags for audit */

#define	AUDIT_FIND_EOD		0x80000

/* Flags for addcat_request */

#define	ADDCAT_BARCODE		0x80000		/* has a barcode */

/* Message area structures */

typedef struct sam_message_s {
	uint_t		magic;
	enum sam_message command;
	exit_FIFO_id	exit_id;
	union {
		uchar_t			start_of_request;
		audit_request_t		audit_request;
		catchg_request_t	catchg_request;
		clean_request_t		clean_request;
		export_request_t	export_request;
		import_request_t	import_request;
		label_request_t		label_request;
		mount_request_t		mount_request;
		load_u_request_t	load_u_request;
		state_change_t		state_change;
		todo_request_t		todo_request;
		unload_request_t	unload_request;
		cmdmove_request_t	cmdmove_request;
		addcat_request_t	addcat_request;
		tp_notify_mount_t	tp_notify_mount;
		rmt_mess_t		rmt_message;
		tapealert_request_t	tapealert_request;
		sef_request_t		sef_request;
		query_all_drvs_t	query_all_drvs_request;
		query_mnt_stat_t	query_mnt_stat;
	} param;
} sam_message_t;

#define	MESS_HEADER_SIZE (sizeof (uint_t) + sizeof (enum sam_message))

#if !defined(_KERNEL)
/* message area in the shared memory segment */
typedef struct message_request {
	mutex_t		mutex;		/* mutex for the struct */
	cond_t		cond_i;		/* for the issuers */
	cond_t		cond_r;		/* for the receivers */
	enum sam_mess_type mtype;	/* type of message */
	sam_message_t	message;	/* the message */
} message_request_t;
#endif

#endif				/* !defined(_AML_MESSAGE_H) */
