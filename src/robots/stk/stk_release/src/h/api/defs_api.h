/* P4_Id - $Id: h/api/defs_api.h#2 $ */

#ifndef _DEFS_API_
#define _DEFS_API_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      system-wide definitions needed for ACSAPI
 *
 * Modified by:
 *
 *      D. A. Myers     16-Sep-1993     Original
 *      E. A. Alongi	12-Oct-1993     Added TYPE_SSI to TYPE enumerated list.
 *      J. Borzuchowski 05-Aug-1993	    R5.0 Mixed Media-- Added
 *                                      TYPE_MIXED_MEDIA_INFO and TYPE_MEDIA_TYPE to TYPE enum.
 *                                      Added CLN_CART_CAPABILITY enum, MM_MAX_COMPAT_TYPES, 
 *                                      MEDIA_TYPE_NAME_LEN, and DRIVE_TYPE_NAME_LEN.
 *      J. Borzuchowski 26-Aug-1993     R5.0 Mixed Media-- Added BYPASS 
 *                                      bit definition to message options.
 *      J. Borzuchowski 03-Nov-1993     R5.0 Mixed Media-- Added 
 *                                      MEDIA_TYPE_LEN and DRIVE_TYPE_LEN.
 *      J. Borzuchowski 17-Nov-1993     R5.0 Mixed Media-- Moved
 *                                      MEDIA_TYPE_LEN and DRIVE_TYPE_LEN to ../defs.h since 
 *                                      these are only used internally.
 *      H. Grapek       11-Jan-1994     Added Library Station internal
 *                                      definitions
 *      Alec Sharp      02-Mar-1994     R5.0 BR#145 - added TYPE_CONFIG
 *      Howard Freeman  25-Nov-1996     R5.2 added TYPE_LMU and Query LMU and
 *                                      Switch LMU support.
 *      N. Afsar        06-Jan-1998     R5.3 added COMMAND_MOVE.
 *      J. A. Hanson    18-Sep-1998     Added message option bit definitions for 
 *                                      VTV_DUPLEX and VTV_SCRATCH.
 *      F. P. Thibaud   19-Aug-1999     support for ACSERRV
 *      George Noble    08-Sep-2000     Issue 647307 - roll-up Duncan Roe
 *                                      code - add TYPE_FIN. 
 *      George Noble    16-Nov-2000     R6.0 add COMMAND_RCVY,TYPE_CR,TYPE_MVD 
 *      George Noble    29-Dec-2000     R6.0 add TYPE_MISSING, TYPE_ERRANT 
 *      Van Lepthien    18-Sep-2001     ACSLS 6.1 and CSC DTK 2.2 synchronized. 
 *                                      Changes for Display and Event Notification.
 *      Scott Siao      18-Nov-2001     Added new types for Display command.
 *      Scott Siao      05-Dec-2001     Added TYPE_PTP for Event Notification.
 *      George Noble    11-Dec-2001     @A110 - Added BACKGROUND, ALL_DRIVES.
 *      Scott Siao      09-Jan-2002     Added COMMAND_MOUNT_PINFO and moved
 *                                      COMMAND SWITCH in order to synchronize
 *                                      with Library Station.
 *      Scott Siao      22-Jan-2002     Added COMMAND_MONITOR_EVENT for 
 *                                      event notification and synchronized
 *                                      with ACSLS code, changed order of
 *                                      some members in TYPE enum and added
 *                                      TYPE_DISP, TYPE_CLMON, TYPE_MON,
 *                                      TYPE_CAP_CELL, TYPE_DIAG_CELL,
 *                                      and TYPE_RECOV_CELL.
 *      Scott Siao      05-Feb-2002     Added MAX_DRG, MAX_SPN,
 *                                      SCRATCH.
 *                                      for Virtual Support.
 *      
 *      Scott Siao      06-Feb-2002     Synchronized TYPE enumeration with
 *                                      Library Station, inserted the
 *                                      following types:
 *                                        TYPE_DRIVE_GROUP,
 *                                        TYPE_SUBPOOL_NAME,
 *                                    	  TYPE_MOUNT_PINFO,
 *                                    	  TYPE_VTDID,
 *                                    	  TYPE_MGMT_CLAS,
 *                                    	  TYPE_JOB_NAME, and moved the
 *                                      existing ones down.  This change
 *                                      was coordinated with ACSLS and
 *                                      LS.
 *      Scott Siao      06-Mar-2002     Synchronized Command enumeration with
 *                                      ACSLS, added COMMAND_CONFIG, 
 *                                      and COMMAND_CONFIRM_CONFIG. 
 *      Scott Siao      06-Mar-2002     Synchronized w/ ACSLS, added 
 *                                      TYPE_DCONFIG.
 *
 */

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

