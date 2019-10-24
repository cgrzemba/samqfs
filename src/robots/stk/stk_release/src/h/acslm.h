/* P4_Id - $Id: //depot/acsls_dev/h/acslm.h#2 $ */

#ifndef _ACSLM_
#define _ACSLM_         
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header is intended for use within the scope of the
 *      ACSLM process and any child processes needing such information.
 *
 * Modified by:
 *
 *      H. L. Freeman IV 27-Oct-1988.    Original.
 *      H. I. Grapek     26-Apr-1990     added set clean support
 *                                       (lm_drive_table, auto_clean)
 *      H. I. Grapek     30-Apr-90       added request, resaponse struct
 *                                       entries for set_clean, added
 *                                       module definitions for lm_chk_drvtbl,
 *                                       lm_mk_drvtbl, and lm_set_drvtbl
 *      H. I. Grapek     03-May-1990     added venter def for joseph.
 *      J. W. Montgomery 04-May-1990     added mount_scratch.
 *      D. L. Trachy     23-May-1990     added lock commands
 *      J. W. Montgomery 31-May-1990     added lm_mount_count.
 *      H. I. Grapek     09-Jun-1990     added ext_eject def for joseph.
 *      H. I. Grapek     26-Jun-1990     added audit trail support
 *      H. I. Grapek     27-Jun-1990     Added lm_rp_table
 *      H. I. Grapek     15-Jul-1990     added set cap support
 *      H. I. Grapek     03-Aug-1990     Removed response opts from rp_table.
 *      H. I. Grapek     24-Aug-1990     Added LM_RECOVERY_FLAG
 *      H. I. Grapek     01-Nov-1990     Added vary_in_progress flag to 
 *                                       LM_OFFLINE_VARY struct.
 *      H. I. Grapek     14-July-1991    Added lm_cap_table definition for 
 *                                       data and routines.  Added VERSION 
 *                                       to lm_req_table.
 *      H. I. Grapek     20-Jul-1991     Added LM_MESSAGES (like to CLM mgs)
 *                       27-Sep-1991     Changes per code review.
 *                                       Added defs for queue magic numbers
 *                                       code review prompted cleanup.
 *                                       added typedef for ulong.
 *      H. I. Grapek     05-Nov-1991     modified LM_OFFLINE_VARY struct.
 *      Alec Sharp       15-Jun-1992     Added lm_cvt_v2_v3, lm_cvt_v3_v2,
 *                                       lm_minimum_version
 *      Alec Sharp       15-Aug-1992     Added set_owner_request and
 *                                       set_owner_response.
 *      Alec Sharp       27-Jul-1993     Removed lm_minimum_version.
 *	David Farmer	 20-Aug-1993	 Added ifdef sun around typedef ulong
 *	J. Borzuchowski	 21-Aug-1993	 R5.0 Mixed Media-- Added procedure
 *					 definitions for new routines
 *					 lm_cvt_v3_v4 and lm_cvt_v4_v3.
 *      Alec Sharp	 01-Dec-1993	 Removed lm_message
 *	J. Borzuchowski  10-Mar-1994	 R5.0 BR#174-- Added the request ID as 
 *					 a parameter to routine lm_rp_trail.
 *	D. A. Myers	 12-Oct-1994	 Porting changes
 *	E. A. Alongi	 05-Sep-1995	 Added lm_query_count; for persistent
 *					 query process.
 *	H. L. Freeman	 03-Apr-1997	 R5.2 - Switch LMU support.
 *	N. Afsar     	 02-Feb-1998	 R5.3 - Support for MOVE command. 
 *      George Noble     16-Nov-2000     R6.0 - Support for RCVY (Cartridge
 *                                       Recovery) request/response. 
 *      Scott Siao       17-Oct-2001     Added support for register, 
 *                                       unregister, and check_registration commands
 *      Scott Siao       12-Nov-2001     Added support for display 
 *      Scott Siao       06-Feb-2002     Added support for mount_pinfo (virtual)
 *
 */

/*
 *      Header Files:
 */

#ifndef _CL_QM_DEFS_
#include "cl_qm_defs.h"
#endif

#ifndef _LH_DEFS_
#include "lh_defs.h"
#endif

#ifndef _LM_STRUCTS_
#include "lm_structs.h"
#endif

#ifndef _V2_STRUCTS_
#include "v2_structs.h"
#endif

#ifndef _V3_STRUCTS_
#include "v3_structs.h"
#endif

