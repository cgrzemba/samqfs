/*
 * generic.h
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

/*
 * $Revision: 1.26 $
 */

#ifndef _GENERIC_H
#define	_GENERIC_H

#include "aml/device.h"
#include "aml/preview.h"
#include "aml/scsi.h"
#include "aml/mode_sense.h"
#include "aml/historian.h"
#include "../lib/librobots.h"
#include "../common/common.h"

#if !SAM_OPEN_SOURCE
#include "aci.h"
#endif

#include "api_errors.h"

#define	GENERIC_MAIN_THREADS	2	/* number of main thread */
#define	GENERIC_MSG_THREAD	0	/* index of the message thread */
#define	GENERIC_WORK_THREAD	1	/* index of the worklist thread */
#define	REQUEST_NOT_COMPLETE   -100	/* used for request completion */

typedef struct aci_vol_desc aci_vol_desc_t;
typedef struct aci_drive_entry aci_drive_entry_t;
typedef struct aci_client_entry aci_client_entry_t;
typedef struct aci_req_entry aci_req_entry_t;
typedef struct aci_sideinfo aci_sideinfo_t;
typedef enum aci_media aci_media_t;


/*
 * The transport thread will return the following errcode if there was an
 * error loading media but it was able to clear the transport after the
 * error.
 */

#define	RECOVERED_MEDIA_MOVE    		-90

/*
 * The transport thread will return INCOMPATIBLE_MEDIA_MOVE
 * if the media and the drive are incompatible
 */
#define	INCOMPATIBLE_MEDIA_MOVE			-80

/* following used with wait_for_export() */
#define	FULL    1
#define	EMPTY   0

/* following are used with the move_media and exchange_media functions */
typedef union move_flags_u {
	struct {
		uint_t
#if	defined(_BIT_FIELDS_HTOL)
		noerror:1,

		unused:31;
#else				/* defined(_BIT_FIELDS_HTOL) */
		unused:31,

		noerror:1;
#endif				/* defined(_BIT_FIELDS_HTOL) */
	}		b;
	uint_t	   bits;
}		move_flags_t;

/* Mode sense page codes */

#define	MODE_SENSE_ALL_PAGES    0x3F
#define	MODE_SENSE_ELEMENT_PAGE 0x1D	/* Element Address Assignment  */
#define	MODE_SENSE_XPORT_PAGE   0x1E	/* Transport Geometry Parameters */
#define	MODE_SENSE_DEVICE_PAGE  0x1F	/* Device Capabilities  */

/* Element types */

#define	ALL_ELEMENTS	    0
#define	TRANSPORT_ELEMENT	1
#define	STORAGE_ELEMENT	 2
#define	IMPORT_EXPORT_ELEMENT   3
#define	DATA_TRANSFER_ELEMENT   4

/* Additional sense code qualifier */

#define	ERR_ASCQ_BECOMMING  0x1	/* Becomming ready */
#define	ERR_ASCQ_REZERO	0x2	/* Need to rezero unit */
#define	ERR_ASCQ_MECH	0x3	/* Mechanical Hindrance */

/* Some upper limits */
#define	MAX_STORE_STATUS    512	/* maximum number of elements for */
	/* a read_element_status for storage */
/* Structures */

/*
 * Tables and structurs internal to the generic
 * library driver.  For all of these structs,
 * the main mutex is used for locking all fields except
 * list management and catalog access.  For those fields, use the mutex
 * accociated with the fields.
 */

/* drive_state - One for each drive in a library. */

typedef struct drive_state_s {
	COMMON_DRIVE_STATE
	aci_drive_entry_t *aci_drive_entry;
/* Not used by grau */
	ushort_t	 element; /* element address of this drive */
/* end - Not used */
	ushort_t	 media_element;	/* element address of media */
}		drive_state_t;

/* xport_state.  Information about the state of a transport element. */

typedef struct xport_state_s {
	COMMON_XPORT_STATE
	int		sequence;	/* aci sequence number */
/* Not used by grau */
	ushort_t	 element; /* element adderss of this xport */
	ushort_t	 media_element;	/* element address of media */
/* end - Not used */
}		xport_state_t;

