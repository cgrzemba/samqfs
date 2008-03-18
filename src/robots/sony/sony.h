/*
 * sony.h
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

/*
 * $Revision: 1.20 $
 */

#if !defined(_STK_H)
#define	_STK_H

#include "aml/device.h"
#include "aml/preview.h"
#include "aml/historian.h"
#include "../lib/librobots.h"
#include "../common/common.h"
#include "psc.h"

#define	SONY_MAIN_THREADS	2	/* number of main thread */
#define	SONY_MSG_THREAD		0	/* index of the message thread */
#define	SONY_WORK_THREAD	1	/* index of the worklist thread */
#define	REQUEST_NOT_COMPLETE	-100	/* used for request completion */

typedef struct _PSCCASSINFO PscCassInfo_t;
typedef struct _PSCBININFO  PscBinInfo_t;

/* macros */

/* return a unsigned int pointer to a DRIVEID */
#define	U_ID(d) (uint_t *)(&(d))

/* Structures */

/*
 * Layout and use of the robot private area.  Since the Sony API is not
 * thread safe, a helper process is used to issue the command to the
 * API.  Since a fork blocks while there is outstanding system calls, the
 * helper must be started once and passed messages.  The private area is
 * used for the messages and control.
 */

enum sony_priv_type {
	SONY_PRIV_NORMAL = 0,		/* there is a message */
	SONY_PRIV_VOID = 0xff		/* no message */
};

typedef struct sony_priv_mess {
	mutex_t	mutex;			/* for the whole thing */
	cond_t	cond_i;			/* for the issuers */
	cond_t	cond_r;			/* for the receivers */
	enum sony_priv_type mtype;	/* the message type */
	char	message[512];		/* the message */
} sony_priv_mess_t;

/*
 * Tables and structurs internal to the stk library driver.
 * For all of these structs, the main mutex is used for locking
 * all fields except list management and catalog access.  For those
 * fields use the mutex accociated with the fields.
 */

/* drive_state - one for each drive in a library */
typedef struct drive_state_s {
	COMMON_DRIVE_STATE
	PscBinInfo_t	*PscBinInfo;
	char		*BinNoChar;
} drive_state_t;

/* xport_state.  Information about the state of a transport element */
typedef struct xport_state_s {
	COMMON_XPORT_STATE
	int	sequence;
} xport_state_t;

/* import export state */
typedef struct iport_state_s {
	COMMON_IPORT_STATE
} iport_state_t;

/* library_t. Information about the state of the library itself */
typedef struct library_s {
	COMMON_LIBRARY_STATE
	sony_priv_mess_t *help_msg;
	char	*sony_userid;
	char	*sony_servername;
	pid_t	helper_pid;
	int	inc_free_running;
} library_t;

/*
 * sony_information_t - information sent to the transport for the helper
 * routine.
 */
typedef struct sony_information {
	PscCassInfo_t	*PscCassInfo;
	PscBinInfo_t	*PscBinInfo;
	char		*BinNoChar;
} sony_information_t;

/*
 * sony_helper response struct.  This is sent via a message from the helper
 * to the message thread in response to an api request.  This struct
 * MUST fit in a sam_message_t.param area.
 */
typedef struct {
	robo_event_t	*event;		/* address of request event */
	int		api_status;	/* from the call */
	int		d_errno;	/* d_errno if api call fails */
	int		sequence;
	union
	{
		PscCassInfo_t	CassInfo;
		PscBinInfo_t	BinInfo;
	}data;
} sony_resp_api_t;

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
enum sam_scsi_action process_scsi_error(dev_ent_t *, sam_scsi_err_t *, int);
int query_drive(library_t *, drive_state_t *, int *, int *);
int set_driver_idle(drive_state_t *, int);
int spin_drive(drive_state_t *, const int, const int);
int start_audit(library_t *library, robo_event_t *event, const int);
int todo_request(library_t *, robo_event_t *);
int wait_drive_ready(drive_state_t *);

uint_t element_address(library_t *, const uint_t);
uint_t slot_number(library_t *, const uint_t);

char *sam_acs_status(int);

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

boolean_t drive_is_local(library_t *, drive_state_t *);

robo_event_t *get_free_event(library_t *);


#endif /* _SONY_H */