typedef unsigned int    BOOLEAN;        /* {TRUE, FALSE} */

/*
 *  Message buffer definitions for internal server IPC messages and network
 *  messages translated to the native server format.  Memory-aligned message
 *  buffer definitions are included for portability.  MAX_MESSAGE_SIZE must
 *  be a multiple of sizeof(ALIGNED_BYTES).
 */

typedef void           *ALIGNED_BYTES;  /* memory-aligned data type */

#define MAX_MESSAGE_SIZE    4096        /* max IPC message size */
#define MAX_MESSAGE_BLOCK   (MAX_MESSAGE_SIZE / sizeof(ALIGNED_BYTES))

/*
 * Maximum length of lines read from ASCII files. Note that character
 * arrays should be defined 1 character larger to allow for the trailing \0,
 * while documentation should specify 1 character less to allow for the
 * last character being \n.
 */
#define MAX_LINE_LEN         256


/*
 *  The following definitions are used EXTERNALLY in the ACSLM Programmatic
 *  Interface as well as internally by the product.  Since these definitions
 *  are used EXTERNALLY, the ordering of definitions is IMPORTANT!  This is
 *  particularly true of the enumerated types.  To maintain compatibility with
 *  downlevel client applications, the ordering within the enumerated types
 *  MUST NOT BE CHANGED.  All new definitions within a type must be added
 *  before the "*_LAST" definition of that type.
 */

/*
 *  The maximum value MAX_ID can be defined as is 43.  Any larger value will
 *  cause the size of the query_lock response to exceed MAX_MESSAGE_SIZE.
 */
#define MAX_ID              42          /* max identifier count in packet */


/* The maximum number of GROUPIDs in Query_Drive_Group Request.
 * Any larger value will increase the size of the QUERY_REQUEST structure
 * and could lead to problems with downward compatibility.
 */
#define MAX_DRG         20   /* max GROUPIDs in Query_drive_group request*/
   
/*
 * The maximum number of SUBPOOL_NAMEs in Query_Subpool_Name Request.
 * Any larger value will increase the size of the QUERY_REQUEST structure
 * and could lead to problems with downward compatibility.
 */
#define MAX_SPN         20  /* max SUBPOOL_NAMEs in Query_Subpool_name request*/

/* message_option qualifier codes (bit field) */
#define FORCE           0x01
#define INTERMEDIATE    0x02
#define ACKNOWLEDGE     0x04
#define READONLY        0x08
#define BYPASS		0x10
#define VIRTAWARE       0x20
#define SCRATCH         0x40
#define SCRATCH         0x40
#define EXTENDED        0x80

/* These two message_option bits are used by Library Station for Virtual 
 * volume support. They are included here in an attempt to keep the source 
 * base sychronized with Library Station. As per request of Janet Hancock.
 * J. A. Hanson 9/18/98
 */
#define VTV_DUPLEX		0X20
#define VTV_SCRATCH		0X40

