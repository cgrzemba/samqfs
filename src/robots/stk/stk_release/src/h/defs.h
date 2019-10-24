/* SccsId @(#)defs.h	1.2 1/11/94  */
#ifndef _DEFS_
#define _DEFS_
/*
 * Copyright (1988, 2012) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      system-wide definitions
 *      includes db_defs.h, unless already defined, to be complete.
 *
 * Considerations: 
 * 
 *      Because db_defs.h may be included within an SQL declare section of
 *      embedded SQL modules, it is conditionally included here.  Due to the
 *      limitations of the SQL preprocessor, the conditionally included header
 *      file cannot conditionally exclude itself.
 * 
 * Modified by:
 *
 *      D. F. Reed          19-Sep-1988     Original.
 *      D. A. Beidle        10-Sep-1991     Reorganize into external interface
 *                      and internal definitions.  Added LH_ERR_TYPE typedef.
 *      H. I. Grapek        11-Sep-1991     Added cl_csi_write to list of
 *                      STATUS functs.
 *      J. S. Alexander     20-Sep-1991     Added cl_csi_read to list of
 *                      STATUS functs.
 *      D. A. Beidle        27-Sep-1991     Added comments on the necessity
 *                      of conditionally including db_defs.h
 *      D. A. Beidle        18-Apr-1992.    BR#442 - CLM_CAT_TARGET_ERROR 
 *                      message code added.
 *      Alec Sharp          26-Apr-1992     U4-MR6. Added VERSION3 and
 *                      function declarations for cl_db_ functions and
 *                      cl_sql_ functions and cl_caps_in_lsm. Removed revision
 *                      history 20-Sept-88 to 09-Sept-91.
 *                      Added MAX_LOG_MSG_SIZE, cl_pnlid_info,
 *                      VERSION_MINIMUM_SUPPORTED.
 *      Alec Sharp          15-Aug-1992     Added COMMAND_SET_OWNER and
 *                      TYPE_SET_OWNER, cl_vac_ family
 *      Alec Sharp          27-Oct-1992     Added MAX_LINE_LEN.
 *      Alec Sharp          05-Nov-1992     Added cl_abbreviation,
 *                      CLM_FILE_PROBLEM.
 *      Alec Sharp          22-Nov-1992     Changed TRACE macro.
 *	C. N. Hobbie	    05-Feb-1993	    Updated the variant to R4.0S0.4
 *	G. A. Mueller       18-Mar-1993	    Updated the variant to R4.0S0.5
 *                                          and added CLM_LOCKED_VOL_DELETED
 *                                          for bug#405-R4
 *	G. A. Mueller       19-Apr-1993	    Updated the variant to R4.0S0.6
 *	G. A. Mueller       03-May-1993	    Updated the variant to R4.0S2.0
 *                                          for bug#405-R4
 *	T. Z. Khawaja       08-Jul-1993	    Added new values to the CL_MESSAGE
 *					    enumeration for Mixed Media support.
 *      C. A. Paul          20-Jul-1993     Added new error codes to 
 *					    LH_ERR_TYPE.
 *      A. W. Steere        19-Jul-1993     Added new error codes for mixed 
 *					    media config file CLM_ codes.
 *					    Removed cl_trace_enter, 
 *					    cl_trace_volume.
 *      D. B. Farmer        12-Aug-1993     Added ACSSS_VARIANT for bull
 *	Emanuel A. Alongi   17-Aug-1993     Added enumeration constant VERSION4.
 *      Alec Sharp          23-Aug-1993     Added MIN, MAX macros.
 *	David A. Myers	    16-Sep-1993	    Split file with portion required
 *			by ACSAPI into api/defs_api.h
 *	Emanuel A. Alongi   28-Sep-1993	    Added a define for SA_RESTART.
 *      Alec Sharp          04-Oct-1993     Added DEFAULT_TIME_FORMAT
 *	Emanuel A. Alongi   28-Sep-1993	    Conditionally include sys/types.h
 *	David Farmer        17-Nov-1993	    Include ml_pub.h
 *	David Farmer        17-Nov-1993	    moved CL_ASSERT to ml_pub.h and
 *					    removed ml_pub.h
 *	Janet Borzuchowski  17-Nov-1993     R5.0 Mixed Media-- Added
 *			MEDIA_TYPE_LEN and DRIVE_TYPE_LEN.
 *	Janet Borzuchowski  24-Nov-1993	    R5.0 Mixed Media-- Added 
 *			MM_MEDIA_DB_STR_LEN and MM_DRIVE_DB_STR_LEN; added
 *		        MM_MAX_MEDIA_TYPES and MM_MAX_DRIVE_TYPES (moved from
 *			lib/h/cl_mm_pri.h).
 *      Martin Ryder        26-Mar-2001     Changed MM_MAX_DRIVE_TYPES from 20
 *                                          to 40.
 *      Scott Siao          04-Mar-2002     Code cleanup removed ACSSS_VERSION
 *                                          and ACSSS_RELEASE.
 *      Hemendra(wipro)     29-Dec-2002     Commented declaration of following 
 *					    variables for Linux:
 *                                          sys_errlist and sys_siglist
 *      Mitch Black         24-Nov-2004     Increased MAX_LSM_PTP from 4 to 5
 *      Chris Morrison      14-Sep-2012     Increase MM_MAX_MEDIA_TYPES and 
 *                                          MM_MAX_DRIVE_TYPES.
 */

