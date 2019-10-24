/* SccsId @(#)v3_structs.h	2.0 1/11/94  */
#ifndef _V3_STRUCTS_
#define _V3_STRUCTS_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      definitions of VERSION3 specific system data structures.  
 *      most of these structures are common to csi and acslm, 
 *      and are included by both header files.
 *
 * Considerations:
 *
 * Modified by:
 *	Emanuel A. Alongi	07-Jul-93	Initial, actually a copy of
 *					structs.h to preserve the VERSION3
 *					request/response structures.
 *	Janet Borzuchowski	27-Aug-93	R5.0 Mixed Media-- Added
 *					unions V3_REQUEST_TYPE and 
 *					V3_RESPONSE_TYPE, and typedef for
 *					V3_RESPONSE_HEADER, all used for up and
 *					down level conversions.
 *					Changed use of QU_DRV_STATUS to
 *					V3_QU_DRV_STATUS in V3_QU_MNT_STATUS and
 *					V3_MSC_STATUS.
 *	E. A. Alongi    17-Sep-1993.    Added define for V3_QU_MAX_DRV_STATUS.
 *	E. A. Alongi    17-Sep-1993.    Use V3_QU_MAX_DRV_STATUS as dimension
 *					in query mount and mount scratch status
 *					structures.
 */

/*
 *      Header Files:
 */

#ifndef _STRUCTS_
#include "structs.h"
#endif

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/* 
 * maximum number of drive_status entries in mount and mount_scratch  
 * query responses.                 
 */
#define V3_QU_MAX_DRV_STATUS   175         


typedef struct {                        /* fixed portion of request_packet */
    IPC_HEADER      ipc_header;
    MESSAGE_HEADER  message_header;
} V3_REQUEST_HEADER;

typedef struct {                        /* intermediate acknowledgement */
    V3_REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    MESSAGE_ID      message_id;
} V3_ACKNOWLEDGE_RESPONSE;

typedef struct {                        /* audit_request */
    V3_REQUEST_HEADER      request_header;
    CAPID               cap_id;         /*   CAP for ejecting cartridges      */
    TYPE                type;           /*   type of identifiers              */
    unsigned short      count;          /*   number of identifiers            */
    union {                             /*   list of homogeneous IDs to audit */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        PANELID         panel_id[MAX_ID];
        SUBPANELID      subpanel_id[MAX_ID];
    } identifier;
} V3_AUDIT_REQUEST;

typedef struct {                        /* audit_response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    CAPID               cap_id;         /*   CAP for ejecting cartridges   */
    TYPE                type;           /*   type of identifiers           */
    unsigned short      count;          /*   number of audited identifiers */
    union {                             /*   list of IDs audited w/status  */
        AU_ACS_STATUS   acs_status[MAX_ID];
        AU_LSM_STATUS   lsm_status[MAX_ID];
        AU_PNL_STATUS   panel_status[MAX_ID];
        AU_SUB_STATUS   subpanel_status[MAX_ID];
    } identifier_status;
} V3_AUDIT_RESPONSE;

typedef struct {                        /* eject_enter intermediate response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    CAPID               cap_id;         /*   CAP for ejecting cartridges     */
    unsigned short      count;          /*   no. of volumes ejected/entered  */
    VOLUME_STATUS       volume_status[MAX_ID];
} V3_EJECT_ENTER;

typedef V3_EJECT_ENTER     V3_EJECT_RESPONSE;
typedef V3_EJECT_ENTER     V3_ENTER_RESPONSE;

typedef struct {                        /* eject request */
    V3_REQUEST_HEADER      request_header;         
    CAPID               cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* External tape cartridge label */
} V3_EJECT_REQUEST;

typedef struct {                           /* eject request */
    V3_REQUEST_HEADER      request_header;         
    CAPID               cap_id;            /* CAP used for ejection */
    unsigned short      count;             /* Number of cartridges */
    VOLRANGE            vol_range[MAX_ID]; /* External tape cartridge label */
} V3_EXT_EJECT_REQUEST;

typedef struct {                        /* eject request */
    V3_REQUEST_HEADER      request_header;         
    CAPID               cap_id;         /* CAP used for entry */
} V3_ENTER_REQUEST;

typedef struct {                        /* mount request */
    V3_REQUEST_HEADER      request_header;
    VOLID               vol_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} V3_MOUNT_REQUEST;

