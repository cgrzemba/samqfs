/* SccsId @(#)lh_defs.h	1.2 1/11/94  */
#ifndef _LH_DEFS_
#define _LH_DEFS_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      lh_defs
 *
 *
 * Functional Description:
 *
 *      Declaration of all library_handler objects and functions visible to a
 *      request process or the ACSLM.
 *
 *
 * Modified by:
 *
 *      D.E. Skinner        24-Oct-1988     Original.
 *      D.E. Skinner         4-Jan-1989     Changed LH_RESPONSE into a union of
 *                      structures.
 *      D.E. Skinner        27-Jan-1989     Added MAX_RECOV_CELLS constant,
 *                      LH_STATE_LSM enumeration, lh_lsm_status.ready and
 *                      lh_lsm_status.state .
 *      D.E. Skinner        17-Sep-1990     Miscellaneous fixes.
 *      D. A. Beidle        17-Jun-1991     Add Multi-CAP support for LSMs.
 *                      also a bit of code cleanup.
 *      D. A. Beidle        19-Jun-1991     Removed LH_REQ_TYPE_REQ_STATUS,
 *                      LH_REQ_REQ_STATUS and LH_RESP_REQ_STATUS definitions.
 *                      Removed all statistic fields from responses.  Removed
 *                      the unsupported status and slot_full fields from the
 *                      PTP status response.  Added LH_LOCK_TYPE_IDLE.
 *      D. A. Beidle        25-Jun-1991     Added LH_DOOR_STATUS_UNDEFINED.
 *                      Changed "message" to "opmsg" in eject, enter, release
 *                      and reserve requests to reduce confusion with other 
 *                      messages.
 *      D. A. Beidle        10-Sep-1991     Moved LH_ERR_TYPE enumeration to
 *                      defs.h since its values are returned externally.
 *	A. W. Steere	    28-Sep-1991	    Removed CAP_PRIORITY, STATE,
 *			CAP_MODE, and cap_size from LH_CAP_STATUS following PFS
 *	A. W. Steere	    28-Sep-1991	    Removed CAP_PRIORITY, STATE,
 *			CAP_MODE, and cap_size from LH_CAP_STATUS following PFS
 *	M. L. Graham	    20-Apr-1992	    Added MAX_LSM_TRANSPORT constant,
 *                      removed boolean member "online" from connect response,
 *                      added number of panels to the lsm status response,
 *                      changed bound for transports array to use 
 *                      MAX_LSM_TRANSPORT in lsm status response, removed 
 *                      member "recov_cells" from lsm status response.
 *                      changed bound for transports array to use MAX_TRANSPORT,
 *                      in panel status response, removed LH_NR_REASON_CAP_OPEN
 *                      from LH_NR_REASON, remove enumeration LH_PANEL_TYPE,
 *                      added new #defines for the 4400 inner panel types.
 *      Alec Sharp          02-Jun-1992     Made the panel array in the lsm
 *                      status structure back into a typedef (PANEL_TYPE).
 *      J. S. Alexander     02-Jul-1992	    Added NUM_4400_INNER_PNLS #define.
 *      J. S. Alexander     14-Jul-1992	    Incorportated code review comments.
 *      J. S. Alexander     05-Aug-1992	    Added lsms element to the
 *                      LH_STATUS_ACS response. {1}
 *      C. A. Paul          12-Jul-1993     R5.0 - Added media_type to LH_VSN 
 *			and LH_LOCATION and drive_type to LH_STATUS_TRANSPORT
 *			for mixed media support.
 *	Mitch Black	SL8500-reltaed changes.  MAX_LSM_TRANSPORT(16 to 80)
 *				MAX_PTP(4 to 5).
 */


/*
 * Header Files:
 */

#ifndef _STRUCTS_
#include "structs.h"
#endif


/*
 * Defines, Typedefs and Structure Definitions:
 */
typedef  ROW    CAP_ROW;
typedef  COL    CAP_COL;
typedef  DRIVE  TRANSPORT;