/*
 *      Defines, Typedefs and Structure Definitions:
 */

#if defined (sun) && !defined (SOLARIS)
/* define an unsigned long as a ulong */
typedef unsigned long ulong;
#endif

/* ACSLM vary offline table definition - one for each LSM configured */
typedef struct {
    LSMID               lsm_id;                 /* current lsm */
    BOOLEAN             vary_in_progress;       /* True if automatic VARY happening */
    STATE               current_state;          /* state of the LSM */
    STATE               base_state;             /* state to return to from auto vary */
} LM_OFFLINE_VARY;

 /* acslm in memory clean drive table */
typedef struct {
    DRIVEID             drive_id;       /* drive identifier */
    BOOLEAN             flag;           /* true or false */
} LM_DRVTBL;

/* ACSLM in memory CAP table */
typedef struct {
    CAPID               cap_id;         
    BOOLEAN             auto_ent;       /* auto enter in progress on this cap? */
    unsigned long       msg_id;         /* intermediate request id */
} LM_CAPTBL;

typedef union {                                 /* request type union */
   REQUEST_HEADER       generic_request;
   AUDIT_REQUEST        audit_request;
   ENTER_REQUEST        enter_request;
   VENTER_REQUEST       venter_request;
   EJECT_REQUEST        eject_request;
   EXT_EJECT_REQUEST    ext_eject_request;
   VARY_REQUEST         vary_request;
   MOUNT_REQUEST        mount_request;
   MOUNT_SCRATCH_REQUEST mount_scratch_request;
   DISMOUNT_REQUEST     dismount_request;
   QUERY_REQUEST        query_request;
   CANCEL_REQUEST       cancel_request;
   START_REQUEST        start_request;
   IDLE_REQUEST         idle_request;
   SET_SCRATCH_REQUEST  set_scratch_request;
   DEFINE_POOL_REQUEST  define_pool_request;
   DELETE_POOL_REQUEST  delete_pool_request;
   LH_MESSAGE           lh_request;
   SET_CLEAN_REQUEST    set_clean_request;
   LOCK_REQUEST         lock_request;
   UNLOCK_REQUEST       unlock_request;
   CLEAR_LOCK_REQUEST   clear_lock_request;
   QUERY_LOCK_REQUEST   query_lock_request;
   SET_CAP_REQUEST      set_cap_request;
   SET_OWNER_REQUEST    set_owner_request;
   SWITCH_REQUEST       switch_request;
   MOVE_REQUEST         move_request;
   RCVY_REQUEST         rcvy_request;
   REGISTER_REQUEST     register_request;
   UNREGISTER_REQUEST   unregister_request;
   CHECK_REGISTRATION_REQUEST   check_registration_request;
   DISPLAY_REQUEST      display_request;
   MOUNT_PINFO_REQUEST  mount_pinfo_request;
} REQUEST_TYPE;

typedef struct {                                /* response header */
   REQUEST_HEADER       request_header;
   RESPONSE_STATUS      response_status;
} RESPONSE_HEADER;

typedef union {                                 /* response type */
   RESPONSE_HEADER      generic_response;
   ACKNOWLEDGE_RESPONSE acknowledge_response;
   AUDIT_RESPONSE       audit_response;
   ENTER_RESPONSE       enter_response;
   EJECT_RESPONSE       eject_response;
   VARY_RESPONSE        vary_response;
   MOUNT_RESPONSE       mount_response;
   MOUNT_SCRATCH_RESPONSE mount_scratch_response;
   DISMOUNT_RESPONSE    dismount_response;
   QUERY_RESPONSE       query_response;
   CANCEL_RESPONSE      cancel_response;
   START_RESPONSE       start_response;
   IDLE_RESPONSE        idle_response;
   SET_SCRATCH_RESPONSE set_scratch_response;
   DEFINE_POOL_RESPONSE define_pool_response;
   DELETE_POOL_RESPONSE delete_pool_response;
   SET_CLEAN_RESPONSE   set_clean_response;
   LOCK_RESPONSE        lock_response;
   UNLOCK_RESPONSE      unlock_response;
   CLEAR_LOCK_RESPONSE  clear_lock_response;
   QUERY_LOCK_RESPONSE  query_lock_response;
   SET_CAP_RESPONSE     set_cap_response;
   SET_OWNER_RESPONSE   set_owner_response;
   SWITCH_RESPONSE      switch_response;
   MOVE_RESPONSE        move_response;
   RCVY_RESPONSE        rcvy_response;
   REGISTER_RESPONSE    register_response;
   UNREGISTER_RESPONSE  unregister_response;
   CHECK_REGISTRATION_RESPONSE check_registration_response;
   DISPLAY_RESPONSE     display_response;
   MOUNT_PINFO_RESPONSE mount_pinfo_response;
} RESPONSE_TYPE;