typedef struct  {                       /* mount response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V3_MOUNT_RESPONSE;

typedef struct {                        /* mount scratch request */
    V3_REQUEST_HEADER      request_header;
    POOLID              pool_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} V3_MOUNT_SCRATCH_REQUEST;

typedef struct  {                       /* mount scratch response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    DRIVEID             drive_id;
    VOLID               vol_id;
} V3_MOUNT_SCRATCH_RESPONSE;

typedef struct {                        /* dismount request */
    V3_REQUEST_HEADER      request_header;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V3_DISMOUNT_REQUEST;

typedef struct  {                       /* dismount response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V3_DISMOUNT_RESPONSE;

typedef struct {                        /* lock request */
    V3_REQUEST_HEADER      request_header;
    TYPE                type;
    unsigned short      count;
    union {
        VOLID           vol_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
    } identifier;
} V3_LOCK_REQUEST;

typedef V3_LOCK_REQUEST    V3_CLEAR_LOCK_REQUEST;
typedef V3_LOCK_REQUEST    V3_QUERY_LOCK_REQUEST;
typedef V3_LOCK_REQUEST    V3_UNLOCK_REQUEST;

typedef struct {                        /* lock response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        LO_VOL_STATUS   volume_status[MAX_ID];
        LO_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} V3_LOCK_RESPONSE;

typedef V3_LOCK_RESPONSE   V3_CLEAR_LOCK_RESPONSE;
typedef V3_LOCK_RESPONSE   V3_UNLOCK_RESPONSE;

typedef struct {                        /* query_request */
    V3_REQUEST_HEADER      request_header;
    TYPE                type;           /* type of query */
    unsigned short      count;          /* number of identifiers */
    union {                             /* list of homogeneous IDs to query */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        CAPID           cap_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        VOLID           vol_id[MAX_ID];
        MESSAGE_ID      request[MAX_ID];
        PORTID          port_id[MAX_ID];
        POOLID          pool_id[MAX_ID];
    } identifier;
} V3_QUERY_REQUEST;

typedef struct {                        /* query clean status */
    VOLID           vol_id;             /*   volume identifier */
    CELLID          home_location;      /*   cell location of clean cartridge */
    unsigned short  max_use;            /*   number of uses allowed */
    unsigned short  current_use;        /*   current usage level */
    STATUS          status;             /*   status of cleaning cartridge */
} V3_QU_CLN_STATUS;

typedef struct {                        /* query drive status */
    DRIVEID         drive_id;           /*   drive for status              */
    STATE           state;              /*   drive state                   */
    VOLID           vol_id;             /*   volume if STATUS_DRIVE_IN_USE */
    STATUS          status;             /*   drive status                  */
} V3_QU_DRV_STATUS;

typedef struct {                        /* query MOUNT status */
    VOLID           vol_id;             /*   volume for drive proximity list  */
    STATUS          status;             /*   volume status                    */
    unsigned short  drive_count;        /*   number of drive identifiers      */
    V3_QU_DRV_STATUS   drive_status[V3_QU_MAX_DRV_STATUS];
                                        /*   drive list in proximity order    */
} V3_QU_MNT_STATUS;

typedef struct {                        /* query volume status */
    VOLID           vol_id;             /*   volume for status            */
    LOCATION        location_type;      /*   LOCATION_CELL or LOCATION_DRIVE */
    union {                             /*   current location of volume   */
        CELLID      cell_id;            /*     if STATUS_VOLUME_HOME      */
        DRIVEID     drive_id;           /*     if STATUS_VOLUME_IN_DRIVE  */
    } location;                         /*     undefined if none of above */
    STATUS          status;             /*   volume status                */
} V3_QU_VOL_STATUS;

typedef struct {                        /* query scratch status */
    VOLID           vol_id;
    CELLID          home_location;
    POOLID          pool_id;
    STATUS          status;
} V3_QU_SCR_STATUS;

typedef struct {                        /* query mount_scratch status */
    POOLID          pool_id;
    STATUS          status;
    unsigned short  drive_count;
    V3_QU_DRV_STATUS   drive_list[V3_QU_MAX_DRV_STATUS];
} V3_QU_MSC_STATUS;

typedef struct {                        /* query_response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;           /* type of query */
    unsigned short      count;          /* number of identifiers */
    union {                             /* list of IDs queried w/status */
        QU_SRV_STATUS           server_status[MAX_ID];
        QU_ACS_STATUS           acs_status[MAX_ID];
        QU_LSM_STATUS           lsm_status[MAX_ID];
        QU_CAP_STATUS           cap_status[MAX_ID];
        V3_QU_CLN_STATUS           clean_volume_status[MAX_ID];
        V3_QU_DRV_STATUS           drive_status[MAX_ID];
        V3_QU_MNT_STATUS           mount_status[MAX_ID];
        V3_QU_VOL_STATUS           volume_status[MAX_ID];
        QU_PRT_STATUS           port_status[MAX_ID];
        QU_REQ_STATUS           request_status[MAX_ID];
        V3_QU_SCR_STATUS           scratch_status[MAX_ID];
        QU_POL_STATUS           pool_status[MAX_ID];
        V3_QU_MSC_STATUS           mount_scratch_status[MAX_ID];
    } status_response;
} V3_QUERY_RESPONSE;

typedef struct {                        /* query lock response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        QL_VOL_STATUS   volume_status[MAX_ID];
        QL_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} V3_QUERY_LOCK_RESPONSE;

typedef struct {                        /* vary request */
    V3_REQUEST_HEADER      request_header;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                             /* list of homogeneous IDs to vary */
        ACS         acs_id[MAX_ID];
        LSMID       lsm_id[MAX_ID];
        DRIVEID     drive_id[MAX_ID];
        PORTID      port_id[MAX_ID];
        CAPID       cap_id[MAX_ID];
    } identifier;
} V3_VARY_REQUEST;