#define  MIN_IPM             MIN_PORT   /* Lowest IPM address                 */
#define  MAX_IPM             MAX_PORT   /* Highest IPM address                */
#define  MAX_TRANSPORT       4          /* Max # of tape transports per drive */
#define  MAX_LSM_TRANSPORT   80         /* Max # of tape transports per lsm   */
#define  PTP_SLOTS           4          /* # of slots per PTP                 */
#define  MAX_PTP             5          /* Max # of PTPs per LSM              */
#define  HANDS               2          /* # of hands per LSM                 */
#define  VERSION_SIZE        16         /* Size of lh_lh_status.version[]     */
#define  LH_MAX_REQUESTS     32         /* Max # of requests to lh            */
#define  MAX_RECOV_CELLS     10         /* Maximum # of recovery cells per LSM*/
#define  NULL_IPC_IDENT      0L         /* a null ipc identifier              */
#define  LH_MAX_PRIORITY     99         /* Maximum priority for a request     */
#define  NUM_4400_INNER_PNLS 8          /* Number of inner panels in a 4400   */


/*
 *---------------------------  RESOURCE ADDRESSES  ---------------------------
 */

typedef  enum  {
    LH_ADDR_TYPE_FIRST = 0 ,
    LH_ADDR_TYPE_ACS       ,            /* ACS                  */
    LH_ADDR_TYPE_CAP       ,            /* CAP                  */
    LH_ADDR_TYPE_CAP_CELL  ,            /* CAP cell             */
    LH_ADDR_TYPE_CELL      ,            /* Panel cell           */
                            /*  5 */
    LH_ADDR_TYPE_DIAG_CELL ,            /* Diagnostic cell      */
    LH_ADDR_TYPE_LH        ,            /* library_handler      */
    LH_ADDR_TYPE_LIBRARY   ,            /* Library              */
    LH_ADDR_TYPE_LMU       ,            /* LMU                  */
    LH_ADDR_TYPE_LSM       ,            /* LSM                  */
                            /* 10 */
    LH_ADDR_TYPE_NONE      ,            /* None specified       */
    LH_ADDR_TYPE_PANEL     ,            /* Panel                */
    LH_ADDR_TYPE_PORT      ,            /* ACS port             */
    LH_ADDR_TYPE_PTP       ,            /* Pass-thru port       */
    LH_ADDR_TYPE_RECOV_CELL,            /* In-transit cartridge */
                            /* 15 */
    LH_ADDR_TYPE_TRANSPORT ,            /* Tape transport       */
    LH_ADDR_TYPE_LAST
} LH_ADDR_TYPE;

typedef  struct lh_addr_acs  {
    ACS            acs;
} LH_ADDR_ACS;

typedef  struct lh_addr_cap  {
    ACS            acs;
    LSM            lsm;
    CAP            cap;
} LH_ADDR_CAP;

typedef  struct lh_addr_cap_cell  {
    ACS            acs;
    LSM            lsm;
    CAP            cap;
    CAP_ROW        row;
    CAP_COL        column;
} LH_ADDR_CAP_CELL;

typedef  struct lh_addr_cell  {
    ACS            acs;
    LSM            lsm;
    PANEL          panel;
    ROW            row;
    COL            column;
} LH_ADDR_CELL;

typedef  struct lh_addr_diag_cell  {
    ACS            acs;
    LSM            lsm;
    PANEL          panel;
    ROW            row;
    COL            column;
} LH_ADDR_DIAG_CELL;

typedef  struct lh_addr_lh  {
    unsigned char  unused;
} LH_ADDR_LH;

typedef  struct lh_addr_library  {
    unsigned char  unused;
} LH_ADDR_LIBRARY;

typedef  struct lh_addr_lmu  {
    ACS            acs;
} LH_ADDR_LMU;

typedef  struct lh_addr_lsm  {
    ACS            acs;
    LSM            lsm;
} LH_ADDR_LSM;

typedef  struct lh_addr_none  {
    unsigned char  unused;
} LH_ADDR_NONE;

typedef  struct lh_addr_panel  {
    ACS            acs;
    LSM            lsm;
    PANEL          panel;
} LH_ADDR_PANEL;

typedef  struct lh_addr_port  {
    ACS            acs;
    PORT           port;
    char           name [PORT_NAME_SIZE];
} LH_ADDR_PORT;

typedef  struct lh_addr_ptp  {
    ACS            acs;
    unsigned char  ptp;
} LH_ADDR_PTP;

typedef  struct lh_addr_recov_cell  {
    ACS            acs;
    LSM            lsm;
    ROW            row;
} LH_ADDR_RECOV_CELL;