/*
 *      Header Files:
 */

#if ! defined (__sys_types_h) && ! defined (_H_TYPES)
#include <sys/types.h>
#endif

#ifndef sun
#ifndef _H_SELECT
#include <sys/select.h>
#endif
#endif

#ifndef _DB_DEFS_
#include "db_defs.h"
#endif

#ifndef _DEFS_API_
#include "api/defs_api.h"
#endif


/* signal stuff: SA_RESTART is a sigaction() flag (sa_flags) defined on most
 * platforms.   This flag causes certain "slow" systems calls (usually those
 * that can block) to restart after being interrupted by a signal.  The
 * SA_RESTART flag is not defined under SunOS because the interrupted system
 * call is automatically restarted by default.  So, in order to make it possible
 * to write one version of the sigaction() code, SA_RESTART is defined here for
 * platforms running SunOS.
 */
#ifndef SA_RESTART
#define SA_RESTART	0
#endif

/*
 * General macros to determine minimum and maximum values
 */

#ifndef  MAX
#define  MAX(a,b)  (((a)>(b))?(a):(b))
#endif /* MAX */

#ifndef  MIN
#define  MIN(a,b)  (((a)<(b))?(a):(b))
#endif /* MIN */

/* Default date/time format string */
#define DEFAULT_TIME_FORMAT   "%m-%d-%y %H:%M:%S"