typedef struct {                        /* vary response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                             /* list of IDs varied w/status */
        VA_ACS_STATUS   acs_status[MAX_ID];
        VA_LSM_STATUS   lsm_status[MAX_ID];
        VA_DRV_STATUS   drive_status[MAX_ID];
        VA_PRT_STATUS   port_status[MAX_ID];
        VA_CAP_STATUS   cap_status[MAX_ID];
    } device_status;
} V3_VARY_RESPONSE;

typedef struct {                                /* venter request */
    V3_REQUEST_HEADER      request_header;         
    CAPID               cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* Virtual tape cartridge label */
} V3_VENTER_REQUEST;

typedef struct {                                /* define pool request */
    V3_REQUEST_HEADER  request_header;
    unsigned long   low_water_mark;
    unsigned long   high_water_mark;
    unsigned long   pool_attributes;
    unsigned short  count;
    POOLID          pool_id[MAX_ID];
} V3_DEFINE_POOL_REQUEST;

typedef struct {                                /* define pool response */
    V3_REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    unsigned long   low_water_mark;
    unsigned long   high_water_mark;
    unsigned long   pool_attributes;
    unsigned short  count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} V3_DEFINE_POOL_RESPONSE;

typedef struct {                                /* delete pool request */
    V3_REQUEST_HEADER  request_header;
    unsigned short  count;
    POOLID          pool_id[MAX_ID];
} V3_DELETE_POOL_REQUEST;

typedef struct {                                /* delete pool response */
    V3_REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    unsigned short  count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} V3_DELETE_POOL_RESPONSE;

typedef struct {                                /* set cap request */
    V3_REQUEST_HEADER  request_header;
    CAP_PRIORITY    cap_priority;
    CAP_MODE        cap_mode;
    unsigned short  count;
    CAPID           cap_id[MAX_ID];
} V3_SET_CAP_REQUEST;

typedef struct {                                /* set cap response */
    V3_REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    CAP_PRIORITY    cap_priority;
    CAP_MODE        cap_mode;
    unsigned short  count;
    struct {
        CAPID           cap_id;
        RESPONSE_STATUS status;
    } set_cap_status[MAX_ID];
} V3_SET_CAP_RESPONSE;

typedef struct {                                /* set cap request */
    V3_REQUEST_HEADER  request_header;
    unsigned short  max_use;
    unsigned short  count;
    VOLRANGE        vol_range[MAX_ID];
} V3_SET_CLEAN_REQUEST;

typedef struct {                                /* set cap response */
    V3_REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    unsigned short  max_use;
    unsigned short  count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } volume_status[MAX_ID];
} V3_SET_CLEAN_RESPONSE;

typedef struct {                                /* set owner request */
    V3_REQUEST_HEADER  request_header;
    USERID          owner_id;
    TYPE            type;
    unsigned short  count;
    VOLRANGE        vol_range[MAX_ID];
} V3_SET_OWNER_REQUEST;

typedef struct {                                /* set owner response */
    V3_REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    USERID          owner_id;
    TYPE            type;
    unsigned short  count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } volume_status[MAX_ID];
} V3_SET_OWNER_RESPONSE;

