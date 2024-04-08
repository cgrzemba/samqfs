/*
 * stk.h
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

/*
 * $Revision: 1.28 $
 */

#ifndef _STK_H
#define	_STK_H

#include "aml/device.h"
#include "aml/preview.h"
#include "aml/historian.h"
#include "../lib/librobots.h"
#include "../common/common.h"
#include "acssys.h"
#include "acsapi.h"
#include "identifier.h"

#define	STK_MAIN_THREADS	2	/* number of main thread */
#define	STK_MSG_THREAD		0	/* index of the message thread */
#define	STK_WORK_THREAD		1	/* index of the worklist thread */
#define	REQUEST_NOT_COMPLETE	-100	/* used for request completion */

/* Additional sense code qualifier */

#define	ERR_ASCQ_BECOMMING	0x1	/* Becomming ready */
#define	ERR_ASCQ_REZERO		0x2	/* Need to rezero unit */
#define	ERR_ASCQ_MECH		0x3	/* Mechanical Hindrance */

/* macros */

/* return a unsigned int pointer to a DRIVEID */
#define	U_ID(d)  (uint_t *)(&(d))

/* return 4 location components from a DRIVEID */
#define	DRIVE_LOC(d) \
	d.panel_id.lsm_id.acs, d.panel_id.lsm_id.lsm, d.panel_id.panel, d.drive

/* Compare drive_ids */
#define	DRIVE_ID_MATCH(a, b) (a.panel_id.lsm_id.acs == b.panel_id.lsm_id.acs &&\
	a.panel_id.lsm_id.lsm == b.panel_id.lsm_id.lsm &&\
	a.panel_id.panel == b.panel_id.panel &&\
	a.drive == b.drive) ? TRUE : FALSE

/* Structures */

/*
 * Layout and use of the robot private area.  Since the stk API is not
 * thread safe, a helper process is used to isse the command to the
 * API.  Since a fork blocks while there is outstanding system calls, the
 * helper must be started once and passed messages.  The private area is
 * used for the messages and control.
 */

enum stk_priv_type {
	STK_PRIV_NORMAL = 0,	/* there is a message */
	STK_PRIV_VOID = 0xff	/* no message */
};

typedef struct stk_priv_mess {
	mutex_t	mutex;			/* for the whole thing */
	cond_t	cond_i;			/* for the issuers */
	cond_t	cond_r;			/* for the receivers */
	enum	stk_priv_type mtype;	/* the message type */
	char	message[512];		/* the message */
}stk_priv_mess_t;

/*
 * Tables and structurs internal to the stk library driver.
 * For all of these structs, the main mutex is used for locking
 * all fields except list management and catalog access.  For those
 * fields use the mutex associated with the fields.
 */

typedef struct {
	STATUS		query_vol_status;
	unsigned short	count;
	QU_VOL_STATUS	vol_status[1];
} SAM_ACS_QUERY_VOL_RESPONSE;


typedef struct {
	STATUS		query_drv_status;
	unsigned short	count;
	QU_DRV_STATUS	drv_status[MAX_ID];
} SAM_ACS_QUERY_DRV_RESPONSE;


typedef struct {
	STATUS			eject_status;
	CAPID			cap_id;
	unsigned short		count;
	VOLID			vol_id[1];
	STATUS			vol_status[1];
} SAM_ACS_EJECT_RESPONSE;

typedef struct {
	STATUS			query_mnt_stat;
	unsigned short	count;
	QU_MNT_STATUS	mnt_status[1];
} SAM_ACS_QUERY_MNT_RESPONSE;

/* drive_state - One for each drive in a library. */

typedef struct drive_state_s {
	COMMON_DRIVE_STATE
	int	align;
	DRIVEID	drive_id;	/* leave after int to force int */
}drive_state_t;

/* xport_state.  Information about the state of a transport element. */
typedef struct xport_state_s {
	COMMON_XPORT_STATE
	SEQ_NO	sequence;	/* stk sequence number */
}xport_state_t;

/* The stk does not have one but the common code requires it */
typedef struct iport_state_s {
	COMMON_IPORT_STATE
}iport_state_t;

/*
 * library_t. Information about the state of the library itself.
 */

typedef struct media_capacity_s {
	int	count;		/* how many */
	uint64_t	*capacity;	/* array of capacities, units of 1024 */
}media_capacity_t;

typedef struct library_s {
	COMMON_LIBRARY_STATE
	stk_priv_mess_t		*help_msg;
	int			inc_free_running;
	pid_t			helper_pid;
	pid_t			ssi_pid;
	media_capacity_t	media_capacity;
	char			*access;
	CAPID			capid;
	boolean_t		hasam_running;
}library_t;

/*
 * stk_information_t - information sent to the transport for the helper
 * routine.
 */
