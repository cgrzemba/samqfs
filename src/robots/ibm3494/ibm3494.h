/*
 * ibm3494.h
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
 * $Revision: 1.22 $
 */

#if !defined(_IBM3494_H)
#define	_IBM3494_H

#include "aml/device.h"
#include "aml/preview.h"
#include "aml/historian.h"
#include "../lib/librobots.h"
#include "../common/common.h"
#include "mtlibio.h"

#define	IBM_MAIN_THREADS	3	 /* number of main thread */
#define	IBM_MSG_THREAD		0	 /* index of the message thread */
#define	IBM_WORK_THREAD		1	 /* index of the worklist thread */
#define	IBM_DELAY_THREAD	2	 /* index of delay response thread */
#define	REQUEST_NOT_COMPLETE   -100	 /* used for request completion */

#define	IS_3592_MEDIA(attr) ((((attr) >> 4) & 0x3) == 0x02)
#define	IS_3590_MEDIA(attr) (! IS_3592_MEDIA(attr))

/* Additional sense code qualifier */

#define	ERR_ASCQ_BECOMMING  0x1	 /* Becomming ready */
#define	ERR_ASCQ_REZERO	0x2	 /* Need to rezero unit */
#define	ERR_ASCQ_MECH	0x3	 /* Mechanical Hindrance */

/* Categories */

#define	INSERT_CATEGORY	0xff00		/* inserted */
#define	EJECT_CATEGORY	0xff10		/* eject */
#define	B_EJECT_CATEGORY    0xff11	/* Bulk eject */
#define	SERVICE_CATEGORY    0xfff9	/* CE cartridge */
#define	MAN_EJECTED_CAT	0xfffa		/* Manually ejected */
#define	PURGE_VOL_CATEGORY  0xfffb	/* Remove it from database */
#define	CLEAN_CATEGORY	0xfffe		/* drive cleaning cartridge */
#define	NORMAL_CATEGORY	0x0004

#define	BS1 MT_LIB_POS | MT_LIB_MMOS | MT_LIB_DO
#define	BS2 MT_LIB_SEIO | MT_LIB_OL | MT_LIB_IR
#define	BS3 MT_LIB_CK1 | MT_LIB_EA | MT_LIB_MMMOS
#define	BAD_STATE (BS1 | BS2 | BS3)

/* Handy constant for error messages */
#define	LIBEQ library->un->eq

/* Define human readable messages for completion codes */
#if defined(CC_CODES)
char *cc_codes[] = {
	"Complete - No error",			/* MTCC_COMPLETE	  */
	"Complete - Vision not operational",    /* MTCC_COMPLETE_VISION   */
	"Complete - VOLSER not readable",	/* MTCC_COMPLETE_NOTREAD  */
	"Complete - Cat. assign. not changed",  /* MTCC_COMPLETE_CAT	  */
	"Cancelled - Program request",		/* MTCC_CANCEL_PROGREQ    */
	"Cancelled - Order sequence",		/* MTCC_CANCEL_ORDERSEQ   */
	"Cancelled - Manual mode",		/* MTCC_CANCEL_MANMODE    */
	"Failed - Hardware",			/* MTCC_FAILED_HARDWARE   */
	"Failed - Vision not operational",	/* MTCC_FAILED_VISION	  */
	"Failed - VOLSER not readable",		/* MTCC_FAILED_NOTREAD    */
	"Failed - Volume inaccessible",		/* MTCC_FAILED_INACC	  */
	"Failed - Volume misplaced",		/* MTCC_FAILED_MISPLACED  */
	"Failed - Category empty",		/* MTCC_FAILED_CATEMPTY   */
	"Failed - Volume manually ejected",	/* MTCC_FAILED_MANEJECT   */
	"Failed - Vol no longer in inventory",  /* MTCC_FAILED_INVENTORY  */
	"Failed - Device no longer available",  /* MTCC_FAILED_NOTAVAIL   */
	"Failed - Permanent Load failure",	/* MTCC_FAILED_LOADFAIL   */
	"Failed - Load Failure-Cart damaged",   /* MTCC_FAILED_DAMAGED    */
	"Complete - Demount",			/* MTCC_COMPLETE_DEMOUNT  */
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"LMCP is not configured",		/* MTCC_NO_LMCP	   */
	"Device is not command-port LMCP",	/* MTCC_NOT_CMDPORT_LMCP  */
	"Device is not configured",		/* MTCC_NO_DEV	    */
	"Device is not in library",		/* MTCC_NO_DEVLIB   */
	"Not enough memory",			/* MTCC_NO_MEM	    */
	"Device is in use",			/* MTCC_DEV_INUSE   */
	"I/O failed",				/* MTCC_IO_FAILED   */
	"Device is invalid",			/* MTCC_DEV_INVALID */
	"Device not notification-port LMCP",    /* MTCC_NOT_NTFPORT_LMCP  */
	"Invalid subcmd parameter",		/* MTCC_INVALID_SUBCMD    */
	"No library device is configured",	/* MTCC_LIB_NOT_CONFIG    */
	"Internal error",			/* MTCC_INTERNAL_ERROR    */
	"Invalid cancel type",			/* MTCC_INVALID_CANCELTYP */
	"Not an LMCP device",			/* MTCC_NOT_LMCP	  */
	"Library offline to host",		/* MTCC_LIB_OFFLINE	  */
	"Volume not unloaded from drive",	/* MTCC_DRIVE_UNLOAD	*/
};
#else
extern char *cc_codes[MTCC_DRIVE_UNLOAD + 1];
#endif
#define	HIGH_CC MTCC_DRIVE_UNLOAD