typedef struct request_table {                  /* ACSLM request_table */
   long                 request_pid;    /* spawned PID of the entry */
   REQUEST_TYPE         *request_ptr;   /* ptr to the request_packet */
   unsigned long        byte_count;     /* byte count of message */
   STATUS               status;         /* status of the spawned process */
   STATUS               exit_status;    /* process exit status field */
   COMMAND              command;        /* request command */
   TYPE                 requestor_type; /* who was originator of request */
   char                 return_socket_name[SOCKET_NAME_SIZE];
   unsigned int         resource_count; /* resources needed to run */
   time_t               ts_que;         /* time just before being queued up */
   time_t               ts_write;       /* just before the ipc_write */
   time_t               ts_fmt;         /* just before the ipc_write to requestor */
   VERSION              version;        /* originating version */
   int                  pktCnt;        /* Number of response packets */
} LM_REQUEST_TABLE;

/* ACSLM error message size */
#define LM_ERROR_MSG_SIZE       256
#define LM_FILE_NAME_SIZE       14

/* ACSLM Global request process table */
typedef struct rp_tbl {                         /* ACSLM Request proc Table */
   COMMAND              command_value;  /* value of command */
   char                 process_filename[LM_FILE_NAME_SIZE];
   BOOLEAN              spawned;        /* TRUE if spawned by the rp_create */
   BOOLEAN              idle_prc;       /* TRUE if command is processed during
                                         * idle/idle pending state */
   BOOLEAN              recov_prc;      /* TRUE if command is processed during
                                         * recovery state */
   BOOLEAN              cancellable;    /* TRUE if command is cancellable */
   int                  resource_count; /* number of resources needed for command */
   unsigned char        req_msg_mask;   /* valid request message options mask */
   unsigned long        req_ext_mask;   /* valid request extended options mask */
   VERSION              version;        /* the version thich this command started */
} LM_RP_TBL;

/* ACSLM generic input/output buffers */
extern ALIGNED_BYTES acslm_input_buffer[MAX_MESSAGE_BLOCK];
extern ALIGNED_BYTES acslm_output_buffer[MAX_MESSAGE_BLOCK];

/* 
 * ACSLM constants for server system processing resource slots 
 * Maxuprc minus number of common system server routines always running 
 * see lm_init.c for maxuprc 
 */
extern  int             maxuprc;
#define MAX_REQUEST_PROCESS_SLOTS  maxuprc - 10

/* environment variable for altering MAX_REQUEST_PROCESS_SLOTS */
#define MAX_ACS_PROCESSES          "MAX_ACS_PROCESSES"

/* ACSLM constants for timeouts on waits and ipc read selects */
#define LM_SELECT_TIMEOUT          1
#define LM_SLEEP_TIMEOUT           15

/* ACSLM Request Queue Validation Interval(sec) */
#define LM_QAUDIT_INTERVAL         300

/* ACSLM constant for queue management usage */
#define LM_MAX_QUEUES              1

/* ACSLM usage for MESSAGE_HEADER message_options element */
#define LM_NO_OPTIONS              0
#define LM_FINAL_MSG               0x00

/* Constants used for acslm request table traversing */
#define LM_TRAVERSE_FWD            0
#define LM_TRAVERSE_REV            1
#define LM_TRAVERSE_FIRST          0
#define LM_TRAVERSE_NEXT           1
#define LM_TRAVERSE_LAST           2

/* ACSLM REQUEST PROCESS RESOURCE CONSTANTS */
#define LM_NO_RESOURCES            0
#define LM_MIN_RESOURCES           1

/* ACSLM Idle/Termination Flags */
#define LM_IDLE_FLAG               0
#define LM_IDLE_FORCE_FLAG         1
#define LM_TERMINATE_FLAG          2
#define LM_CANCEL_FLAG             3
#define LM_RECOVERY_FLAG           4

/* ACSLM default vary offline interval/window (sec) */
#define LM_VARY_OFFLINE_INTERVAL   (long)(2*60)

/* define for auto clean drive environment variable */
#define AUTO_CLEAN              "AUTO_CLEAN"

