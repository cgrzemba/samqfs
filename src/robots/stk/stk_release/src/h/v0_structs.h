/* SccsId @(#)v0_structs.h	1.2 1/11/94  */
#ifndef _V0_STRUCTS_
#define _V0_STRUCTS_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header is intended for use within the scope of the
 *      ACSLM process and any child processes needing such information.
 *
 *      This is a conglomeration of all the release 1 specific information
 *
 * Considerations:
 *
 *      Make sure that the routine which includes this file
 *      includes "lm_structs.h" before including this one.
 *
 * Modified by:
 *
 *      H. I. Grapek        07-Jul-1990     stolen from r1, comglomerate of
 *                              stuff from the files acslm.h lm_structs.h and
 *                              structs.h
 *      H. I. Grapek    31-Jul-1991     Added V0_CAPID def.
 *                                      Added V0_MESSAGE_HEADER
 *      H. I. Grapek    23-Sept-1991    mods needed for query
 *                              removed V0_CAPID def.
 *                              added V0_QU_CAP_STATUS
 *                              Removed include of identifier.h and db_structs.h
 *                              just make sure that the routine which calls this
 *                              one includes lm_structs before this one.
 *	Emanuel Alongi  07-Jul-1993	Because of changes in R5.0 query 
 *				response statuses, modified V0_QUERY_RESPONSE
 *				structure and added definitions for
 *				V0_QU_DRV_STATUS, V0_QU_MNT_STATUS, and
 *				V0_QU_VOL_STATUS.
 *	E. A. Alongi    17-Sep-1993.    Added define for V0_MAX_ACS_DRIVES.
 *	E. A. Alongi    17-Sep-1993.    Use V0_MAX_ACS_DRIVES as dimension
 *					in query mount status structure.
 */

/*
 *      Header Files:
 */

#ifndef _STRUCTS_
#include "structs.h"
#endif

/* 
 * maximum number of drive_status entries in mount query response.
 */
#define V0_MAX_ACS_DRIVES   128         

/*
 *      Defines, Typedefs and Structure Definitions:
 */

typedef struct {
    unsigned short      packet_id;      /* client-specified */
    COMMAND             command;
    unsigned char       message_options;
} V0_MESSAGE_HEADER;

typedef struct {                        /* fixed portion of request_packet */
    IPC_HEADER          ipc_header;
    V0_MESSAGE_HEADER   message_header;
} V0_REQUEST_HEADER;

typedef struct {                        /* intermediate acknowledgement */
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          message_id;
} V0_ACKNOWLEDGE_RESPONSE;


typedef struct {                        /* audit_request  */
    V0_REQUEST_HEADER   request_header;
    V0_CAPID            cap_id;         /* CAP for ejecting cartridges  */
    TYPE                type;           /* type of identifiers  */
    unsigned short      count;          /* number of identifiers */
    union {                             /* list of homogeneous IDs to audit */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        PANELID         panel_id[MAX_ID];
        SUBPANELID      subpanel_id[MAX_ID];
    } identifier;
} V0_AUDIT_REQUEST;

typedef struct {                        /* audit_response  */
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    V0_CAPID            cap_id;         /* CAP for ejecting cartridges   */
    TYPE                type;           /* type of identifiers           */
    unsigned short      count;          /* number of audited identifiers */
    union {                             /* list of IDs audited w/status  */
        AU_ACS_STATUS   acs_status[MAX_ID];
        AU_LSM_STATUS   lsm_status[MAX_ID];
        AU_PNL_STATUS   panel_status[MAX_ID];
        AU_SUB_STATUS   subpanel_status[MAX_ID];
    } identifier_status;
} V0_AUDIT_RESPONSE;

typedef struct {                        /* eject_enter intermediate response */
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    V0_CAPID            cap_id;         /* CAP for ejecting cartridges     */
    unsigned short      count;          /* no. of volumes ejected/entered  */
    VOLUME_STATUS       volume_status[MAX_ID];
} V0_EJECT_ENTER;

typedef struct {                        /* eject request */
    V0_REQUEST_HEADER   request_header;
    V0_CAPID            cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* External tape cartridge label */
} V0_EJECT_REQUEST;

typedef V0_EJECT_ENTER V0_EJECT_RESPONSE;

typedef struct {                        /* eject request */
    V0_REQUEST_HEADER   request_header;
    V0_CAPID            cap_id;         /* CAP used for entry */
} V0_ENTER_REQUEST;

typedef V0_EJECT_ENTER V0_ENTER_RESPONSE;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    VOLID               vol_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} V0_MOUNT_REQUEST;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V0_MOUNT_RESPONSE;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V0_DISMOUNT_REQUEST;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} V0_DISMOUNT_RESPONSE;

/* 
 * query_request                      
 */
