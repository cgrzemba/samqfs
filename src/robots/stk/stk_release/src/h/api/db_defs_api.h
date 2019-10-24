/* P4_Id - $Id: h/api/db_defs_api.h#1 $ */

#ifndef _DB_DEFS_API_
#define _DB_DEFS_API_
/*
 * Copyright (1988, 2012) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header file defines system-wide data definitions used in database
 *      transactions and ACSLS subsystem interfaces (i.e., ACSLM programmatic 
 *      interface) as used by the ACSAPI.
 *
 * Modified by:
 *
 *      D. A. Myers     06-Oct-1988     Original
 *      A. W. Steere    18-Nov-1993     added ALL_MEDIA_TYPE (-2)
 *      H. Grapek       11-Jan-1994     Added Library Station internal
 *                                      definitions
 *      C. A. Paul      18-Apr-1994     R5.0 - Changed PCAP_PRIORITY value
 *                                      and added DEFAULT_PRIORITY.
 *      H. Freeman      11-Jan-1997     Added Query/Switch LMU support
 *      H. Freeman      24-Jun-1997     R5.2 Bug Fix # 129, Added new STATUS
 *      Jeri Miller     25-Jul-1997     R5.2 BR #58 put some comments by
 *                                      MIN_DRIVE & MAX_DRIVE, also added
 *                                      MAX_DRIVE_PANEL
 *      N.  Afsar       06-JAN-1998     R5.3 Added STATUS_MOVE_ACTIVITY
 *      H. Freeman      24-Jan-1998     R5.2.1 Added new CHANGE_DIRECTION
 *      Stephen Davies  12-Mar-1998     R5.2.1 - Changed PANEL_TYPE from
 *                                      signed char to signed int, in order to
 *                                      support new SCSI panel types.
 *      J.  Hanson      16-SEP-1998     R5.3 Changed MAX_DRIVE to 19 for 20
 *                                      drive panel support.
 *      Wipro (Vishal)  21-Dec-2000     TCP-IP Support Added. PORT_NAME_SIZE has
 *                                      been changed from 32 to 70.
 *      George Noble    16-Nov-2000     R6.0 Added STATUS_VOLUME_MISSING.
 *      George Noble    29-Dec-2000     R6.0 Added STATUS_VOLUME_ERRANT.
 *      Van Lepthien    18-Sep-2001     ACSLS 6.1 and CSC DTK 2.2 synchronized.
 *                                      Add STATUS values for event notification.
 *      Scott Siao      09-Oct-2001     Added typdef for HAND for event 
 *                                      notification. Added typdef for 
 *                                      EVENT_SEQUENCE for event notif.
 *      Scott Siao      29-Nov-2001     Changed RESOURCE_LMU_NOW_MASTER = 31, 
 *                                      to RESOURCE_LMU_NEW_MASTER=31
 *      Scott Siao      23-Jan-2002     Added MIN_VTD, JOB_NAME_SIZE,
 *                                      STEP_NAME_SIZE,DATASET_NAME_SIZE,
 *                                      SUBPOOL_NAME_SIZE, GROUPID_SIZE,
 *                                      and MGMT_CLAS_SIZE,
 *                                      VOLUME_TYPE_MVC,
 *                                      VOLUME_TYPE_VTV,
 *                                      GROUP_TYPE,
 *                                      for Virtual Support.
 *      Scott Siao      06-Feb-2002     Changed RESOURCE_LMU_SWITCHOVER_COMPLETE
 *                                      to RESOURCE_LMU_RECOVERY_COMPLETE.
 *      wipro (Vishal)  06-Oct-2001     Added STATUS_VOLUME_BEING_RECOVERED.
 *      W. K. Brenholtz 23-Oct-2001     Added definition for LSM_TYPE and enum
 *                                      for LSM types
 *      Wipro (Manjula) 30-Oct-2001     Added HAND, MAX_HAND and MIN_HAND,
 *                                      MAX_LMUS
 *                                      Added few enum values in STATUS for
 *                                      acsmon process
 *                                      Added STATUS_DRIVE_TYPE_CHG and
 *                                      STATUS_NI_TIMEOUT.
 *      W. K. Brenholtz 06-Dec-2001     Added STATUS_PANEL_NOT_IN_LIBRARY
 *      Wipro (Manaf)   26-Dec-2001     Removed STATUS_VOLUME_ERRANT from the
 *                                      enum list.
 *      Wipro (Hema)    26-Dec-2001     Added structures for Access control.
 *      Wipro (Manaf)   18-Jan-2002     Added STATUS_PANEL_NOT_FOUND to the
 *                                      STATUS enum list.
 *      Wipro (Vishal)  19-Jan-2002     Added new values into the STATUS enum.
 *      Scott Siao      18-Mar-2002     Synched w/ ACSLS 6.1: added 
 *                                      to STATUS enum 182 to 218.  Also added
 *                                      comments that explain that enums need
 *                                      to be in sync between the products
 *                                      that use them.
 *      Scott Siao      23-Apr-2002     Fixed comment in panel transports per panel
 *                                      section, updated for 20 panel drives.
 *      Scott Siao      13-May-2002     Synched with ACSLS, Added new STATUS for
 *                                      license keys.
 *      Wipro(Subhash)  28-May-2004     Added EVENT_REPLY_DRIVE_ACTIVITY,
 *                                      EVENT_CLASS_DRIVE_ACTIVITY.
 *                                      Also moved RESOURCE_DATA to mon_pub.h.
 *            [Mitch's Note - Nov 2004: RESOURCE_DATA was moved to ident_api.h
 *             within the CDK by Wipro; the above statement is in error, since
 *             mon_pub.h is not shipped with the CDK.  It is 
 *             advisable that these H files be made the same in a future release
 *             of ACSLS and the CDK, where both use the same API H files.]
 *      Mitch Black     22-Nov-2004     Changes made to reflect MAX values now
 *                                      supported by ACSLS
 *                                      (driven by SL8500/ACSLS 7.1 modifications).
 *                                      MAX_ACS changed from 126 to 31
 *                                      MAX_LSM changed from 23 to 126
 *                                      MAX_PANEL changed from 19 to 60.
 *      Mitch Black     29-Dec-2004     Added values to RESOURCE_EVENT enum.
 *      Joseph Nofi     15-Jul-2011     XAPI support;
 *                                      Added new STATUS values 
 *      Chris Morrison  14-Sep-2012     Increase MAX_CAP, MAX_DRIVE, & MAX_ROW
 *                                      for SL3000.
 *      Chris Morrison  26-Dec-2012     Add new VOLUME_TYPE enums.
 * 
 */