/* definitions for audit trail environment variable */
#define LM_RP_TRAIL             "LM_RP_TRAIL"


/* definitions for queue's pid values. (>0 is valid pid) */
#define LMP_NONE         0      /* proc complete, queue not deleted yet */
#define LMP_EMPTY       -1      /* queue slot empty, not processed yet */
#define LMP_PERSISTANT  -2      /* persistant process, special class. */


/* ACSLM EVENT LOG MESSAGES */
typedef enum {
    LMM_FIRST = 0,      /* illegal */
    LMM_BAD_ACSID,      /* ACS Identifier xx Invalid */
    LMM_BAD_STAT,       /* Unexpected status for command detected */
    LMM_BAD_EXECL,      /* Unexpected EXECL failure */ 
    LMM_BAD_FORK,       /* Unexpected FORK failure on request x */ 
    LMM_BAD_LSMID,      /* LSM Identifier Invalid */
    LMM_BAD_NDX,        /* Unexpected index detected */
    LMM_BAD_PACKET,     /* Bad packet detected */
    LMM_BAD_PORTID,     /* PORT Identifier Invalid */
    LMM_BAD_REQ_TBL,    /* Unexpected request table entry status detected */
    LMM_BAD_VARY,       /* Creation of VARY request failed */
    LMM_CAP_CLOSED,     /* LH_MSG_TYPE_CAP_CLOSED received for LSM Identifier */
    LMM_CAP_OPENED,     /* LH_MSG_TYPE_CAP_OPENED received for LSM Identifier */
    LMM_LMU_READY,      /* LH_MSG_TYPE_LMU_READY received for ACS Identifier */
    LMM_LSM_NOT_READY,  /* LH_MSG_TYPE_LSM_NOT_READY received for LSM Identifier */
    LMM_LSM_READY,      /* LH_MSG_TYPE_LSM_READY received for LSM Identifier */
    LMM_PORT_MSG,       /* LH_MSG_TYPE_PORT_OFFLINE received for PORT Identifier */ 
    LMM_DOOR_CLOSED,    /* ACCESS Door Closed */
    LMM_DOOR_OPENED,    /* ACCESS Door Opened */
    LMM_CAP_NOT_FOUND,  /* CAP not found in configuration */
    LMM_FAIL_IPC_OPEN,  /* Failed to allocate IPC Socket */
    LMM_INVAL_ADDRESS,  /* Invalid addressee detected */
    LMM_INVAL_SDIR,     /* Invalid search direction detected */
    LMM_INVALID_EXIT,   /* Invalid exit status returned */ 
    LMM_INVALID_MSG,    /* Invalid message type received from ACSLH */
    LMM_INVALID_MSGID,  /* Invalid Message Identifier detected */
    LMM_INVALID_TERM,   /* Invalid Terminate Flag detected */
    LMM_NO_ACCESS,      /* Unable to access member */ 
    LMM_NO_DELETE,      /* Unable to delete member */ 
    LMM_NO_MATCH,       /* Unable to find matching queue member */ 
    LMM_NO_QUEUE,       /* Failure to create request queue */
    LMM_NULL_PTR,       /* Null pointer to request packet received */
    LMM_BLANK_SOCKET,   /* Blank socket name detected */
    LMM_CLEAN_DRIVE,    /* Clean Transport Drive id Invalid */
    LMM_DUPLICATE,      /* Message Sequence Out-of-order, Final already received  */
    LMM_FATAL,          /* Unexpected error, exiting to ACSSS */
    LMM_FATAL_STATE,    /* Unexpected ACSLM state, exiting to ACSSS */
    LMM_FINAL_DET,      /* Final response detected */
    LMM_FINAL_NOTIF,    /* Final response generated */
    LMM_FREE_ERROR,     /* Allocated memory could not be freed */ 
    LMM_LSM_ONLINE,     /* Attempting to Vary LSM Identifier ONLINE */
    LMM_NOT_FATAL,      /* Unexpected error, not exiting  */
    LMM_OFFLINE_FORCE,  /* Attempting to Vary LSM Identifier OFFLINE with Force */
    LMM_PORT_OFFLINE,   /* Attempting to Vary PORT identiifer OFFLINE */
    LMM_RETRANS,        /* Transmission of message to itself on detected */
    LMM_RESID_REQ,      /* Residual request detected */ 
    LMM_REQ_ERROR,      /* Error in request queue detected, queue recreated */
    LMM_REQ_INCON,      /* Inconsistencies found in request queue, corrected */
    LMM_UNEXP_MEMBER,   /* Unexpected member identifier detected */
    LMM_UNKN_STATE,     /* Unexpected status detected in lm_request_table */
    LMM_TRACE_TRANS,    /* Entering transition */
    LMM_LAST            /* illegal */
} LMM_MESSAGE;