/* library error codes */
typedef  enum  {
    LH_ERR_FIRST = 0,                   /* illegal */
    LH_ERR_ADDR_INACCESSIBLE,
    LH_ERR_ADDR_TYPE_INVALID,
    LH_ERR_ADDR_UNDEFINED,
    LH_ERR_CANCEL_PENDING,
                                /*  5 */
    LH_ERR_CANCEL_TOO_LATE,
    LH_ERR_CAP_BUSY,
    LH_ERR_CAP_FAILURE,
    LH_ERR_DESTINATION_FULL,
    LH_ERR_FIRST_EXCEEDS_LAST,
                                /* 10 */
    LH_ERR_LH_BUSY,
    LH_ERR_LH_FAILURE,
    LH_ERR_LMU_FAILURE,
    LH_ERR_LSM_FAILURE,
    LH_ERR_LSM_OFFLINE,
                                /* 15 */
    LH_ERR_LSM_OFFLINE_MTCE,
    LH_ERR_MULTI_ACS,
    LH_ERR_MULTI_LSM,
    LH_ERR_MULTI_PANEL,
    LH_ERR_MULTI_TYPE,
                                /* 20 */
    LH_ERR_PATH_UNAVAILABLE,
    LH_ERR_PORT_CONNECT,
    LH_ERR_PORT_DISCONNECT,
    LH_ERR_REQUEST_CANCELLED,
    LH_ERR_REQUEST_INVALID,
                                /* 25 */
    LH_ERR_REQUEST_NOT_ACTIVE,
    LH_ERR_SOURCE_EMPTY,
    LH_ERR_TRANSPORT_BUSY,
    LH_ERR_TRANSPORT_FAILURE,
    LH_ERR_UNABLE_TO_CANCEL,
                                /* 30 */
    LH_ERR_VARY_OVERRIDDEN,
    LH_ERR_VARY_PENDING,
    LH_ERR_VSN_INVALID,
    LH_ERR_VSN_VERIF_FAILED,
    LH_ERR_ALREADY_RESERVED,
                                /* 35 */
    LH_ERR_CAP_OPEN,
    LH_ERR_LMU_LEVEL_INVALID,
    LH_ERR_NO_ERROR,                    /* internal LH status only */
    LH_ERR_NOT_RESERVED,
    LH_ERR_NO_MAGAZINE,
                                /* 40 */
    LH_ERR_MEDIA_VERIF_FAIL,
    LH_ERR_MEDIA_VSN_VERIF_FAIL,
    LH_ERR_INCOMPATIBLE_MEDIA_DRIVE,
    LH_ERR_MEDIA_TYPE_INVALID,
    LH_ERR_LAST                         /* illegal */
} LH_ERR_TYPE;

/*
 * The following definitions are used INTERNALLY by all components of the 
 * product.  Since they are only used INTERNALLY, the order of definitions
 * is not important.  However it is suggested that the alphabetical order
 * within the enumerated types be maintained.
 */

#define CAP_MSG_INTERVAL    120         /* how often empty/fill cap msg sent*/
#define DATAGRAM_PATH       "/tmp/"     /* path for building datagram files */
#define MAX_ACSMT_PROCS     2           /* max number of ACSMT processes    */
#define MAX_CSI             20          /* max number of CSIs allowed       */
#define MAX_LSM_PTP         5           /* max number of PTPs per LSM       */
#define MAX_PORTS           16          /* max communication ports per ACS  */
#define MAX_RETRY           10          /* max lh_request retry attempts    */
#define RETRY_TIMEOUT       2           /* time-out seconds between retries */


/* IPC_HEADER option values (bit field) */
#define RETRY           0x01

/* well-known socket name definitions */
/* uses IP port numbers > 50000 (IPPORT_USERRESERVED) */
#define ACSEL           "50001"
#define ACSLH           "50002"
#define ACSLM           "50003"
#define ACSSA           "50004"
#define ACSSS           "50005"
#define ACSPD           "50006"
#define ACSLOCK         "50007"
#define ACSSV           "50008"
#define ACSCM           "50009"
#define ACES            "50010"
#define ACSMT           "50100"
#define ANY_PORT        "0"

/* execution trace support definitions.  low-order byte is trace_value */
#define TRACE_ACSSS_DAEMON      0x00000100L
#define TRACE_CSI               0x00000200L
#define TRACE_ACSLM             0x00000400L
#define TRACE_MOUNT             0x00000800L
#define TRACE_DISMOUNT          0x00001000L
#define TRACE_ENTER             0x00002000L
#define TRACE_EJECT             0x00004000L
#define TRACE_AUDIT             0x00008000L
#define TRACE_QUERY             0x00010000L
#define TRACE_VARY              0x00020000L
#define TRACE_RECOVERY          0x00040000L
#define TRACE_ACSSA             0x00080000L
#define TRACE_CP                0x00100000L
#define TRACE_LIBRARY_HANDLER   0x00200000L
#define TRACE_EVENT_LOGGER      0x00400000L
#define TRACE_CSI_PACKETS       0x00800000L
#define TRACE_LOCK_SERVER       0x01000000L
#define TRACE_SET_CAP           0x02000000L
#define TRACE_SET_CLEAN         0x04000000L
#define TRACE_ACSCM             0x08000000L


