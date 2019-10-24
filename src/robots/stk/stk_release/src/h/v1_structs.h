/* SccsId @(#)v1_structs.h	1.2 1/11/94  */
#ifndef _V1_STRUCTS_
#define _V1_STRUCTS_
/*
 * Copyright (1990, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      definitions of Version1 specific system data structures.  
 *      most of these structures are common to csi and acslm, 
 *      and are included by both header files.
 *
 * Considerations:
 *
 *      Make sure that the routine which includes this file
 *      includes "lm_structs.h" before including this one.
 *
 * Modified by:
 *
 *      H. I. Grapek    20-Jun-1991     stolen from r2 (VERSION1),
 *                              conglomerate of stuff from the files
 *                              acslm.h lm_structs.h and structs.h
 *      H. I. Grapek    13-Sep-1991     Changes needed for query...
 *                              Removed include of identifier.h and db_structs.h
 *                              just make sure that the routine which calls this one
 *                              includes lm_structs before this one.
 *      Alec Sharp      26-Jun-1992     Added V1_EJECT_ENTER to V1_RESPONSE_TYPE
 *	Emanuel Alongi  07-Jul-1993	Because of changes to query response
 *				structures, added structure definitions for
 *				V1_QU_CLN_STATUS, V1_QU_DRV_STATUS, V1_QU_MNT_
 *				STATUS, V1_QU_VOL_STATUS, V1_QU_SCR_STATUS,
 *				V1_QU_MSC_STATUS to preserve v1 structures.
 *	E. A. Alongi    17-Sep-1993.    Added define for V1_MAX_ACS_DRIVES.
 *	E. A. Alongi    17-Sep-1993.    Use V1_MAX_ACS_DRIVES as dimension
 *					in query mount and mount scratch status
 *					structures.
 *      J. Borzuchowski 17-Sep-1993.    R5.0 Mixed Media-- Use V1_QU_DRV_STATUS
 *				in declaration of drive status structs in
 *				V1_QU_MNT_STATUS and V1_QU_MSC_STATUS.
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
#define V1_MAX_ACS_DRIVES   128         

typedef struct {                /* fixed portion of request_packet */
    IPC_HEADER          ipc_header;
    MESSAGE_HEADER      message_header;
} V1_REQUEST_HEADER;

typedef struct {                /* intermediate acknowledgement */
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          message_id;
} V1_ACKNOWLEDGE_RESPONSE;

typedef struct {                /* audit_request */
    V1_REQUEST_HEADER   request_header;
    V1_CAPID            cap_id;         /* CAP for ejecting cartridges      */
    TYPE                type;           /* type of identifiers              */
    unsigned short      count;          /* number of identifiers            */
    union {                             /* list of homogeneous IDs to audit */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        PANELID         panel_id[MAX_ID];
        SUBPANELID      subpanel_id[MAX_ID];
    } identifier;
} V1_AUDIT_REQUEST;

typedef struct {                /* audit_response  */
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    V1_CAPID            cap_id;         /* CAP for ejecting cartridges   */
    TYPE                type;           /* type of identifiers */
    unsigned short      count;          /* number of audited identifiers */
    union {                             /* list of IDs audited w/status  */
        AU_ACS_STATUS   acs_status[MAX_ID];
        AU_LSM_STATUS   lsm_status[MAX_ID];
        AU_PNL_STATUS   panel_status[MAX_ID];
        AU_SUB_STATUS   subpanel_status[MAX_ID];
    } identifier_status;
} V1_AUDIT_RESPONSE;

typedef struct {                /* eject_enter intermediate response */
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    V1_CAPID            cap_id;         /* CAP for ejecting cartridges     */
    unsigned short      count;          /* no. of volumes ejected/entered  */
    VOLUME_STATUS       volume_status[MAX_ID];
} V1_EJECT_ENTER;

typedef struct {                /* eject request */
    V1_REQUEST_HEADER   request_header;
    V1_CAPID            cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* External tape cartridge label */
} V1_EJECT_REQUEST;

typedef struct {                /* range eject request */
    V1_REQUEST_HEADER   request_header;
    V1_CAPID            cap_id;                 /* CAP used for ejection */
    unsigned short      count;                  /* Number of cartridges */
    VOLRANGE            vol_range[MAX_ID];      /* Ext tape cartridge label */
} V1_EXT_EJECT_REQUEST;

typedef V1_EJECT_ENTER V1_EJECT_RESPONSE;

typedef struct {                /* eject request */
    V1_REQUEST_HEADER   request_header;
    V1_CAPID            cap_id;         /* CAP used for entry */
} V1_ENTER_REQUEST;

typedef V1_EJECT_ENTER V1_ENTER_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    VOLID               vol_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} V1_MOUNT_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V1_MOUNT_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    POOLID              pool_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} V1_MOUNT_SCRATCH_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    DRIVEID             drive_id;
    VOLID               vol_id;
} V1_MOUNT_SCRATCH_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V1_DISMOUNT_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V1_DISMOUNT_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    TYPE                type;
    unsigned short      count;
    union {
        VOLID           vol_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
    } identifier;
} V1_LOCK_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        LO_VOL_STATUS   volume_status[MAX_ID];
        LO_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} V1_LOCK_RESPONSE;