typedef  struct lh_addr_transport  {
    ACS            acs;
    LSM            lsm;
    PANEL          panel;
    TRANSPORT      transport;
    BOOLEAN        force_unload;
    BOOLEAN        write_protect;
} LH_ADDR_TRANSPORT;

typedef  struct lh_addr  {
    LH_ADDR_TYPE  type;
    union  {
        LH_ADDR_ACS         acs;
        LH_ADDR_CAP         cap;
        LH_ADDR_CAP_CELL    cap_cell;
        LH_ADDR_CELL        cell;
        LH_ADDR_DIAG_CELL   diag_cell;
        LH_ADDR_LH          lh;
        LH_ADDR_LIBRARY     library;
        LH_ADDR_LMU         lmu;
        LH_ADDR_LSM         lsm;
        LH_ADDR_NONE        none;
        LH_ADDR_PANEL       panel;
        LH_ADDR_PORT        port;
        LH_ADDR_PTP         ptp;
        LH_ADDR_RECOV_CELL  recov_cell;
        LH_ADDR_TRANSPORT   transport;
    } address;
} LH_ADDR;



/*
 *----------------------------  VSN/VOLSER INFO  -----------------------------
 */

typedef  enum  {
    LH_VSN_TYPE_FIRST = 0,
    LH_VSN_TYPE_BLANK    ,              /* VSN should be blank    */
    LH_VSN_TYPE_LABELED  ,              /* VSN should be readable */
    LH_VSN_TYPE_NONE     ,              /* VSN not specified      */
    LH_VSN_TYPE_LAST
} LH_VSN_TYPE;

/*
 * media_type was added to LH_VSN so that volser and media type would have
 * a one to one relationship.
 */
typedef  struct lh_vsn  {
    LH_VSN_TYPE  type;
    MEDIA_TYPE   media_type;
    char         vsn [EXTERNAL_LABEL_SIZE];
} LH_VSN;



/*
 *------------------------  LIBRARY_HANDLER REQUESTS  ------------------------
 */

typedef  enum  {
    LH_REQ_TYPE_FIRST = 0 ,
    LH_REQ_TYPE_CANCEL    ,
    LH_REQ_TYPE_CATALOG   ,
    LH_REQ_TYPE_CONNECT   ,
    LH_REQ_TYPE_DISCONNECT,
                            /*  5 */
    LH_REQ_TYPE_EJECT     ,
    LH_REQ_TYPE_ENTER     ,
    LH_REQ_TYPE_MOVE      ,
    LH_REQ_TYPE_RELEASE   ,
    LH_REQ_TYPE_RESERVE   ,
                            /* 10 */
    LH_REQ_TYPE_STATUS    ,
    LH_REQ_TYPE_VARY      ,
    LH_REQ_TYPE_LAST
} LH_REQ_TYPE;

typedef  struct lh_cancel_request  {
    unsigned long   ipc_identifier;
} LH_REQ_CANCEL;

typedef  enum  {
    LH_CAT_OPTION_FIRST = 0,
    LH_CAT_OPTION_ALL,
    LH_CAT_OPTION_FIRST_EMPTY,
    LH_CAT_OPTION_LAST
} LH_CAT_OPTION;

typedef  struct lh_catalog_request  {
    LH_ADDR         first;
    LH_ADDR         last;
    LH_CAT_OPTION   option;
} LH_REQ_CATALOG;

typedef  struct lh_connect_request  {
    LH_ADDR_PORT  port;
} LH_REQ_CONNECT;

typedef  struct lh_disconnect_request  {
    LH_ADDR_PORT  port;
} LH_REQ_DISCONNECT;

typedef  enum  {
    LH_OPMSG_FIRST = 0,
    LH_OPMSG_LIBRARY_UNAVAILABLE,
    LH_OPMSG_LOAD_CARTRIDGES,
    LH_OPMSG_NO_MESSAGE,
    LH_OPMSG_REMOVE_CARTRIDGES,
    LH_OPMSG_LAST
} LH_OPMSG;

typedef  struct lh_eject_request  {
    LH_ADDR_CAP     cap;
    LH_OPMSG        opmsg;
} LH_REQ_EJECT;

typedef  struct lh_enter_request  {
    LH_ADDR_CAP     cap;
    LH_OPMSG        opmsg;
} LH_REQ_ENTER;