#define TRACE(lev)          /* TRUE if trace enabled    */ \
/*lint -e568*/ (trace_value != 0 && (trace_value & 0xff) >= lev) /*lint +e568*/
    
typedef void (*SIGFUNCP)();             /* pointer to signal handler */

/* common library event log messages */
typedef enum {
    CLM_FIRST = 0,                      /* illegal */
    CLM_ABORT_TRANSITION,
    CLM_ALLOC_ERROR,
    CLM_ASSERTION,
    CLM_CAT_TARGET_ERROR,
                                /*  5 */
    CLM_DB_DEADLOCK,
    CLM_DB_TIMEOUT,
    CLM_DUP_TYPE_NUM,
    CLM_DUP_TYPE_STR,
    CLM_DESTINATION_FULL,
                                /* 10 */
    CLM_FILE_PROBLEM,
    CLM_FIXED_MEDIA_TYPE,
    CLM_FIXED_VOLUME_TYPE,
    CLM_FUNC_FAILED,
    CLM_INC_TYPES,
                                /* 15 */
    CLM_INV_ARG_NUM,
    CLM_INV_ARG_STR,
    CLM_INV_NUM_ARGS,
    CLM_IPC_ATTACH,
    CLM_IPC_OPEN,
                                /* 20 */
    CLM_IPC_SEND,
    CLM_KILL_ERROR,
    CLM_LOCKED_VOL_DELETED,
    CLM_MSG_TIMEOUT,
    CLM_MSG_TOO_SMALL,
                                /* 25 */
    CLM_NO_TYPES,
    CLM_NOT_BOOLEAN,
    CLM_NOT_DEFINED,
    CLM_SIGNAL_ERROR,
    CLM_SOURCE_EMPTY,
                                /* 30 */
    CLM_TABLE_INCORRECT,
    CLM_TOO_MANY_COMPAT,
    CLM_TRACE_TRANSITION,
    CLM_UNDEF_TRANSITION,
    CLM_UNEXP_CAT_STATUS,
                                /* 35 */
    CLM_UNEXP_COMMAND,
    CLM_UNEXP_EVENT,
    CLM_UNEXP_LD_STATUS,
    CLM_UNEXP_LH_REQUEST,
    CLM_UNEXP_LH_RESPONSE,
                                /* 40 */
    CLM_UNEXP_MESSAGE,
    CLM_UNEXP_ORIGINATOR,
    CLM_UNEXP_REQUESTOR,
    CLM_UNEXP_SIGNAL,
    CLM_UNEXP_STATE,
                                /* 45 */
    CLM_UNEXP_STATUS,
    CLM_UNEXP_TYPE,
    CLM_UNKNOWN_MEDIA_TYPE,
    CLM_UNLINK_ERROR,
    CLM_UNSUP_LH_ERROR,
                                /* 50 */
    CLM_UNSUP_LH_REQUEST,
    CLM_UNSUP_VERSION,
    CLM_VOL_FOUND,
    CLM_VOL_MISPLACED,
    CLM_VOL_MOVED,
                                /* 55 */
    CLM_VOL_NOT_FOUND,
    CLM_LAST                            /* illegal */
} CL_MESSAGE;

/* data base field update codes */
typedef enum {
    FIELD_FIRST = 0,                    /* illegal */
    FIELD_ACTIVITY,
    FIELD_CAP_MODE,
    FIELD_LOCKID,
    FIELD_MAX_USE,
                                /*  5 */
    FIELD_POOLID,
    FIELD_PRIORITY,
    FIELD_STATE,
    FIELD_STATUS,
    FIELD_VOLUME_TYPE,
                                /* 10 */
    FIELD_LAST                          /* illegal */
} FIELD;

/* log_option codes */
typedef enum {
    LOG_OPTION_FIRST = 0,               /* illegal */
    LOG_OPTION_EVENT,
    LOG_OPTION_TRACE,
    LOG_OPTION_LAST                     /* illegal */
} LOG_OPTION;