/* iport_state. Information on the import/export element. */

typedef struct iport_state_s {
	COMMON_IPORT_STATE
/* Not used by grau */
	ushort_t element;	/* element address  */
	ushort_t	 media_element;	/* element address of media */
/* end - Not used */
}		iport_state_t;

/* element range information. */

typedef struct library_range_s {
	ushort_t	 default_transport;
	ushort_t	 transport_lower;
	ushort_t	 transport_upper;
	ushort_t	 transport_count;
	ushort_t	 drives_lower;
	ushort_t	 drives_upper;
	ushort_t	 drives_count;
	ushort_t	 ie_lower;
	ushort_t	 ie_upper;
	ushort_t	 ie_count;
	ushort_t	 storage_lower;
	ushort_t	 storage_upper;
	ushort_t	 storage_count;
}		library_range_t;


/* Structures */

/*
 * Tables and structurs internal to the API
 * library driver.  For all of these structs,
 * the main mutex is used for locking all fields except
 * list management and catalog access.  For those fields, use the mutex
 * accociated with the fields.
 */

/*
 * Layout and use of the robot private area.  Since the generic APIs are not
 * thread safe, a helper process is used to issue the command to the
 * API.  Since a fork blocks while there is outstanding system calls, the
 * helper must be started once and passed messages.  The private area is
 * used for the messages and control.
 */

enum api_priv_type {
	API_PRIV_NORMAL = 0,	/* there is a message */
	API_PRIV_VOID = 0xff	/* no message */
};

typedef struct api_priv_mess {
	mutex_t	 mutex;	/* for the whole thing */
	cond_t	  cond_i;	/* for the issuers */
	cond_t	  cond_r;	/* for the receivers */
	enum api_priv_type mtype;	/* the message type */
	char	    message[512];	/* the message */
}		api_priv_mess_t;

/* ACI - information - used in transport requests */

typedef struct aci_information {
	aci_drive_entry_t *aci_drive_entry;
	aci_vol_desc_t *aci_vol_desc;
}		aci_information_t;

/*
 * api_helper response struct.  This is sent via a message from the helper
 * to the message thread in response to a api request.  This struct
 * MUST fit in a sam_message_t.param area.
 */
typedef struct {
	robo_event_t   *event;	/* address of request event */
	int		api_status;	/* return from api call */
	int		d_errno; /* d_errno if api call failed */
	int		sequence;	/* sequence number */
#if !SAM_OPEN_SOURCE
	union {
		aci_vol_desc_t  vol_des;
		aci_drive_entry_t drive_ent;
		aci_req_entry_t req_ent;
	}		data;
#endif
}		api_resp_api_t;


/*
 * library_t. Information about the state of the library itself.
 */

typedef struct library_s {
	COMMON_LIBRARY_STATE
	api_priv_mess_t *help_msg;	/* address of helper message area */
	/* already adjusted for shm address */
	pid_t	   helper_pid;	/* fulltime helper */
/* API specific stuff */
	char	   *api_client;	/* this client name */
	char	   *api_server;	/* DAS server hostname */
	char	   *api_import;	/* DAS import name */
	char	   *api_export;	/* DAS export name */
	char	   *api_helper_name;	/* Name of the helper program */
/* Not used by grau - inc_free_running IS */
	library_range_t range;	/* element range information */
	robot_ms_page1f_t *page1f;	/* pointer to mode sense page1f */
	sam_scsi_err_t *scsi_err_tab;	/* pointer to the sense code table */
	int		open_fd; /* file decsriptor of the library */
/* end - Not used */
	int		inc_free_running;
}		library_t;



/* Function Prototypes */

int		aci_load_media(library_t *,
		    drive_state_t *, struct CatalogEntry *, int *);
int		aci_force_media(library_t *, drive_state_t *, int *);
int		aci_dismount_media(library_t *, drive_state_t *, int *);
int		aci_view_media(library_t *,
		    struct CatalogEntry *, int *, int *);