/* The C compiler is fine with signed chars, but the C++ compiler
   gives warnings, which is why we use the scheme below. */

#ifdef sun        
#define SIGNED
#else
#define SIGNED signed
#endif

typedef SIGNED char   ACS;              /* acs number/identifier */
#define MIN_ACS         0
#define MAX_ACS         31 
#define ANY_ACS         -1

#define MIN_VTD       0x80              /* use to detect VTD ID */

typedef SIGNED char   LSM;              /* lsm number */
#define MIN_LSM         0
#define MAX_LSM         126
#define ANY_LSM         -1

typedef char            PORT;           /* server-acs communications line */
#define MIN_PORT        0
#define MAX_PORT        15
#define PORT_NAME_SIZE  70       /* Modified -- Wipro -- Dated -- 21-Dec-2000 */

#define CSI_NAME_LENGTH 16              /* name of runnable csi */ 
#define MAX_CSI_LABEL   20              /* max csi name field length */
#define MAX_CSI_DESCRIP 1000            /* max csi desciption field length */

typedef SIGNED char   CAP;              /* cap number */
#define MIN_CAP         0
#define MAX_CAP         11
#define ANY_CAP         -1
#define ALL_CAP         -2

typedef char            PANEL;          /* lsm panel number */
#define MIN_PANEL       0
#define MAX_PANEL       60

typedef char            DRIVE;          /* transport number */
#define MIN_DRIVE       0
#define MAX_DRIVE       31
/*
 * NOTE: These are the minimum and maximum transport IDs per drive panel.
 * There can be up to 32 transports per drive panel, numbered 0 through 31.
 * So the actual maximum number of drives possible on a drive panel would be
 * (MAX_DRIVE - MIN_DRIVE) + 1.
 */

typedef char            ROW;            /* row number within a lsm panel */
#define MIN_ROW         0
#define MAX_ROW         51

typedef char            COL;            /* column number within a lsm panel */
#define MIN_COL         0
#define MAX_COL         23

typedef short           LOCKID;         /* define lock identifier values */
#define NO_LOCK_ID      0
#define MIN_LOCK_ID     0
#define MAX_LOCK_ID     32767           /* must be in bounds for type LOCKID */