typedef V1_LOCK_REQUEST V1_CLEAR_LOCK_REQUEST;

typedef V1_LOCK_RESPONSE V1_CLEAR_LOCK_RESPONSE;

typedef struct {                /* query_request  */
    V1_REQUEST_HEADER   request_header;
    TYPE                type;           /* type of query */
    unsigned short      count;          /* number of identifiers */
    union {                     /* list of homogeneous IDs to query */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        V1_CAPID        cap_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        VOLID           vol_id[MAX_ID];
        MESSAGE_ID      request[MAX_ID];
        PORTID          port_id[MAX_ID];
        POOLID          pool_id[MAX_ID];
    } identifier;
} V1_QUERY_REQUEST;

typedef struct {                /* CAP status (one/cap_id)  */
    V1_CAPID            cap_id;         /* CAP for status         */
    STATUS              status;         /* CAP status             */
    CAP_PRIORITY        cap_priority;   /* CAP priority number    */
    unsigned short      cap_size;       /* number of cells in CAP */
} V1_QU_CAP_STATUS;

typedef struct {                        /* query clean status */
    VOLID           vol_id;             /*   volume identifier */
    CELLID          home_location;      /*   cell location of clean cartridge */
    unsigned short  max_use;            /*   number of uses allowed */
    unsigned short  current_use;        /*   current usage level */
    STATUS          status;             /*   status of cleaning cartridge */
} V1_QU_CLN_STATUS;

typedef struct {                        /* query drive status */
    DRIVEID         drive_id;           /*   drive for status              */
    STATE           state;              /*   drive state                   */
    VOLID           vol_id;             /*   volume if STATUS_DRIVE_IN_USE */
    STATUS          status;             /*   drive status                  */
} V1_QU_DRV_STATUS;

typedef struct {                        /* query MOUNT status */
    VOLID           vol_id;             /*   volume for drive proximity list  */
    STATUS          status;             /*   volume status                    */
    unsigned short  drive_count;        /*   number of drive identifiers      */
    V1_QU_DRV_STATUS   drive_status[V1_MAX_ACS_DRIVES];
                                        /*   drive list in proximity order    */
} V1_QU_MNT_STATUS;

typedef struct {                        /* query volume status */
    VOLID           vol_id;             /*   volume for status            */
    LOCATION        location_type;      /*   LOCATION_CELL or LOCATION_DRIVE */
    union {                             /*   current location of volume   */
        CELLID      cell_id;            /*     if STATUS_VOLUME_HOME      */
        DRIVEID     drive_id;           /*     if STATUS_VOLUME_IN_DRIVE  */
    } location;                         /*     undefined if none of above */
    STATUS          status;             /*   volume status                */
} V1_QU_VOL_STATUS;

typedef struct {                        /* query scratch status */
    VOLID           vol_id;
    CELLID          home_location;
    POOLID          pool_id;
    STATUS          status;
} V1_QU_SCR_STATUS;

typedef struct {                        /* query mount_scratch status */
    POOLID          pool_id;
    STATUS          status;
    unsigned short  drive_count;
    V1_QU_DRV_STATUS   drive_list[V1_MAX_ACS_DRIVES];
} V1_QU_MSC_STATUS;

typedef struct {                /* query_response  */
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;           /* type of query */
    unsigned short      count;          /* # identifiers */
    union {                     /* list of IDs queried w/status */
        QU_SRV_STATUS    server_status[MAX_ID];
        QU_ACS_STATUS    acs_status[MAX_ID];
        QU_LSM_STATUS    lsm_status[MAX_ID];
        V1_QU_CAP_STATUS cap_status[MAX_ID];
        V1_QU_CLN_STATUS    clean_volume_status[MAX_ID];
        V1_QU_DRV_STATUS    drive_status[MAX_ID];
        V1_QU_MNT_STATUS    mount_status[MAX_ID];
        V1_QU_VOL_STATUS    volume_status[MAX_ID];
        QU_PRT_STATUS    port_status[MAX_ID];
        QU_REQ_STATUS    request_status[MAX_ID];
        V1_QU_SCR_STATUS    scratch_status[MAX_ID];
        QU_POL_STATUS    pool_status[MAX_ID];
        V1_QU_MSC_STATUS    mount_scratch_status[MAX_ID];
    } status_response;
} V1_QUERY_RESPONSE;

typedef V1_LOCK_REQUEST V1_QUERY_LOCK_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        QL_VOL_STATUS   volume_status[MAX_ID];
        QL_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} V1_QUERY_LOCK_RESPONSE;

typedef V1_LOCK_REQUEST V1_UNLOCK_REQUEST;