typedef  struct lh_move_request  {
    LH_ADDR  source;
    LH_ADDR  destination;
    LH_VSN   vsn;
} LH_REQ_MOVE;

typedef  enum  {
    LH_LOCK_TYPE_FIRST = 0,
    LH_LOCK_TYPE_IDLE,
    LH_LOCK_TYPE_LOCK,
    LH_LOCK_TYPE_NOLOCK,
    LH_LOCK_TYPE_RECOVERY,
    LH_LOCK_TYPE_UNLOCK,
    LH_LOCK_TYPE_LAST
} LH_LOCK_TYPE;

typedef  struct lh_release_request  {
    LH_ADDR_CAP     cap;
    LH_LOCK_TYPE    mode;
    LH_OPMSG        opmsg;
} LH_REQ_RELEASE;

typedef  struct lh_reserve_request  {
    LH_ADDR_CAP     cap;
    LH_LOCK_TYPE    mode;
    LH_OPMSG        opmsg;
} LH_REQ_RESERVE;

typedef  struct lh_status_request  {
    LH_ADDR  resource;
} LH_REQ_STATUS;

typedef  enum  {
    LH_VARY_ACTION_FIRST = 0    ,
    LH_VARY_ACTION_ONLINE       ,
    LH_VARY_ACTION_OFFLINE      ,
    LH_VARY_ACTION_OFFLINE_FORCE,
    LH_VARY_ACTION_LAST
} LH_VARY_ACTION;

typedef  struct lh_vary_request  {
    LH_ADDR         resource;
    LH_VARY_ACTION  action;
} LH_REQ_VARY;


typedef  struct lh_request  {
    IPC_HEADER       ipc_header;
    LH_REQ_TYPE      request_type;
    REQUEST_PRIORITY request_priority;
    union {
        LH_REQ_CANCEL       cancel;
        LH_REQ_CATALOG      catalog;
        LH_REQ_CONNECT      connect;
        LH_REQ_DISCONNECT   disconnect;
        LH_REQ_EJECT        eject;
        LH_REQ_ENTER        enter;
        LH_REQ_MOVE         move;
        LH_REQ_RELEASE      release;
        LH_REQ_RESERVE      reserve;
        LH_REQ_STATUS       status;
        LH_REQ_VARY         vary;
    } request;
} LH_REQUEST;

/*
 *------------------------  LIBRARY_HANDLER RESPONSES  -----------------------
 */

typedef  struct lh_cancel_response  {
    unsigned long   ipc_identifier;
    LH_REQUEST      request;
} LH_RESP_CANCEL;

typedef  enum  {
    LH_CAT_STATUS_FIRST = 0        ,
    LH_CAT_STATUS_BAD_MOVE         ,
    LH_CAT_STATUS_EMPTY            ,
    LH_CAT_STATUS_INACCESSIBLE     ,
    LH_CAT_STATUS_LOADED           ,
                            /*  5 */
    LH_CAT_STATUS_MISSING          ,
    LH_CAT_STATUS_NO_TRANSPORT_COMM,
    LH_CAT_STATUS_READABLE         ,
    LH_CAT_STATUS_UNREADABLE       ,
    LH_CAT_STATUS_LAST
} LH_CAT_STATUS;

typedef  struct lh_location  {
    short  status;     /* lh_location.status stores only values of type     */
                       /*   LH_CAT_STATUS.  It is declared as type short    */
                       /*   instead of type LH_CAT_STATUS in order to       */
                       /*   reduce the size of large catalog responses.     */
    MEDIA_TYPE media_type;
    char       vsn [EXTERNAL_LABEL_SIZE];
} LH_LOCATION;

typedef  struct lh_catalog_response  {
    LH_ADDR         first;
    LH_ADDR         last;
    unsigned short  locations;
    LH_LOCATION     location [1];
} LH_RESP_CATALOG;

typedef  struct lh_connect_response  {
    LH_ADDR_PORT    port;
} LH_RESP_CONNECT;

typedef  struct lh_disconnect_response  {
    LH_ADDR_PORT  port;
} LH_RESP_DISCONNECT;

typedef  enum  {
    LH_DOOR_STATUS_FIRST = 0,
    LH_DOOR_STATUS_CLOSED,
    LH_DOOR_STATUS_OPENED,
    LH_DOOR_STATUS_UNDEFINED,
    LH_DOOR_STATUS_UNLOCKED,
    LH_DOOR_STATUS_LAST
} LH_DOOR_STATUS;