/* extended_options qualifier codes (bit field) */
#define WAIT            0x00000001
#define RESET           0x00000002
#define VIRTUAL         0x00000004
#define CONTINUOUS      0x00000008
#define RANGE           0x00000010
#define DIAGNOSTIC      0x00000020
#define BACKGROUND      0x10000000      /* internal responses only @A110 */
#define ALL_DRIVES      0x20000000      /* used for LibraryStation @E166 */
#define AUTOMATIC       0x40000000      /* internally-generated request */
#define CLEAN_DRIVE     0x80000000
/* 11-Dec-2001 George Noble     @A110 - Added BACKGROUND, ALL_DRIVES.
 *  BACKGROUND - on internal response, tells acslm to send response (as FINAL)
 *      to caller, but to retain the (still active, maybe cancellable) request
 *      until a FINAL response is issued.   
 *  ALL_DRIVES - defined/used in LibraryStation, added to prevent conflicts.
 */
typedef unsigned short  MESSAGE_ID;     /* request id assigned by acslm */
#define MIN_MESSAGE     1               /* min MESSAGE_ID value */
#define MAX_MESSAGE     65535           /* max MESSAGE_ID value */

/*
 * Maximum message size for messages that get logged using calls such
 * as cl_log_event and cl_log_unexpected
 */
#define MAX_LOG_MSG_SIZE     256 

/* ------------- command codes ------------------- */

typedef enum {
    COMMAND_FIRST = 0,                  /* illegal */
    COMMAND_AUDIT,
    COMMAND_CANCEL,
    COMMAND_DISMOUNT,
    COMMAND_EJECT,
                                /*  5 */
    COMMAND_ENTER,
    COMMAND_IDLE,
    COMMAND_MOUNT,
    COMMAND_QUERY,
    COMMAND_RECOVERY,                   /* ACSLM internal use only */
                                /* 10 */
    COMMAND_START,
    COMMAND_VARY,
    COMMAND_UNSOLICITED_EVENT,
    COMMAND_TERMINATE,                  /* ACSLM internal use only */
    COMMAND_ABORT,                      /* ACSLM internal use only */
                                /* 15 */
    COMMAND_SET_SCRATCH,
    COMMAND_DEFINE_POOL,
    COMMAND_DELETE_POOL,
    COMMAND_SET_CLEAN,
    COMMAND_MOUNT_SCRATCH,
                                /* 20 */
    COMMAND_UNLOCK,
    COMMAND_LOCK,
    COMMAND_CLEAR_LOCK,
    COMMAND_QUERY_LOCK,
    COMMAND_SET_CAP,
                                /* 25 */
    COMMAND_LS_RES_AVAIL,               /* ACSLM internal use only */
    COMMAND_LS_RES_REM,                 /* ACSLM internal use only */
    COMMAND_INIT,                       /* ACSLM internal use only */
    COMMAND_SELECT,
    COMMAND_SET_OWNER,
				/* 30 */
    COMMAND_DB_REQUEST,			/* Library Station Internal */
    COMMAND_MOUNT_PINFO,              /* Library Station Virtual Support */
    COMMAND_MOVE, 
    COMMAND_RCVY,                       /* Cartridge Recovery */
    COMMAND_SWITCH,                     /* Dual LMU, switch lmu */
				/* 35 */
    COMMAND_DISPLAY,                    /* Display Command */
    COMMAND_REGISTER,                   /* Event Notification */
    COMMAND_UNREGISTER,                 /* Event Notification */
    COMMAND_CHECK_REGISTRATION,         /* Event Notification */
    COMMAND_MONITOR_EVENT,              /* ACSLM internal use only */
				/* 40 */
    COMMAND_CONFIG,                     /* Dynamic Config Utility */
    COMMAND_CONFIRM_CONFIG,             /* Acscfg process */

    /* When adding a command, add it immeditately prior to COMMAND_LAST */
    COMMAND_LAST                        /* illegal */
} COMMAND;