typedef struct {                                /* set scratch request */
    V3_REQUEST_HEADER  request_header;
    POOLID          pool_id;
    unsigned short  count;
    VOLRANGE        vol_range[MAX_ID];
} V3_SET_SCRATCH_REQUEST;

typedef struct {                                /* set scratch response */
    V3_REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    POOLID          pool_id;
    unsigned short  count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } scratch_status[MAX_ID];
} V3_SET_SCRATCH_RESPONSE;

typedef struct {                                /* cancel request */
    V3_REQUEST_HEADER      request_header;
    MESSAGE_ID          request;
} V3_CANCEL_REQUEST;

typedef struct {                                /* cancel response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          request;
} V3_CANCEL_RESPONSE;

typedef struct {                                /* start request */
    V3_REQUEST_HEADER      request_header;
} V3_START_REQUEST;

typedef struct {                                /* start response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
} V3_START_RESPONSE;

typedef struct {                                /* idle request */
     V3_REQUEST_HEADER     request_header;
} V3_IDLE_REQUEST;

typedef struct {                                /* idle response */
    V3_REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
} V3_IDLE_RESPONSE;

typedef struct {                                /* initialization request */
    V3_REQUEST_HEADER      request_header;
} V3_INIT_REQUEST;

typedef union {                                 /* request type union */
   V3_REQUEST_HEADER        generic_request;
   V3_AUDIT_REQUEST         audit_request;
   V3_ENTER_REQUEST         enter_request;
   V3_VENTER_REQUEST        venter_request;
   V3_EJECT_REQUEST         eject_request;
   V3_EXT_EJECT_REQUEST     ext_eject_request;
   V3_VARY_REQUEST          vary_request;
   V3_MOUNT_REQUEST         mount_request;
   V3_MOUNT_SCRATCH_REQUEST mount_scratch_request;
   V3_DISMOUNT_REQUEST      dismount_request;
   V3_QUERY_REQUEST         query_request;
   V3_CANCEL_REQUEST        cancel_request;
   V3_START_REQUEST         start_request;
   V3_IDLE_REQUEST          idle_request;
   V3_SET_SCRATCH_REQUEST   set_scratch_request;
   V3_DEFINE_POOL_REQUEST   define_pool_request;
   V3_DELETE_POOL_REQUEST   delete_pool_request;
   V3_SET_CLEAN_REQUEST     set_clean_request;
   V3_LOCK_REQUEST          lock_request;
   V3_UNLOCK_REQUEST        unlock_request;
   V3_CLEAR_LOCK_REQUEST    clear_lock_request;
   V3_QUERY_LOCK_REQUEST    query_lock_request;
   V3_SET_CAP_REQUEST       set_cap_request;
} V3_REQUEST_TYPE;

typedef struct {                                /* response header */
   V3_REQUEST_HEADER       request_header;
   RESPONSE_STATUS         response_status;
} V3_RESPONSE_HEADER;

typedef union {                                 /* response type union */
   V3_RESPONSE_HEADER        generic_response;
   V3_ACKNOWLEDGE_RESPONSE   acknowledge_response;
   V3_AUDIT_RESPONSE         audit_response;
   V3_ENTER_RESPONSE         enter_response;
   V3_EJECT_ENTER            eject_enter;
   V3_EJECT_RESPONSE         eject_response;
   V3_VARY_RESPONSE          vary_response;
   V3_MOUNT_RESPONSE         mount_response;
   V3_MOUNT_SCRATCH_RESPONSE mount_scratch_response;
   V3_DISMOUNT_RESPONSE      dismount_response;
   V3_QUERY_RESPONSE         query_response;
   V3_CANCEL_RESPONSE        cancel_response;
   V3_START_RESPONSE         start_response;
   V3_IDLE_RESPONSE          idle_response;
   V3_SET_SCRATCH_RESPONSE   set_scratch_response;
   V3_DEFINE_POOL_RESPONSE   define_pool_response;
   V3_DELETE_POOL_RESPONSE   delete_pool_response;
   V3_SET_CLEAN_RESPONSE     set_clean_response;
   V3_LOCK_RESPONSE          lock_response;
   V3_UNLOCK_RESPONSE        unlock_response;
   V3_CLEAR_LOCK_RESPONSE    clear_lock_response;
   V3_QUERY_LOCK_RESPONSE    query_lock_response;
   V3_SET_CAP_RESPONSE       set_cap_response;
} V3_RESPONSE_TYPE;

#endif /* _V3_STRUCTS_ */