typedef SIGNED char     MEDIA_TYPE; /* media type of a volume */
#define ANY_MEDIA_TYPE      (-1)
#define ALL_MEDIA_TYPE      (-2)
#define UNKNOWN_MEDIA_TYPE  (-3)

typedef SIGNED char     DRIVE_TYPE; /* transport type */
#define ANY_DRIVE_TYPE      (-1)
#define UNKNOWN_DRIVE_TYPE  (-3)

typedef long            POOL;           /* define scratch pool values */
#define COMMON_POOL     0
#define MAX_POOL        65534
#define SAME_POOL       -1

#define MAX_WATER_MARK  2147483647      /* equals MAXLONG of values.h */
#define MIN_WATER_MARK  0               /* equals MAXLONG of values.h */

typedef SIGNED char   CAP_PRIORITY;     /* define CAP priority values */
#define NO_PRIORITY     0
#define MIN_PRIORITY    1
#define MAX_PRIORITY    16
#define SAME_PRIORITY   -1

typedef char            REQUEST_PRIORITY; /* LH request priority - 0-99 */

#define PCAP_PRIORITY    98          /* Request priority for PCAP operations */
#define DEFAULT_PRIORITY 25          /* Default request priority */
#define PCAP_SIZE        1           /* Maximum number of cells in PCAP  */

typedef SIGNED int PANEL_TYPE;      /* Panel type variable              */

typedef char             HAND;      /* Hand identifier for event notification */
#define MIN_HAND         0
#define MAX_HAND         1          /* must be in bounds for type HANDID */

typedef unsigned long    EVENT_SEQUENCE;   /* event sequence id for event notificatio*/

#define EXTERNAL_USERID_SIZE    64
typedef struct {
    char user_label[EXTERNAL_USERID_SIZE];
} USERID;

#define EXTERNAL_PASSWORD_SIZE  64
typedef struct {
    char password[EXTERNAL_PASSWORD_SIZE];
} PASSWORD;

typedef struct {
    USERID      user_id;
    PASSWORD    password;
} ACCESSID;


typedef long            FREECELLS;      /* count of unused cells within a */
                                        /* server or acs or lsm */
#ifdef LINUX
#undef OVERFLOW
#endif

#define OVERFLOW        1
#define EXTERNAL_LABEL_SIZE 6           /* label characters */
#define SOCKET_NAME_SIZE 14             /* max characters in socket name */

#define JOB_NAME_SIZE       8    /* number of characters in job name */
#define STEP_NAME_SIZE      8    /* number of characters in step name */
#define DATASET_NAME_SIZE   44   /* number of characters in dataset name */
#define SUBPOOL_NAME_SIZE   13   /* number of characters in subpool name */
#define GROUPID_SIZE        8    /* number of characters in VTSS name */
#define MGMT_CLAS_SIZE      8    /* number of characters in mgmt class name */


#define MAX_DRIVE_PANEL 4               /* max number of drive panels */
                                        /*     allowed per LSM        */


/*
 * --------------- PLEASE READ THIS ------------------------------------
 * The following enumerated types must be kept strictly in their current
 * order since they appear in the external programmatic interface. If you
 * add enumerated values, add them at the end, immediately before the
 * XXX_LAST value.
 */

/* cell location codes */
typedef enum {
    LOCATION_FIRST = 0,                 /* illegal */
    LOCATION_CELL,
    LOCATION_DRIVE,
    LOCATION_LAST                       /* illegal */
} LOCATION;

/* state codes */
typedef enum {
    STATE_FIRST = 0,                    /* illegal */
    STATE_CANCELLED,                    /* process state only */
    STATE_DIAGNOSTIC,
    STATE_IDLE,
    STATE_IDLE_PENDING,
                                /*  5 */
    STATE_OFFLINE,
    STATE_OFFLINE_PENDING,
    STATE_ONLINE,
    STATE_RECOVERY,
    STATE_RUN,
                                /*  10 */
    STATE_CONNECT, 
    STATE_DISCONNECT,
    STATE_DISBAND_1,
    STATE_DISBAND_2,
    STATE_DISBAND_3,
    STATE_DISBAND_4,
    STATE_JOIN_1,
    STATE_JOIN_2,
    STATE_RESIGN_1,
    STATE_RESIGN_2,
                               /* 20 */
    STATE_RESIGN_3,

    STATE_LAST                          /* illegal */
} STATE;