/* database query_type codes */
typedef enum {
    QUERY_TYPE_FIRST = 0,               /* illegal */
    QUERY_TYPE_ALL,                     /* get all records in table         */
    QUERY_TYPE_ALL_ACS,                 /* get all records with same ACS    */
    QUERY_TYPE_ALL_CELL,                /* get all records w/same home cell */
    QUERY_TYPE_ALL_DRIVE,               /* get all records w/same drive     */
                                /*  5 */
    QUERY_TYPE_ALL_LSM,                 /* get all records with same LSM    */
    QUERY_TYPE_LSM_RESERVED,            /* get record by LSM & rsv'd status */
    QUERY_TYPE_NEXT,                    /* get next sequential record       */
    QUERY_TYPE_ONE,                     /* get record by primary key        */
    QUERY_TYPE_ONE_CELL,                /* get record by cell identifier    */
                                /* 10 */
    QUERY_TYPE_ONE_DRIVE,               /* get record by drive identifier   */
    QUERY_TYPE_LAST                     /* illegal */
} QUERY_TYPE;

/* cell select option codes */
typedef enum {
    SELECT_OPTION_FIRST = 0,            /* illegal */
    SELECT_OPTION_ACS,                  /* if lsm full, try any lsm in acs  */
    SELECT_OPTION_LSM,                  /* try specified lsm only */
    SELECT_OPTION_LAST                  /* illegal */
} SELECT_OPTION;

/* database write_mode codes */
typedef enum {
    WRITE_MODE_FIRST = 0,               /* illegal */
    WRITE_MODE_CREATE,                  /* create a new record */
    WRITE_MODE_UPDATE,                  /* update an existing record */
    WRITE_MODE_LAST                     /* illegal */
} WRITE_MODE;

/*
 * Define media and drive type ASCII lengths needed for database queries.  
 */
#define MEDIA_TYPE_LEN		3
#define DRIVE_TYPE_LEN		3

/*
 * Define max number of types for media and drive types.
 */
#define MM_MAX_MEDIA_TYPES	70
#define MM_MAX_DRIVE_TYPES	80

/* Define length of comma separated media types string and drive types string, 
 * used for database queries.
 */
#define MM_MEDIA_DB_STR_LEN	(MM_MAX_MEDIA_TYPES*(MEDIA_TYPE_LEN + 1))+1
#define MM_DRIVE_DB_STR_LEN	(MM_MAX_DRIVE_TYPES*(DRIVE_TYPE_LEN + 1))+1

/*
 *      Global Variable Declarations:
 */

/*
 * The following system and library global variables are declared to permit
 * all components of the product to access system-defined data. 
 */

#ifndef LINUX
extern  char           * sys_errlist[];  /* list of errno message strings    */
extern  char           * sys_siglist[];  /* list of signal names             */
#endif

/*
 * The following product global variables are declared to provide data needed
 * by all components of the product.  Declarations added in R3 and subsequent
 * releases begin with the "cl_" prefix in accordance with the revised coding
 * guidelines.
 */

extern  char            acsss_version[];/* current ACSLS version/variant    */
extern int              sd_in;          /* module input socket descriptor */
extern int              n_fds;          /* number of input descriptors */
extern int              fd_list[FD_SETSIZE];
                                        /* input descriptor list */
extern char             my_sock_name[SOCKET_NAME_SIZE];
                                        /* module input socket name */
extern TYPE             my_module_type; /* executing module's type */
extern TYPE             requestor_type; /* request originator's module type */
extern int              restart_count;  /* process failed/restart count */
extern MESSAGE_ID       request_id;     /* associated request ID, or 0 if */
                                        /* not associated with a request */
extern STATE            process_state;  /* executing process' state flag */
extern unsigned long    trace_module;   /* module trace define value */
extern unsigned long    trace_value;    /* trace flag value */


/*
 *      Function Prototypes
 */

#endif /* _DEFS_ */