/* Set up some typedefs for the IBM supplied structures */

typedef struct mtlewarg IBM_wait_arg_t;
typedef struct mtlewret IBM_wait_ret_t;
typedef struct msg_info IBM_info_t;
typedef struct mtlmarg  IBM_mount_t;
typedef struct mtldarg  IBM_dismount_t;
typedef struct mtlqarg  IBM_query_t;
typedef struct mtlqret  IBM_query_ret_t;
typedef struct lib_query_info IBM_query_info_t;
typedef struct mtlsvcarg IBM_set_category_t;

/* Returned errors from media changer requests */

typedef enum {
	MC_REQ_OK = 0,	/* no error */
	MC_REQ_DD,	/* failed, down drive */
	MC_REQ_TR,	/* failed, toss request */
	MC_REQ_DC,	/* failed, delete catalog entry */
	MC_REQ_FL	/* failed, just failed */
}req_comp_t;

/* Structures */

	/*
	 * Used for passing information to the "transport" thread.  The
	 * ret_data field, if not NULL, was malloced by the transport
	 * thread and must be freed by the caller.
	 */
typedef struct {
	int   drive_id;		/* drive id if needed */
	int   sub_cmd;		/* Sub-command if needed */
	int   seqno;		/* Sequence number if needed */
	ushort_t targ_cat;	/* target category */
	ushort_t src_cat;	/* source category */
	void  *ret_data;	/* retruned info */
	char  volser[8];	/* volser if needed */
}ibm_req_info_t;

/* Double ended link list of the events waiting for completion */

typedef struct  delay_list_ent {
	uint_t   req_id;	/* as assigned by the lcmpd */
	robo_event_t *event;	/* event to signal */
	struct delay_list_ent *last;
	struct delay_list_ent *next;
} delay_list_ent_t;

/*
 * Tables and structurs internal to the ibm library driver.
 * For all of these structs, the main mutex is used for locking
 * all fields except list management and catalog access.  For those
 * fields use the mutex accociated with the fields.
 */

/* drive_state - One for each drive in a library. */
typedef struct drive_state_s {
	COMMON_DRIVE_STATE
	int    drive_id;
}drive_state_t;

/* xport_state.  Information about the state of a transport element. */
typedef struct xport_state_s {
	COMMON_XPORT_STATE
}xport_state_t;

/* import export state */
typedef struct iport_state_s {
	COMMON_IPORT_STATE
}iport_state_t;

/* library_t. Information about the state of the library itself. */
typedef struct library_s {
	COMMON_LIBRARY_STATE
	int    open_fd;			/* open "fd" for lmpc daemon */
	int    stor_count;		/* storage count from library */
	int    inc_free_running;
	mutex_t dlist_mutex;
	delay_list_ent_t *delay_list;
	ushort_t sam_category;		 /* category used by sam */
	union {
    struct {
	ushort_t
#if	defined(_BIT_FIELDS_HTOL)
	shared: 1,			/* library is shared */
	unused: 15;
#else					/* defined(_BIT_FIELDS_HTOL) */
	unused: 15,
	shared: 1;			/* library is shared */
#endif  				/* defined(_BIT_FIELDS_HTOL) */
	}b;
    ushort_t bits;
	}options;
}library_t;

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
enum sam_scsi_action process_scsi_error(dev_ent_t *, sam_scsi_err_t *, int);
int set_driver_idle(drive_state_t *, int);
int spin_drive(drive_state_t *, const int, const int);
int start_audit(library_t *library, robo_event_t *event, const int);
int todo_request(library_t *, robo_event_t *);
int wait_drive_ready(drive_state_t *);

uint_t element_address(library_t *, const uint_t);
uint_t slot_number(library_t *, const uint_t);

void *delay_resp(void *);
void *drive_thread(void *);
void *manage_list(void *);
void *monitor_msg(void *);
void *transport_thread(void *);
void *reconcile_catalog(void *);
void add_to_cat_req(library_t *, robo_event_t *);
void add_to_end(library_t *, robo_event_t *);
void clear_driver_idle(drive_state_t *, int);
void disp_of_event(library_t *, robo_event_t *, int);
void down_drive(drive_state_t *, int);
void down_library(library_t *, int);
void export_media(library_t *, robo_event_t *);
void free_allocated(library_t *);
void historian_resp(library_t *, robo_event_t *);
void load_unavail_request(library_t *, robo_event_t *);
void schedule_export(library_t *, uint_t);
void state_request(library_t *, robo_event_t *);
void tapealert_solicit(library_t *, robo_event_t *);
void sef_solicit(library_t *, robo_event_t *);
void unload_all_drives(library_t *);
void unload_request(library_t *, robo_event_t *);
void wait_library_ready(library_t *);

drive_state_t *search_drives(library_t *, uint_t);
drive_state_t *find_idle_drive(library_t *);
drive_state_t *find_empty_drive(drive_state_t *);

boolean_t drive_is_local(library_t *, drive_state_t *);

req_comp_t query_drive(library_t *, drive_state_t *, int *drive_status,
    int *drive_state);
req_comp_t force_media(library_t *, drive_state_t *);
req_comp_t load_media(library_t *, drive_state_t *, struct CatalogEntry *,
    ushort_t);
req_comp_t dismount_media(library_t *, drive_state_t *);
req_comp_t view_media(library_t *, char *, void **);
req_comp_t view_media_category(library_t *, int, void **, ushort_t);
req_comp_t set_media_category(library_t *, char *, ushort_t, ushort_t);
req_comp_t query_library(library_t *, int, int, void **, ushort_t);

robo_event_t *get_free_event(library_t *);


#endif /* _IBM3494_H */