typedef  struct lh_eject_response  {
    LH_ADDR_CAP     cap;
    LH_DOOR_STATUS  status;
} LH_RESP_EJECT;

typedef  struct lh_enter_response  {
    LH_ADDR_CAP     cap;
    LH_DOOR_STATUS  status;
} LH_RESP_ENTER;

typedef  struct lh_error_response  {
    LH_ERR_TYPE     error;
    LH_ADDR         resource;
    BOOLEAN         recovery;
    LH_ADDR         address;
    LH_REQUEST      request;
    char            message [sizeof(int)];
} LH_RESP_ERROR;

typedef  struct lh_move_response  {
    LH_ADDR         source;
    LH_ADDR         destination;
    LH_VSN          vsn;
} LH_RESP_MOVE;

typedef  struct lh_release_response  {
    LH_ADDR_CAP     cap;
    LH_DOOR_STATUS  status;
} LH_RESP_RELEASE;

typedef  struct lh_reserve_response  {
    LH_ADDR_CAP     cap;
    LH_DOOR_STATUS  status;
} LH_RESP_RESERVE;

typedef  enum  {
    LH_CONDITION_FIRST = 0  ,
    LH_CONDITION_INOPERATIVE,
    LH_CONDITION_MTCE_REQD  ,
    LH_CONDITION_OPERATIVE  ,
    LH_CONDITION_LAST
} LH_CONDITION;

typedef  struct lh_acs_status  {
    int             lsms;
    BOOLEAN         lsm_accessible [MAX_LSM+1];
    unsigned char   num_ptp;
    unsigned short  ports;
    LH_ADDR_PORT    port [1];
} LH_STATUS_ACS;

typedef  enum  {
    LH_CAP_STATUS_FIRST = 0,
    LH_CAP_STATUS_EJECT,
    LH_CAP_STATUS_ENTER,
    LH_CAP_STATUS_IDLE,
    LH_CAP_STATUS_LAST
} LH_CAP_STATUS;

typedef  struct lh_cap_status  {
    BOOLEAN             operational;
    BOOLEAN             reserved;
    BOOLEAN             cap_scan;
    LH_CAP_STATUS       cap_status;
    LH_DOOR_STATUS      door_status;
    unsigned char       owner;
    unsigned char       available_cells;
    LH_ADDR_CAP_CELL    first;
    LH_ADDR_CAP_CELL    last;
    CAP_ROW             magazine_rows;
    CAP_COL             magazine_cols;
} LH_STATUS_CAP;

typedef  struct lh_lh_status  {
    char            version [VERSION_SIZE];
    unsigned char   host_id;
    unsigned int    ports;
    unsigned int    ports_online;
    LH_ADDR_PORT    port [1];
} LH_STATUS_LH;

typedef  struct lh_library_status  {
    BOOLEAN         acs_accessible [MAX_ACS+1];
} LH_STATUS_LIBRARY;

typedef  struct lh_lmu_status  {
    LH_CONDITION    ipm_status [MAX_IPM+1];
} LH_STATUS_LMU;

typedef  enum  {
    LH_STATE_LSM_FIRST = 0,
    LH_STATE_LSM_ONLINE   ,             /* LSM is online                     */
    LH_STATE_LSM_OFFLINE  ,             /* LSM is offline                    */
    LH_STATE_LSM_PENDING  ,             /* LSM is offline pending            */
    LH_STATE_LSM_MAINT    ,             /* LSM is offline (maintenance mode) */
    LH_STATE_LSM_LAST
} LH_STATE_LSM;


/*
 *  Define inner panel identifiers that the ACSLH must assume.
 */
#define LH_INNER_PANEL		-1     /* 4400 inner panel, plain */
#define LH_INNER_ADJ_PANEL	-2     /* 4400 inner panel, with door hinges */
#define LH_INNER_DOOR_PANEL	-3     /* 4400 inner panel, door to robot */