/* volid volume_type codes */
typedef enum {
    VOLUME_TYPE_FIRST = 0,              /* illegal */
    VOLUME_TYPE_DIAGNOSTIC,             /* volid may contain blanks */
    VOLUME_TYPE_STANDARD,               /* volid must be 6 chars, [A-Z][0-9] */
    VOLUME_TYPE_DATA,                   /* vol_type field in volumetable */
    VOLUME_TYPE_SCRATCH,                /* vol_type field in volumetable */
                                /*  5 */
    VOLUME_TYPE_CLEAN,                  /* vol_type field in volumetable */
    VOLUME_TYPE_MVC,                    /* multiple volume cartridge     */
    VOLUME_TYPE_VTV,                    /* virtual type volume           */
    VOLUME_TYPE_SPENT_CLEANER,          /* spent (used-up) cleaning cart.*/
    VOLUME_TYPE_MEDIA_ERROR,            /* cartridge with media error    */
                                        /* may be used in the future      */
                                /*  10 */
    VOLUME_TYPE_UNSUPPORTED_MEDIA,      /* media not supported by library */
                                        /* may be used in the future      */
    VOLUME_TYPE_C_OR_D,                 /* either a cleaning or data cart */
                                        /* reserved - used internally     */
    VOLUME_TYPE_LAST                    /* illegal */
} VOLUME_TYPE;

/* label attributes for volumes */
typedef enum {                          /* valid values for label attribute */
    LABEL_ATTRIBUTE_STANDARD = 0,
    LABEL_ATTRIBUTE_VIRTUAL,
    LABEL_ATTRIBUTE_LAST                /* illegal */
} LABEL_ATTR;

/* CAP modes */
typedef enum {                          /* valid values for CAP modes */
    CAP_MODE_FIRST = 0,
    CAP_MODE_AUTOMATIC,
    CAP_MODE_MANUAL,
    CAP_MODE_SAME,
    CAP_MODE_LAST                       /* illegal */
} CAP_MODE;