typedef struct {                
    V0_REQUEST_HEADER   request_header;
    TYPE                type;           /* type of query */
    unsigned short      count;          /* number of identifiers */
    union {                             /* list of homogeneous IDs to query */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        V0_CAPID        cap_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        VOLID           vol_id[MAX_ID];
        MESSAGE_ID      request[MAX_ID];
        PORTID          port_id[MAX_ID];
    } identifier;
} V0_QUERY_REQUEST;

typedef struct {                        /* CAP status (one/cap_id) */
    V0_CAPID            cap_id;         /* CAP for status */
    STATUS              status;         /* CAP status */
} V0_QU_CAP_STATUS;

typedef struct {                        /* query drive status */
    DRIVEID         drive_id;           /*   drive for status              */
    STATE           state;              /*   drive state                   */
    VOLID           vol_id;             /*   volume if STATUS_DRIVE_IN_USE */
    STATUS          status;             /*   drive status                  */
} V0_QU_DRV_STATUS;

typedef struct {                        /* query MOUNT status */
    VOLID            vol_id;            /*   volume for drive proximity list  */
    STATUS           status;            /*   volume status                    */
    unsigned short   drive_count;       /*   number of drive identifiers      */
    V0_QU_DRV_STATUS drive_status[V0_MAX_ACS_DRIVES];
                                        /*   drive list in proximity order    */
} V0_QU_MNT_STATUS;

typedef struct {                        /* query volume status */
    VOLID           vol_id;             /*   volume for status            */
    LOCATION        location_type;      /*   LOCATION_CELL or LOCATION_DRIVE */
    union {                             /*   current location of volume   */
        CELLID      cell_id;            /*     if STATUS_VOLUME_HOME      */
        DRIVEID     drive_id;           /*     if STATUS_VOLUME_IN_DRIVE  */
    } location;                         /*     undefined if none of above */
    STATUS          status;             /*   volume status                */
} V0_QU_VOL_STATUS;

/* 
 * query_response                 
 */
typedef struct {                
    V0_REQUEST_HEADER    request_header;
    RESPONSE_STATUS      message_status;
    TYPE                 type;          /* type of query */
    unsigned short       count;         /* number of identifiers */
    union {                             /* list of IDs queried w/status */
        QU_SRV_STATUS    server_status[MAX_ID];
        QU_ACS_STATUS    acs_status[MAX_ID];
        QU_LSM_STATUS    lsm_status[MAX_ID];
        V0_QU_CAP_STATUS cap_status[MAX_ID];
        V0_QU_DRV_STATUS drive_status[MAX_ID];
        V0_QU_MNT_STATUS mount_status[MAX_ID];
        V0_QU_VOL_STATUS volume_status[MAX_ID];
        QU_PRT_STATUS    port_status[MAX_ID];
        QU_REQ_STATUS    request_status[MAX_ID];
    } status_response;
} V0_QUERY_RESPONSE;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                     /* list of homogeneous IDs to vary */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        PORTID          port_id[MAX_ID];
    } identifier;
} V0_VARY_REQUEST;

typedef struct {
    V0_REQUEST_HEADER   request_header;
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
} V0_VARY_RESPONSE;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    MESSAGE_ID          request;
} V0_CANCEL_REQUEST;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          request;
} V0_CANCEL_RESPONSE;

typedef struct {
    V0_REQUEST_HEADER   request_header;
} V0_START_REQUEST;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
} V0_START_RESPONSE;

typedef struct {
    V0_REQUEST_HEADER   request_header;
} V0_IDLE_REQUEST;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     message_status;
} V0_IDLE_RESPONSE;

typedef union {
    V0_REQUEST_HEADER   generic_request;
    V0_AUDIT_REQUEST    audit_request;
    V0_ENTER_REQUEST    enter_request;
    V0_EJECT_REQUEST    eject_request;
    V0_VARY_REQUEST     vary_request;
    V0_MOUNT_REQUEST    mount_request;
    V0_DISMOUNT_REQUEST dismount_request;
    V0_QUERY_REQUEST    query_request;
    V0_CANCEL_REQUEST   cancel_request;
    V0_START_REQUEST    start_request;
    V0_IDLE_REQUEST     idle_request;
} V0_REQUEST_TYPE;

typedef struct {
    V0_REQUEST_HEADER   request_header;
    RESPONSE_STATUS     response_status;
} V0_RESPONSE_HEADER;

typedef union {
    V0_RESPONSE_HEADER      generic_response;
    V0_ACKNOWLEDGE_RESPONSE acknowledge_response;
    V0_AUDIT_RESPONSE       audit_response;
    V0_ENTER_RESPONSE       enter_response;
    V0_EJECT_RESPONSE       eject_response;
    V0_VARY_RESPONSE        vary_response;
    V0_MOUNT_RESPONSE       mount_response;
    V0_DISMOUNT_RESPONSE    dismount_response;
    V0_QUERY_RESPONSE       query_response;
    V0_CANCEL_RESPONSE      cancel_response;
    V0_START_RESPONSE       start_response;
    V0_IDLE_RESPONSE        idle_response;
} V0_RESPONSE_TYPE;

#endif /* _V0_STRUCTS_ */