typedef  struct lh_lsm_status  {
    BOOLEAN             ready;
    LH_STATE_LSM        state;
    unsigned char	hands;
    LH_CONDITION        hand_status [HANDS];
    BOOLEAN             hand_empty [HANDS];
    BOOLEAN             door_closed;
    unsigned char       caps;
    unsigned short      panels;
    PANEL_TYPE          panel [MAX_PANEL+1];
    unsigned short      transports;
    LH_ADDR_TRANSPORT   transport [MAX_LSM_TRANSPORT];
    unsigned char       num_ptp;
    LH_ADDR_PTP         ptp [MAX_PTP];
} LH_STATUS_LSM;

typedef  struct lh_panel_status  {
    char                type;
    unsigned short      transports;
    LH_ADDR_TRANSPORT   transport [MAX_TRANSPORT];
} LH_STATUS_PANEL;

typedef  struct lh_port_status  {
    LH_ADDR_PORT       port;
    BOOLEAN            online;
} LH_STATUS_PORT;

typedef  struct lh_ptp_status  {
    unsigned char      ptp;
    LH_ADDR_PANEL      master;
    LH_ADDR_PANEL      slave;
} LH_STATUS_PTP;

typedef  enum  {
    LH_TAPE_STATUS_FIRST = 0,
    LH_TAPE_STATUS_EMPTY    ,
    LH_TAPE_STATUS_LOADED   ,
    LH_TAPE_STATUS_NO_COMM  ,
    LH_TAPE_STATUS_UNLOADED ,
    LH_TAPE_STATUS_LAST
} LH_TAPE_STATUS;

typedef  struct lh_transport_status  {
    DRIVE_TYPE         drive_type;
    LH_TAPE_STATUS     status;
    BOOLEAN            ready;
    BOOLEAN            clean;
} LH_STATUS_TRANSPORT;

typedef  enum  {
    LH_STATUS_TYPE_FIRST = 0,
    LH_STATUS_TYPE_ACS      ,
    LH_STATUS_TYPE_CAP      ,
    LH_STATUS_TYPE_LH       ,
    LH_STATUS_TYPE_LIBRARY  ,
                            /*  5 */
    LH_STATUS_TYPE_LMU      ,
    LH_STATUS_TYPE_LSM      ,
    LH_STATUS_TYPE_PANEL    ,
    LH_STATUS_TYPE_PORT     ,
    LH_STATUS_TYPE_PTP      ,
                            /* 10 */
    LH_STATUS_TYPE_TRANSPORT,
    LH_STATUS_TYPE_LAST
} LH_STATUS_TYPE;

typedef  struct lh_status_response  {
    LH_ADDR         resource;
    LH_STATUS_TYPE  status_type;
    union {
        LH_STATUS_ACS        acs;
        LH_STATUS_CAP        cap;
        LH_STATUS_LH         lh;
        LH_STATUS_LIBRARY    library;
        LH_STATUS_LMU        lmu;
        LH_STATUS_LSM        lsm;
        LH_STATUS_PANEL      panel;
        LH_STATUS_PORT       port;
        LH_STATUS_PTP        ptp;
        LH_STATUS_TRANSPORT  transport;
    } status;
} LH_RESP_STATUS;

typedef  struct lh_vary_response  {
    LH_ADDR         resource;
    LH_VARY_ACTION  action;
    BOOLEAN         online;
} LH_RESP_VARY;

typedef  enum  {
    LH_RESP_TYPE_FIRST = 0,
    LH_RESP_TYPE_INTERMED ,
    LH_RESP_TYPE_ERROR    ,
    LH_RESP_TYPE_FINAL    ,
    LH_RESP_TYPE_LAST
} LH_RESP_TYPE;

typedef  struct lh_response  {
    IPC_HEADER      ipc_header;
    LH_REQ_TYPE     request_type;
    LH_RESP_TYPE    response_type; 
    union {
        LH_RESP_CANCEL          cancel;
        LH_RESP_CATALOG         catalog;
        LH_RESP_CONNECT         connect;
        LH_RESP_DISCONNECT      disconnect;
        LH_RESP_EJECT           eject;
        LH_RESP_ENTER           enter;
        LH_RESP_ERROR           error;
        LH_RESP_MOVE            move;
        LH_RESP_RELEASE         release;
        LH_RESP_RESERVE         reserve;
        LH_RESP_STATUS          status;
        LH_RESP_VARY            vary;
    } response;
} LH_RESPONSE;


/*
 *------------------------  LIBRARY_HANDLER MESSAGES ------------------------
 */