/* group types */                                             
typedef enum { 
    GROUP_TYPE_FIRST = 0,               /* illegal */
    GROUP_TYPE_VTSS,                    /* VTSS group */
    GROUP_TYPE_LAST                     /* illegal */ 
} GROUP_TYPE;
/*
 * Status codes.  Note that these status code are generally intended for
 * external interface use.  Internal software errors should be give the
 * status of STATUS_PROCESS_FAILURE.  If any changes are made to this enumeration
 * they should be reflected in the ACSLS enum and the Library Station enum
 * as well as the Horizon Library Manager enum.
 *
 * Please be aware that since this is an enumeration members should not
 * be removed, only appended.
 *
 */
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ACS_FULL,
    STATUS_ACS_NOT_IN_LIBRARY,
    STATUS_ACS_OFFLINE,
    STATUS_ACSLM_IDLE,
                                /*   5 */
    STATUS_ACTIVITY_END,
    STATUS_ACTIVITY_START,
    STATUS_AUDIT_ACTIVITY,
    STATUS_AUDIT_IN_PROGRESS,
    STATUS_CANCELLED,
                                /*  10 */
    STATUS_CAP_AVAILABLE,
    STATUS_CAP_FULL,
    STATUS_CAP_IN_USE,
    STATUS_CELL_EMPTY,
    STATUS_CELL_FULL,
                                /*  15 */
    STATUS_CELL_INACCESSIBLE,
    STATUS_CELL_RESERVED,
    STATUS_CLEAN_DRIVE,
    STATUS_COMMUNICATION_FAILED,
    STATUS_CONFIGURATION_ERROR,
                                /*  20 */
    STATUS_COUNT_TOO_SMALL,
    STATUS_COUNT_TOO_LARGE,
    STATUS_CURRENT,
    STATUS_DATABASE_ERROR,
    STATUS_DEGRADED_MODE,
                                /*  25 */
    STATUS_DONE,
    STATUS_DOOR_CLOSED,
    STATUS_DOOR_OPENED,
    STATUS_DRIVE_AVAILABLE,
    STATUS_DRIVE_IN_USE,
                                /*  30 */
    STATUS_DRIVE_NOT_IN_LIBRARY,
    STATUS_DRIVE_OFFLINE,
    STATUS_DRIVE_RESERVED,
    STATUS_DUPLICATE_LABEL,
    STATUS_EJECT_ACTIVITY,
                                /*  35 */
    STATUS_ENTER_ACTIVITY,
    STATUS_EVENT_LOG_FULL,
    STATUS_IDLE_PENDING,
    STATUS_INPUT_CARTRIDGES,
    STATUS_INVALID_ACS,
                                /*  40 */
    STATUS_INVALID_COLUMN,
    STATUS_INVALID_COMMAND,
    STATUS_INVALID_DRIVE,
    STATUS_INVALID_LSM,
    STATUS_INVALID_MESSAGE,
                                /*  45 */
    STATUS_INVALID_OPTION,
    STATUS_INVALID_PANEL,
    STATUS_INVALID_PORT,
    STATUS_INVALID_ROW,
    STATUS_INVALID_STATE,
                                /*  50 */
    STATUS_INVALID_SUBPANEL,
    STATUS_INVALID_TYPE,
    STATUS_INVALID_VALUE,
    STATUS_INVALID_VOLUME,
    STATUS_IPC_FAILURE,
                                /*  55 */
    STATUS_LIBRARY_BUSY,
    STATUS_LIBRARY_FAILURE,
    STATUS_LIBRARY_NOT_AVAILABLE,
    STATUS_LOCATION_OCCUPIED,
    STATUS_LSM_FULL,
                                /*  60 */
    STATUS_LSM_NOT_IN_LIBRARY,
    STATUS_LSM_OFFLINE,
    STATUS_MESSAGE_NOT_FOUND,
    STATUS_MESSAGE_TOO_LARGE,
    STATUS_MESSAGE_TOO_SMALL,
                                /*  65 */
    STATUS_MISPLACED_TAPE,
    STATUS_MULTI_ACS_AUDIT,
    STATUS_NORMAL,
    STATUS_NONE,
    STATUS_NOT_IN_SAME_ACS,
                                /*  70 */
    STATUS_ONLINE,
    STATUS_OFFLINE,
    STATUS_PENDING,
    STATUS_PORT_NOT_IN_LIBRARY,
    STATUS_PROCESS_FAILURE,
                                /*  75 */
    STATUS_RECOVERY_COMPLETE,
    STATUS_RECOVERY_FAILED,
    STATUS_RECOVERY_INCOMPLETE,
    STATUS_RECOVERY_STARTED,
    STATUS_REMOVE_CARTRIDGES,
                                /*  80 */
    STATUS_RETRY,
    STATUS_STATE_UNCHANGED,
    STATUS_TERMINATED,
    STATUS_VALID,
    STATUS_VALUE_UNCHANGED,
                                /*  85 */
    STATUS_VARY_DISALLOWED,
    STATUS_VOLUME_ADDED,
    STATUS_VOLUME_EJECTED,
    STATUS_VOLUME_ENTERED,
    STATUS_VOLUME_FOUND,
                                /*  90 */
    STATUS_VOLUME_HOME,
    STATUS_VOLUME_IN_DRIVE,
    STATUS_VOLUME_IN_TRANSIT,
    STATUS_VOLUME_NOT_IN_DRIVE,
    STATUS_VOLUME_NOT_IN_LIBRARY,
                                /*  95 */
    STATUS_UNREADABLE_LABEL,
    STATUS_UNSUPPORTED_OPTION,
    STATUS_UNSUPPORTED_STATE,
    STATUS_UNSUPPORTED_TYPE,
    STATUS_VOLUME_IN_USE,
                                /* 100 */
    STATUS_PORT_FAILURE,
    STATUS_MAX_PORTS,
    STATUS_PORT_ALREADY_OPEN,
    STATUS_QUEUE_FAILURE,
    STATUS_NI_FAILURE,
    STATUS_RPC_FAILURE = STATUS_NI_FAILURE,  /* dup'd for diff. net archs */
                                /* 105 */
    STATUS_NI_TIMEDOUT,
    STATUS_INVALID_COMM_SERVICE,
    STATUS_COMPLETE,
    STATUS_AUDIT_FAILED,
    STATUS_NO_PORTS_ONLINE,
                                /* 110 */
    STATUS_CARTRIDGES_IN_CAP,
    STATUS_TRANSLATION_FAILURE,
    STATUS_DATABASE_DEADLOCK,
    STATUS_DIAGNOSTIC,
    STATUS_DUPLICATE_IDENTIFIER,
                                /* 115 */
    STATUS_EVENT_LOG_FAILURE,
    STATUS_DISMOUNT_ACTIVITY,
    STATUS_MOUNT_ACTIVITY,
    STATUS_POOL_NOT_FOUND,
    STATUS_POOL_NOT_EMPTY,
                                /* 120 */
    STATUS_INVALID_RANGE,
    STATUS_INVALID_POOL,
    STATUS_POOL_HIGH_WATER,
    STATUS_POOL_LOW_WATER,
    STATUS_INVALID_VERSION,
                                /* 125 */
    STATUS_MISSING_OPTION,
    STATUS_INCORRECT_ATTRIBUTE,
    STATUS_INVALID_LOCKID,
    STATUS_VOLUME_AVAILABLE,
    STATUS_READABLE_LABEL,
                                /* 130 */
    STATUS_NO_CAP_AVAILABLE,
    STATUS_LOCK_FAILED,
    STATUS_DEADLOCK,
    STATUS_LOCKID_NOT_FOUND,
    STATUS_INCORRECT_LOCKID,
                                /* 135 */
    STATUS_SCRATCH_NOT_AVAILABLE,
    STATUS_CLEAN_DRIVE_COMPLETE,
    STATUS_VOLUME_NOT_FOUND,
    STATUS_CAP_DOOR_OPEN,
    STATUS_CAP_INOPERATIVE,
                                /* 140 */
    STATUS_DISK_FULL,
    STATUS_CAP_NOT_IN_LIBRARY,
    STATUS_CAP_OFFLINE,
    STATUS_INVALID_CAP,
    STATUS_INCORRECT_CAP_MODE,
                                /* 145 */
    STATUS_INCORRECT_STATE,
    STATUS_VARY_IN_PROGRESS,
    STATUS_ACS_ONLINE,
    STATUS_AUTOMATIC,
    STATUS_MANUAL,
                                /* 150 */
    STATUS_VOLUME_DELETED,
    STATUS_INSERT_MAGAZINES,
    STATUS_UNSUPPORTED_COMMAND,
    STATUS_COMMAND_ACCESS_DENIED,
    STATUS_VOLUME_ACCESS_DENIED,
                                /* 155 */
    STATUS_OWNER_NOT_FOUND,
    STATUS_INVALID_DRIVE_TYPE,
    STATUS_INVALID_MEDIA_TYPE,
    STATUS_INCOMPATIBLE_MEDIA_TYPE,
    STATUS_DRIVE_FOUND,         /* Library Station Internal */
                                /* 160 */
    STATUS_CAP_DONE,            /* Library Station Internal */
    STATUS_INVALID_SUBSYSID,        /* Library Station Internal */
    STATUS_LSM_OFFLINE_PENDING,     /* Library Station Internal */
    STATUS_PORT_OFFLINE,        /* Library Station Internal */
    STATUS_COMMUNICATING,
                                /* 165 */
    STATUS_NOT_COMMUNICATING,
    STATUS_IDLE,
    STATUS_OFFLINE_PENDING,
    STATUS_SWITCHOVER_INITIATED,
    STATUS_SWITCHOVER_RECOVERY_COMPLETE,
                                /* 170 */
    STATUS_LMU_STATUS_CHANGE_NEW_MASTER,
    STATUS_LMU_STATUS_CHANGE_STANDBY_COMM,
    STATUS_LMU_STATUS_CHANGE_STANDBY_NOT_COMM,
    STATUS_STANDBY_LMU_NO_PORTS,
    STATUS_SWITCHOVER_IN_PROGRESS,
                               /* 175 */
    STATUS_LAST_MASTER_PORT,
    STATUS_NOT_CONFIGD_DUAL,
    STATUS_DISALLOWED_ON_MASTER,
    STATUS_LMU_TO_LMU_LINK_BAD,
    STATUS_TOO_MANY_NAKS,
                                /* 180 */
    STATUS_MASTER_NOT_RESPONDING,
    STATUS_MOVE_ACTIVITY,
    STATUS_VOLUME_MISSING,
    STATUS_VOLUME_BEING_RECOVERED,
    STATUS_INVALID_CLIENT,
                    /* 185 */
    STATUS_VOLUME_REACTIVATED,
    STATUS_VOLUME_ABSENT,
    STATUS_UNIT_ATTENTION,
    STATUS_READY,
    STATUS_NOT_READY,
                    /* 190 */
    STATUS_SERIAL_NUM_CHG,
    STATUS_HARDWARE_ERROR,
    STATUS_MONITOR_COMPLETE,
    STATUS_PTP_NOT_FOUND,
    STATUS_INVALID_EVENT_CLASS,
                    /* 195 */
    STATUS_HAND_NOT_FOUND,
    STATUS_LMU_NOT_FOUND,
    STATUS_OPERATIVE,
    STATUS_INOPERATIVE,
    STATUS_MAINT_REQUIRED,
                    /* 200 */
    STATUS_CAP_DOOR_CLOSED,
    STATUS_LSM_TYPE_CHG,
    STATUS_CONFIGURATION_CHANGED,
    STATUS_DRIVE_TYPE_CHG,
    STATUS_NI_TIMEOUT,
                    /* 205 */
    STATUS_VOLUME_OVER_MAX_CLEAN,
    STATUS_VOLUME_CLEAN_CART_SPENT,
    STATUS_DRIVE_ADDED,
    STATUS_DRIVE_REMOVED,
    STATUS_LMU_TYPE_CHG,
                    /* 210 */
    STATUS_LMU_COMPAT_LVL_CHG,
    STATUS_LMU_NOW_STANDALONE,
    STATUS_LMU_NOW_MASTER,
    STATUS_LMU_NOW_STANDBY,
    STATUS_PANEL_NOT_IN_LIBRARY,
                    /* 215 */
    STATUS_PANEL_NOT_FOUND,
    STATUS_STORAGE_CELL_MAP_CHANGED,
    STATUS_NOT_A_DRIVE_PANEL,
    STATUS_NO_DRIVES_FOUND,
    STATUS_LKEY_INVALID,
                    /* 220 */
    STATUS_LKEY_EXPIRED,
    STATUS_LKEY_DUE_TO_EXPIRE,
    STATUS_LKEY_CAPACITY_EXCEEDED,
    STATUS_MALFORMED_XML,
    STATUS_TAG_NOT_FOUND,
                    /* 225 */
    STATUS_CONTENT_NOT_FOUND,
    STATUS_INVALID_CONTENT,
    STATUS_INCOMPATIBLE_SERVER, 
    STATUS_MAX_REQUESTS_EXCEEDED,
    STATUS_SHARED_MEMORY_ERROR,
                    /* 230 */
    STATUS_MGMTCLAS_NOT_FOUND,
    STATUS_LAST                         /* illegal */

} STATUS;