typedef V1_LOCK_RESPONSE V1_UNLOCK_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                     /* list of homogeneous IDs to vary */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        PORTID          port_id[MAX_ID];
    } identifier;
} V1_VARY_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                     /* list of IDs varied w/status */
        VA_ACS_STATUS   acs_status[MAX_ID];
        VA_LSM_STATUS   lsm_status[MAX_ID];
        VA_DRV_STATUS   drive_status[MAX_ID];
        VA_PRT_STATUS   port_status[MAX_ID];
    } device_status;
} V1_VARY_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    V1_CAPID            cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* Virtual tape cartridge label */
} V1_VENTER_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    unsigned long       low_water_mark;
    unsigned long       high_water_mark;
    unsigned long       pool_attributes;
    unsigned short      count;
    POOLID              pool_id[MAX_ID];
} V1_DEFINE_POOL_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    unsigned long       low_water_mark;
    unsigned long       high_water_mark;
    unsigned long       pool_attributes;
    unsigned short      count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} V1_DEFINE_POOL_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    unsigned short      count;
    POOLID              pool_id[MAX_ID];
} V1_DELETE_POOL_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    unsigned short      count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} V1_DELETE_POOL_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    CAP_PRIORITY        cap_priority;
    unsigned short      count;
    V1_CAPID            cap_id[MAX_ID];
} V1_SET_CAP_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    CAP_PRIORITY        cap_priority;
    unsigned short      count;
    struct {
        V1_CAPID        cap_id;
        RESPONSE_STATUS status;
    } set_cap_status[MAX_ID];
} V1_SET_CAP_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    unsigned short      max_use;
    unsigned short      count;
    VOLRANGE            vol_range[MAX_ID];
} V1_SET_CLEAN_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    unsigned short      max_use;
    unsigned short      count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } volume_status[MAX_ID];
} V1_SET_CLEAN_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    POOLID              pool_id;
    unsigned short      count;
    VOLRANGE            vol_range[MAX_ID];
} V1_SET_SCRATCH_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    unsigned short      count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } scratch_status[MAX_ID];
} V1_SET_SCRATCH_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    MESSAGE_ID          request;
} V1_CANCEL_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          request;
} V1_CANCEL_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
} V1_START_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
} V1_START_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
} V1_IDLE_REQUEST;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
} V1_IDLE_RESPONSE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
} V1_INIT_REQUEST;

typedef union {
    V1_REQUEST_HEADER           generic_request;
    V1_AUDIT_REQUEST            audit_request;
    V1_ENTER_REQUEST            enter_request;
    V1_VENTER_REQUEST           venter_request;
    V1_EJECT_REQUEST            eject_request;
    V1_EXT_EJECT_REQUEST        ext_eject_request;
    V1_VARY_REQUEST             vary_request;
    V1_MOUNT_REQUEST            mount_request;
    V1_MOUNT_SCRATCH_REQUEST    mount_scratch_request;
    V1_DISMOUNT_REQUEST         dismount_request;
    V1_QUERY_REQUEST            query_request;
    V1_CANCEL_REQUEST           cancel_request;
    V1_START_REQUEST            start_request;
    V1_IDLE_REQUEST             idle_request;
    V1_SET_SCRATCH_REQUEST      set_scratch_request;
    V1_DEFINE_POOL_REQUEST      define_pool_request;
    V1_DELETE_POOL_REQUEST      delete_pool_request;
    V1_SET_CLEAN_REQUEST        set_clean_request;
    V1_LOCK_REQUEST             lock_request;
    V1_UNLOCK_REQUEST           unlock_request;
    V1_CLEAR_LOCK_REQUEST       clear_lock_request;
    V1_QUERY_LOCK_REQUEST       query_lock_request;
    V1_SET_CAP_REQUEST          set_cap_request;
} V1_REQUEST_TYPE;

typedef struct {
    V1_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     response_status;
} V1_RESPONSE_HEADER;

typedef union {
    V1_RESPONSE_HEADER          generic_response;
    V1_ACKNOWLEDGE_RESPONSE     acknowledge_response;
    V1_AUDIT_RESPONSE           audit_response;
    V1_ENTER_RESPONSE           enter_response;
    V1_EJECT_ENTER              eject_enter;
    V1_EJECT_RESPONSE           eject_response;
    V1_VARY_RESPONSE            vary_response;
    V1_MOUNT_RESPONSE           mount_response;
    V1_MOUNT_SCRATCH_RESPONSE   mount_scratch_response;
    V1_DISMOUNT_RESPONSE        dismount_response;
    V1_QUERY_RESPONSE           query_response;
    V1_CANCEL_RESPONSE          cancel_response;
    V1_START_RESPONSE           start_response;
    V1_IDLE_RESPONSE            idle_response;
    V1_SET_SCRATCH_RESPONSE     set_scratch_response;
    V1_DEFINE_POOL_RESPONSE     define_pool_response;
    V1_DELETE_POOL_RESPONSE     delete_pool_response;
    V1_SET_CLEAN_RESPONSE       set_clean_response;
    V1_LOCK_RESPONSE            lock_response;
    V1_UNLOCK_RESPONSE          unlock_response;
    V1_CLEAR_LOCK_RESPONSE      clear_lock_response;
    V1_QUERY_LOCK_RESPONSE      query_lock_response;
    V1_SET_CAP_RESPONSE         set_cap_response;
} V1_RESPONSE_TYPE;

#endif /* _V1_STRUCTS_ */