/* type codes */
typedef enum {
    TYPE_FIRST = 0,                     /* illegal */
    TYPE_ACS,                           /* automated cartridge system */
    TYPE_AUDIT,                         /* audit request process */
    TYPE_CAP,                           /* cartridge access port */
    TYPE_CELL,                          /* cell identifier */
                                /*  5 */
    TYPE_CP,                            /* ACSSA command process */
    TYPE_CSI,                           /* client system interface */
    TYPE_DISMOUNT,                      /* dismount request process */
    TYPE_EJECT,                         /* eject request process */
    TYPE_EL,                            /* event logger */
                                /* 10 */
    TYPE_ENTER,                         /* enter request process */
    TYPE_DRIVE,                         /* library drive */
    TYPE_IPC,                           /* inter-process communication */
    TYPE_LH,                            /* library handler */
    TYPE_LM,                            /* library manager (ACSLM) */
                                /* 15 */
    TYPE_LSM,                           /* library storage module */
    TYPE_MOUNT,                         /* query mount request    */
    TYPE_NONE,                          /* no identifier specified */
    TYPE_PANEL,                         /* LSM panel */
    TYPE_PORT,                          /* ACS communications line */
                                /* 20 */
    TYPE_QUERY,                         /* query request process */
    TYPE_RECOVERY,                      /* recovery request process */
    TYPE_REQUEST,                       /* storage server request */
    TYPE_SA,                            /* system administrator (ACSSA) */
    TYPE_SERVER,                        /* storage server */
                                /* 25 */
    TYPE_SUBPANEL,                      /* LSM subpanel */
    TYPE_VARY,                          /* vary request process */
    TYPE_VOLUME,                        /* tape cartridge */
    TYPE_PD,                            /* library printer deamon */
    TYPE_SET_SCRATCH,                   /* set_scratch request process */
                                /* 30 */
    TYPE_DEFINE_POOL,                   /* define_pool request process */
    TYPE_DELETE_POOL,                   /* delete_pool request process */
    TYPE_SCRATCH,                       /* query scratch request   */
    TYPE_POOL,                          /* query pool    request   */
    TYPE_MOUNT_SCRATCH,                 /* query mount_scratch request  */
                                /* 35 */
    TYPE_VOLRANGE,                      /* volrange entity              */
    TYPE_CLEAN,                         /* clean type */
    TYPE_LOCK_SERVER,                   /* lock server type */
    TYPE_SET_CLEAN,                     /* clean request process */
    TYPE_SV,                            /* select process */
                                /* 40 */
    TYPE_MT,                            /* mount  process */
    TYPE_IPC_CLEAN,                     /* cleanup of shared memory ipc */
    TYPE_SET_CAP,                       /* set_cap request process */
    TYPE_LOCK,                          /* lock identifier */
    TYPE_CO_CSI,                        /* CSI co-process  */
                                /* 45 */
    TYPE_DISK_FULL,                     /* notify user disk full */
    TYPE_CM,
    TYPE_SET_OWNER,
    TYPE_MIXED_MEDIA_INFO,		/* mixed media info request type */
    TYPE_MEDIA_TYPE,			/* media type identifier */
				/* 50 */
    TYPE_SSI,				/* storage server interface */
    TYPE_DB_SERVER,			/* Library Station Internal */
    TYPE_DRIVE_GROUP,                   /* NCS Internal */
    TYPE_SUBPOOL_NAME,                  /* NCS Internal */
    TYPE_MOUNT_PINFO,                   /* NCS Internal */
				/* 55 */             
    TYPE_VTDID,                         /* NCS Internal */
    TYPE_MGMT_CLAS,                     /* NCS Internal */
    TYPE_JOB_NAME,                      /* NCS Internal */
    TYPE_STEP_NAME,                     /* NCS Internal */
    TYPE_MOUNT_SCRATCH_PINFO,           /* NCS          */
				/* 60 */
    TYPE_CONFIG,                        /* The Config family */
    TYPE_LMU,                           /* DLMU usage, Query LMU */
    TYPE_SWITCH,                        /* DLMU usage, Switch LMU */
    TYPE_MV,                            /* MOVE COMMAND */
    TYPE_ERRV,			        /* acserrv */
				/* 65 */
    TYPE_FIN,                           /* Final message from cmd_proc 647307 */
    TYPE_CR,                            /* ACSCR - Cartridge Recovery */
    TYPE_MVD,                           /* Manual Volume Delete */
    TYPE_MISSING,                       /* ACSCR - recover missing volumes */
    TYPE_ERRANT,                        /* ACSCR - clean up errant volumes */
                                /* 70 */
    TYPE_SURR,                          /* ACSSURR - "epicenter" surrogate */
    TYPE_HAND,                          /* Hand identifier */
    TYPE_GETTYPES,			/* Internal use for DISPLAY command */
    TYPE_PTP,				/* Supoort for L700e */
    TYPE_DISP,				/* DISPLAY command process */
                                /* 75 */
    TYPE_CLMON,				/* Internal use for Event notification*/
    TYPE_DISPLAY,                       /* Internal use for DISPLAY command */
    TYPE_ERROR,
    TYPE_MON,				/* Event notification process */
    TYPE_CAP_CELL,                      /* CAP_CELL type */
                                /* 80 */
    TYPE_DIAG_CELL,                     /* DIAGNOSTIC CELL type */
    TYPE_RECOV_CELL,                    /* recovery cell type */
    TYPE_DCONFIG,                       /* Dynamic Config utility */

    TYPE_LAST                           /* illegal */
    /* If you add a value to the TYPE_ enumeration, be sure to add it
       immediately before the TYPE_LAST value.  When you add TYPE_ values,
       be sure to update cl_type.c to add the new value. Also
       MAKE SURE ANY CHANGES TO THIS ENUMERATION ARE COORDINATED WITH
       THE NCS LIBRARY STATION AND CSC GROUPS */

} TYPE;