/* only append values at end of this enumeration but before STATUS_LAST */



/*
 * Mode codes.  These mode codes reflect the mode of an lmu
 */
typedef enum {
    MODE_FIRST = 0,
    MODE_DUAL_LMU,
    MODE_SINGLE_LMU,
    MODE_SCSI_LMU,
    MODE_NONE,
    MODE_LAST                           /* illegal */
} MODE;

/*
 * Role codes.  These mode codes reflect the role of an lmu
 */
typedef enum {
    ROLE_FIRST = 0,
    ROLE_MASTER_A,
    ROLE_MASTER_B,
    ROLE_STANDBY_A,
    ROLE_STANDBY_B,
    ROLE_NONE,
    ROLE_LAST                           /* illegal */
} ROLE;

/*
 * Update codes.  These change_direction codes indicate increment or decrement
 */
typedef enum {
    CHANGE_FIRST = 0,
    CHANGE_INCREMENT,
    CHANGE_DECREMENT,
    CHANGE_LAST
} CHANGE_DIRECTION;


/*
 * These event class types are for event notification
 */

typedef enum {
    EVENT_CLASS_FIRST = 0,
    EVENT_CLASS_VOLUME,
    EVENT_CLASS_RESOURCE,
    EVENT_CLASS_DRIVE_ACTIVITY,
    EVENT_CLASS_LAST
} EVENT_CLASS_TYPE;