typedef  enum  {
    LH_MSG_TYPE_FIRST = 0      ,
    LH_MSG_TYPE_AVAILABLE      ,
    LH_MSG_TYPE_CAP_CLOSED     ,
    LH_MSG_TYPE_CAP_OPENED     ,
    LH_MSG_TYPE_CLEAN_TRANSPORT,
                            /*  5 */
    LH_MSG_TYPE_DEGRADED_MODE  ,
    LH_MSG_TYPE_DOOR_CLOSED    ,
    LH_MSG_TYPE_DOOR_OPENED    ,
    LH_MSG_TYPE_LMU_READY      ,
    LH_MSG_TYPE_LSM_NOT_READY  ,
                            /* 10 */
    LH_MSG_TYPE_LSM_READY      ,
    LH_MSG_TYPE_PORT_OFFLINE   ,
    LH_MSG_TYPE_LAST
} LH_MSG_TYPE;

typedef  struct lh_available_message  {
    unsigned short  unused;
} LH_MSG_AVAILABLE;

typedef  struct lh_cap_closed_message  {
    LH_ADDR_CAP     cap;
} LH_MSG_CAP_CLOSED;

typedef  struct lh_cap_opened_message  {
    LH_ADDR_CAP     cap;
} LH_MSG_CAP_OPENED;

typedef  struct lh_clean_transport_message  {
    LH_ADDR_TRANSPORT  transport;
} LH_MSG_CLEAN_TRANSPORT;

typedef  enum  {
    LH_DM_CONDITION_FIRST = 0  ,
    LH_DM_CONDITION_DEGRADED   ,
    LH_DM_CONDITION_INOPERATIVE,
    LH_DM_CONDITION_LAST
} LH_DM_CONDITION;

typedef  struct lh_degraded_mode_message  {
    LH_ADDR          device;
    LH_DM_CONDITION  condition;
    unsigned short   fsc;
} LH_MSG_DEGRADED_MODE;

typedef  struct lh_door_closed_message  {
    LH_ADDR_LSM  lsm;
} LH_MSG_DOOR_CLOSED;

typedef  struct lh_door_opened_message  {
    LH_ADDR_LSM  lsm;
} LH_MSG_DOOR_OPENED;

typedef  struct lh_lmu_ready_message  {
    LH_ADDR_LMU  lmu;
} LH_MSG_LMU_READY;

typedef  enum  {
    LH_NR_REASON_FIRST = 0        ,
    LH_NR_REASON_CONFIG_MISMATCH  ,
    LH_NR_REASON_INIT_FAILED      ,
    LH_NR_REASON_LOST_COMM        ,
    LH_NR_REASON_MECHANISM_FAILED ,
                            /*  5 */
    LH_NR_REASON_PLAYGROUND_FULL  ,
    LH_NR_REASON_CAPACITY_MISMATCH,
    LH_NR_REASON_KEY_DOOR_OPEN    ,
    LH_NR_REASON_LAST
} LH_NR_REASON;

typedef  struct lh_lsm_not_ready_message  {
    LH_ADDR_LSM   lsm;
    LH_NR_REASON  reason;
} LH_MSG_LSM_NOT_READY;

typedef  struct lh_lsm_ready_message  {
    LH_ADDR_LSM  lsm;
} LH_MSG_LSM_READY;

typedef  struct lh_port_offline_message  {
    LH_ADDR_PORT  port;
} LH_MSG_PORT_OFFLINE;

typedef  struct lh_message  {
    IPC_HEADER   ipc_header;
    LH_MSG_TYPE  message_type;
    union  {
        LH_MSG_AVAILABLE        available;
        LH_MSG_CAP_CLOSED       cap_closed;
        LH_MSG_CAP_OPENED       cap_opened;
        LH_MSG_CLEAN_TRANSPORT  clean_transport;
        LH_MSG_DEGRADED_MODE    degraded_mode;
        LH_MSG_DOOR_CLOSED      door_closed;
        LH_MSG_DOOR_OPENED      door_opened;
        LH_MSG_LMU_READY        lmu_ready;
        LH_MSG_LSM_NOT_READY    lsm_not_ready;
        LH_MSG_LSM_READY        lsm_ready;
        LH_MSG_PORT_OFFLINE     port_offline;
    } message;
} LH_MESSAGE;


/*
 * Procedure Type Declarations:
 */


#endif /* _LH_DEFS_ */