int		aci_drive_access(library_t *, drive_state_t *, int *);
int		aci_getside(library_t *, char *string, char *, int *);
int		sam2aci_type(media_t);
int		FindFreeSlot(library_t *library);

int		check_requests(library_t *);
int		clear_drive(drive_state_t *);
int		clear_transport(library_t *, xport_state_t *);
int		drive_is_idle(drive_state_t *);
int		exchange_media(library_t *, const uint_t,
		    const uint_t, const uint_t,
		    const int, const int, const move_flags_t);
int		flip_and_scan(int, drive_state_t *);
int		init_elements(library_t *);
int		initialize(library_t *, dev_ptr_tbl_t *);
int		re_init_library(library_t *);
int		is_barcode(void *);
int		is_cleaning(void *);
int		is_flip_requested(library_t *, drive_state_t *);
int		is_requested(library_t *, drive_state_t *);
int		is_storage(library_t *, const uint_t);
int		label_request(library_t *, robo_event_t *);
int		mount_request(library_t *, robo_event_t *);
int		move_media(library_t *, const uint_t, const uint_t,
		    const uint_t, const int, const move_flags_t);
enum sam_scsi_action process_scsi_error(dev_ent_t *, sam_scsi_err_t *, int);
int
read_element_status(library_t *, const int, const int, const int,
		    void *, const int);
int		rotate_mailbox(library_t *library, const int);
int		set_driver_idle(drive_state_t *, int);
int		spin_drive(drive_state_t *, const int, const int);
int		start_audit(library_t *library, robo_event_t *event, const int);
int		status_element_range(library_t *, const int,
		    const uint_t, const uint_t);
int		todo_request(library_t *, robo_event_t *);
int		update_element_status(library_t *, const int);
int		wait_drive_ready(drive_state_t *);
int		wait_library_ready(library_t *);

uint_t	   element_address(library_t *, const uint_t);
uint_t	   slot_number(library_t *, const uint_t);

enum sam_scsi_action
process_res_codes(dev_ent_t *un, uchar_t, uchar_t,
		uchar_t, sam_scsi_err_t *);

void	   *drive_thread(void *);
void	   *import_thread(void *);
void	   *manage_list(void *);
void	   *move_request(void *);
void	   *monitor_msg(void *);
void	   *transport_thread(void *);
void	    add_to_cat_req(library_t *, robo_event_t *);
void	    add_to_end(library_t *, robo_event_t *);
void	    clear_busy_bit(robo_event_t *, drive_state_t *, const uint_t);
void	    clear_driver_idle(drive_state_t *, int);
void	    disp_of_event(library_t *, robo_event_t *, int);
void	    down_drive(drive_state_t *, int);
void	    stop_use_of_drive(drive_state_t *);
void	    down_library(library_t *, int);
void	    export_media(void *, robo_event_t *, uint_t);
void	    export_request(library_t *, robo_event_t *);
void	    historian_resp(library_t *, robo_event_t *);
void	    free_allocated(library_t *);
void	    import_request(library_t *, robo_event_t *);
void	    init_drive(drive_state_t *);
void	    load_unavail_request(library_t *, robo_event_t *);
void	    schedule_export(library_t *, uint_t);
void	    scsi_unit_attention(library_t *, const int);
void	    set_media_default(media_t *);
void	    set_speclog_panel(library_t *, const int);
void	    state_request(library_t *, robo_event_t *);
void	    tapealert_solicit(library_t *, robo_event_t *);
void	    sef_solicit(library_t *, robo_event_t *);
void	    unload_all_drives(library_t *);
void	    unload_request(library_t *, robo_event_t *);

drive_state_t  *search_drives(library_t *, uint_t);
drive_state_t  *find_idle_drive(library_t *);
drive_state_t  *find_empty_drive(drive_state_t *);

boolean_t	drive_is_local(library_t *, drive_state_t *);

robo_event_t   *get_free_event(library_t *);

#endif	/* _GENERIC_H */