/*
 * For the different event reply types
 */

typedef enum {
    EVENT_REPLY_FIRST = 0,
    EVENT_REPLY_REGISTER,
    EVENT_REPLY_UNREGISTER,
    EVENT_REPLY_SUPERCEDED,
    EVENT_REPLY_SHUTDOWN,
    EVENT_REPLY_CLIENT_CHECK,
    EVENT_REPLY_RESOURCE,
    EVENT_REPLY_VOLUME,
    EVENT_REPLY_DRIVE_ACTIVITY,
    EVENT_REPLY_LAST
} EVENT_REPLY_TYPE;


/* values defined for RESOURCE_EVENT */

typedef enum {
    RESOURCE_FIRST = 0,
    RESOURCE_ONLINE = 1,    /*general codes - 01-19 */
    RESOURCE_OFFLINE,
    RESOURCE_OPERATIVE,        
    RESOURCE_INOPERATIVE,
    RESOURCE_MAINT_REQUIRED,  /*  5  */
    RESOURCE_UNIT_ATTENTION,
    RESOURCE_HARDWARE_ERROR,
    RESOURCE_DEGRADED_MODE,     
    RESOURCE_SERIAL_NUM_CHG,
    RESOURCE_DIAGNOSTIC,      /* 10 */

    /* values less than 20 are reserved for general events */

    /* server specific codes - 21-29  */
    RESOURCE_SERV_CONFIG_MISMATCH = 21,
    RESOURCE_SERV_CONFIG_CHANGE,
    RESOURCE_SERV_START,
    RESOURCE_SERV_IDLE,
    RESOURCE_SERV_IDLE_PENDING,   /*  25  */
    RESOURCE_SERV_FAILURE,
    RESOURCE_SERV_LOG_FAILED,
    RESOURCE_SERV_LOG_FILLED,

    /* LMU  specific codes - 31-39  */
    RESOURCE_LMU_NEW_MASTER = 31,
    RESOURCE_LMU_STBY_COMM,
    RESOURCE_LMU_STBY_NOT_COMM,
    RESOURCE_LMU_RECOVERY_COMPLETE,
    RESOURCE_LMU_NOW_STANDBY,       /* 35 - Not Implemented */
    RESOURCE_LMU_NOW_STANDALONE,    /* Not Implemented */
    RESOURCE_LMU_TYPE_CHG,      /* Not Implemented */
    RESOURCE_LMU_COMPAT_LVL_CHG,    /* Not Implemented */


    /* CAP  specific codes - 41-49  */
    /* See also CAP Configuration Change codes  91-95 */
    RESOURCE_CAP_DOOR_OPEN  = 41,
    RESOURCE_CAP_DOOR_CLOSED,
    RESOURCE_CARTRIDGES_IN_CAP,
    RESOURCE_CAP_ENTER_START,
    RESOURCE_CAP_ENTER_END,        /*  45  */
    RESOURCE_CAP_REMOVE_CARTRIDGES,
    RESOURCE_NO_CAP_AVAILABLE,
    RESOURCE_CAP_INSERT_MAGAZINES,
    RESOURCE_CAP_INPUT_CARTRIDGES,

    /* LSM  specific codes - 51-59  */
    RESOURCE_LSM_DOOR_OPENED  = 51,
    RESOURCE_LSM_DOOR_CLOSED,
    RESOURCE_LSM_RECOVERY_INCOMPLETE,
    RESOURCE_LSM_TYPE_CHG,
    RESOURCE_LSM_ADDED,        /*  55  */
    RESOURCE_LSM_CONFIG_CHANGE,
    RESOURCE_LSM_REMOVED,

    /* drive  specific codes - 61-69  */
    RESOURCE_DRIVE_CLEAN_REQUEST  = 61,
    RESOURCE_DRIVE_CLEANED,
    RESOURCE_DRIVE_TYPE_CHG,
    RESOURCE_DRIVE_ADDED,
    RESOURCE_DRIVE_REMOVED,

    /* POOL  specific codes - 71-79  */
    RESOURCE_POOL_HIGHWATER   = 71,
    RESOURCE_POOL_LOWWATER,

    /* ACS  specific codes - 81-89  */
    RESOURCE_ACS_ADDED     = 81,
    RESOURCE_ACS_CONFIG_CHANGE,
    RESOURCE_ACS_REMOVED,
    RESOURCE_ACS_PORTS_CHANGE,

    /* CAP Configuration Change codes - 91-95   */
    /* See also CAP codes  41-49 */
    RESOURCE_CAP_ADDED     =  91,
    RESOURCE_CAP_CONFIG_CHANGE,
    RESOURCE_CAP_REMOVED,

    RESOURCE_LAST = 99
} RESOURCE_EVENT;

/*
 * ------------- End special enumerated types section --------------------- 
 * Above this point, if you add a value to one of the enumerated lists, be
 * sure to add it at the end, just before the XXX_LAST value.  If you add a
 * value to the STATUS enumeration, be sure to update ../common_lib/cl_status.c
 * to add the new status.
 */

typedef struct {
    int    category;
    int    code;
} SENSE_HLI;

typedef struct {
    unsigned char    sense_key;
    unsigned char    asc;
    unsigned char    ascq;
} SENSE_SCSI;

typedef struct {
    char             fsc[4];
} SENSE_FSC;

#define MAX_SERIAL_NUM_LENGTH  32

typedef struct {
    char    serial_nbr[MAX_SERIAL_NUM_LENGTH +1];
} SERIAL_NUM;

typedef int LSM_TYPE;

#define RESOURCE_ALIGN_PAD_SIZE  64

#endif /* _DB_DEFS_API_ */