typedef struct stk_information {
	VOLID	vol_id;
	int	d2;
	DRIVEID	drive_id;
}stk_information_t;

/*
 * stk_helper response struct.  This is sent via a message from the helper
 * to the message thread in response to a acsapi request.  This struct
 * MUST fit in a sam_message_t.param area.
 */
typedef struct {
	robo_event_t		*event;		/* address of request event */
	int			api_status;	/* from the call */
	STATUS			acs_status;	/* acs_response status */
	ACS_RESPONSE_TYPE	type;		/* type from scs_response */
	COMMAND 		acs_command;	/* what command */
}stk_resp_headr_t;

typedef struct stk_resp_acs {
	stk_resp_headr_t hdr;
	union
	{
		ACS_DISMOUNT_RESPONSE		dismount;
		ACS_MOUNT_RESPONSE		mount;
		ACS_IDLE_RESPONSE		idle;
		ACS_START_RESPONSE		start;
		SAM_ACS_EJECT_RESPONSE		eject;
		SAM_ACS_QUERY_VOL_RESPONSE	query;
		SAM_ACS_QUERY_DRV_RESPONSE	q_drive;
		SAM_ACS_QUERY_MNT_RESPONSE	q_mount;
	}data;
}stk_resp_acs_t;

	/*
	 * It appears that the api wans to retun 4096 bytes all the
	 * time.  Make it so.
	 */
typedef struct full_stk_resp_acs {
	stk_resp_headr_t hdr;
	union
	{
		ACS_DISMOUNT_RESPONSE	dismount;
		ACS_MOUNT_RESPONSE	mount;
		ACS_IDLE_RESPONSE	idle;
		ACS_START_RESPONSE	start;
		ACS_EJECT_RESPONSE	eject;
		ACS_QUERY_VOL_RESPONSE	query;
		ACS_QUERY_DRV_RESPONSE	q_drive;
		ACS_QUERY_MNT_RESPONSE	q_mount;
		char			really_dumb[4096];
	}data;
}full_stk_resp_acs_t;

/* Function Prototypes */

int check_requests(library_t *);
int clear_drive(drive_state_t *);
int drive_is_idle(drive_state_t *);
int exchange_media(library_t *, drive_state_t *, int);
int init_elements(library_t *);
int initialize(library_t *, dev_ptr_tbl_t *);
int re_init_library(library_t *);
int is_cleaning(void *);
int is_flip_requested(library_t *, drive_state_t *);
int is_requested(library_t *, drive_state_t *);
int label_request(library_t *, robo_event_t *);
int mount_request(library_t *, robo_event_t *);
int load_media(library_t *, drive_state_t *, struct CatalogEntry *ce, int *);
int force_media(library_t *, drive_state_t *, int *);
int dismount_media(library_t *, drive_state_t *, int *);
int view_media(library_t *, struct CatalogEntry *ce, int *);
int onestep_eject(library_t *, struct CatalogEntry *ce, int *);
enum sam_scsi_action process_scsi_error(dev_ent_t *, sam_scsi_err_t *, int);
int query_drive(library_t *, drive_state_t *, int *, int *);
int set_driver_idle(drive_state_t *, int);
int spin_drive(drive_state_t *, const int, const int);
int start_audit(library_t *library, robo_event_t *event, const int);
int todo_request(library_t *, robo_event_t *);
int wait_drive_ready(drive_state_t *);

uint_t element_address(library_t *, const uint_t);
uint_t slot_number(library_t *, const uint_t);

char *sam_acs_status(STATUS);

void *drive_thread(void *);
void *manage_list(void *);
void *monitor_msg(void *);
void *transport_thread(void *);
void add_to_cat_req(library_t *, robo_event_t *);
void add_to_end(library_t *, robo_event_t *);
void clear_driver_idle(drive_state_t *, int);
void disp_of_event(library_t *, robo_event_t *, int);
void down_drive(drive_state_t *, int);
void down_library(library_t *, int);
void export_media(library_t *, robo_event_t *);
void historian_resp(library_t *, robo_event_t *);
void free_allocated(library_t *);
void load_unavail_request(library_t *, robo_event_t *);
void state_request(library_t *, robo_event_t *);
void tapealert_solicit(library_t *, robo_event_t *);
void sef_solicit(library_t *, robo_event_t *);
void unload_all_drives(library_t *);
void unload_request(library_t *, robo_event_t *);

drive_state_t *search_drives(library_t *, uint_t);
drive_state_t *find_idle_drive(library_t *);
drive_state_t *find_empty_drive(drive_state_t *);
drive_state_t *query_all_drives(drive_state_t *);
drive_state_t *query_mnt_status(drive_state_t *);

robo_event_t *get_free_event(library_t *);


#endif /* _STK_H */