/*
 *      Global and Static Variable Declarations:
 */
extern STATE            lm_state;               /* current ACSLM state */
extern STATE            lm_previous_state;      /* previous ACSLM state */
extern QM_QID           acslm_req_tbl_ptr;      /* LM request table queue */
extern QM_MID           lm_next_member;         /* LM next queue member */
extern STATUS           lm_process_creation;    /* current rp fork/exec state */
extern int              lm_resources_available; /* current server resources */
extern int              lm_lsm_count;           /* current # lsm's in config */
extern int              lm_process_id;          /* acslm's process id */
extern LM_OFFLINE_VARY  *lm_offline_ptr;        /* ptr to lm vary offline tbl */
extern int              lm_suspend_fork;        /* suspend rp creation flag  */
extern int              lm_mount_count;         /* Number of ACSMT processes */
extern int              lm_query_count;         /* Number of ACSQY processes */
extern int              lm_acssurr_count;        /* Number of ACSSURR processes */

/*
 *      Procedure Type Declarations:
 */

STATUS lm_authentic(char *request_ptr);
STATUS lm_cancel_rp(char *request_ptr);
STATUS lm_chk_drvtbl(DRIVEID drive_id, BOOLEAN *value);
STATUS lm_clean_que(void);
void lm_completion(void);
STATUS lm_cvt_req(REQUEST_TYPE *reqp, int *byte_count);
STATUS lm_cvt_resp(RESPONSE_TYPE *rssp, int *byte_count);
STATUS lm_cvt_v0_v1(char *request_ptr, int *byte_count);
STATUS lm_cvt_v1_v0(char *response_ptr, int *byte_count);
STATUS lm_cvt_v1_v2(char *request_ptr, int *byte_count);
STATUS lm_cvt_v2_v1(char *response_ptr, int *byte_count);
STATUS lm_cvt_v2_v3(V2_REQUEST_TYPE *req_ptr, int *byte_count);
/* The v2 response packet may be used to cast the pointer because
 * there were not size or structure changes between v3 and v2.
 */
STATUS lm_cvt_v3_v2(V2_RESPONSE_TYPE *resp_ptr, int *byte_count);
STATUS lm_cvt_v3_v4(V3_REQUEST_TYPE *request_ptr, int *byte_count);
STATUS lm_cvt_v4_v3(RESPONSE_TYPE *response_ptr, int *byte_count);
STATUS lm_fmt_resp(char *request_ptr, STATUS fmt_status,
		   QM_MID request_member);
STATUS lm_fre_resrc(QM_MID member);
STATUS lm_get_resrc(void);
STATUS lm_idle_proc(int idle_options_flag);
STATUS lm_init(int argc, char *argv[]);
STATUS lm_input(char *request_ptr, int *byte_count);
STATUS lm_mk_captbl(void);
STATUS lm_mk_drvtbl(void);
STATUS lm_msg_hdlr(char *request_ptr, char *response_ptr);
STATUS lm_msg_size(char *request_ptr, int byte_count, int *calc_byte_count);
STATUS lm_output(char *response_ptr, int byte_count);
STATUS lm_req_proc(char *request_ptr, int byte_count);
STATUS lm_req_valid(char *request_ptr, int byte_count);
int lm_resource(char *request_ptr);
STATUS lm_resp_proc(char *response_ptr, int byte_count);
STATUS lm_rp_create(void);
STATUS lm_rp_table(void);
int lm_rp_table_loc(COMMAND cmd);
void lm_rp_trail(LM_REQUEST_TABLE *req_tbl_entry, QM_QID request_ID, MESSAGE_HEADER *msgHdr);
STATUS lm_set_drvtbl(DRIVEID drive_id);
void lm_sig_hdlr(int signal_received);
STATUS lm_split_resp(char dsoc[], RESPONSE_TYPE *rssp, int *byte_count);
int lm_tbl_loc(int search_direction, int select_type,
	       STATUS search_criteria_1, STATUS search_criteria_2);
void lm_terminate(int terminate_flag);
STATUS lm_validator(char *request_ptr, int *byte_count);
STATUS lm_wait_proc(char *request_ptr);

#endif /* _ACSLM_ */