/* version field values */
typedef enum {
    VERSION0 = 0,
    VERSION1,
    VERSION2,
    VERSION3,
    VERSION4,
    VERSION_LAST                        /* illegal */
} VERSION;

#define VERSION_MINIMUM_SUPPORTED   VERSION1

/* As of 7/7/92, I am setting the default minimum version to VERSION1 - i.e.
 * we will support VERSION1 packets, but nothing lower. The intent
 * is to sometime set it to something like VERSION_LAST - 3, so that
 * we can obsolete the oldest version as we come out with new versions.
 * However, this needs more discussion and agreement from the PMP.
 */

/*
 * ACSLS needs to know the cleaning capabilities of the cartridge.  The
 * cartridge is always either a cleaning cartridge, i.e. it is never a 
 * data cartridge, (DD3CLN), the cartridge is never a cleaning cartridge 
 * (3490E) or the LMU does not have enough information to tell that the 
 * cartridge is a cleaning cartridge (3480).
 */
typedef enum {
	CLN_CART_FIRST = 0,
	CLN_CART_NEVER,		/* Never a cleaning cartridge,
			 	 * Always a data cartridge. 
			 	 */

	CLN_CART_INDETERMINATE,	/* LMU can't tell if a cleaning cart */

	CLN_CART_ALWAYS,	/* Always a cleaning cart, never a
				 * data cartridge
				 */
	CLN_CART_LAST
} CLN_CART_CAPABILITY;

/* 
 * Maximum number of compatible media types for a specific type of drive
 * or vice-versa.
 */
#define MM_MAX_COMPAT_TYPES	16

/*
 * Define media and drive type name lengths
 */
#define MEDIA_TYPE_NAME_LEN	10
#define DRIVE_TYPE_NAME_LEN	10

#endif /* _DEFS_API_ */
