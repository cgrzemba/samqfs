/* SccsId @(#)csi_v2_structs.h	1.2 1/11/94  */
#ifndef _CSI_V2_STRUCTS_
#define _CSI_V2_STRUCTS_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Definitions of CSI data structures for VERION 2 and VERSION 3 packets.
 *
 *      The structures defined here have corresponding definitions for the
 *      ACSLM in lm_structs.h.  any modifications to this file MUST be
 *      reflected in lm_structs.h as well.
 *
 * Modified by:
 *	E. A. Alongi	03-Jun-93	Original copied from csi_structs.h, R4
 *					version.
 *	Mitch Black	01-Dec-2004	Removed pdaemon structures.
 *
 */

/*      Header Files: */

#ifndef _CSI_V0_STRUCTS_
#include "csi_v0_structs.h"
#endif

#ifndef _V2_STRUCTS_
#include "v2_structs.h"
#endif

/*      Defines, Typedefs and Structure Definitions: */

typedef struct {                        /* fixed portion of request_packet */
    CSI_HEADER          csi_header;
    MESSAGE_HEADER      message_header;
} CSI_V2_REQUEST_HEADER;

typedef struct {                        /* intermediate acknowledgement */
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          message_id;
} CSI_V2_ACKNOWLEDGE_RESPONSE;

/*****************************************************************************
 *                       AUDIT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* audit_request                      */
    CSI_V2_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;         /*   CAP for ejecting cartridges      */
    TYPE                type;           /*   type of identifiers              */
    unsigned short      count;          /*   number of identifiers            */
    union {                             /*   list of homogeneous IDs to audit */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        PANELID         panel_id[MAX_ID];
        SUBPANELID      subpanel_id[MAX_ID];
    } identifier;
} CSI_V2_AUDIT_REQUEST;

typedef struct {                        /* audit_response                  */
    CSI_V2_REQUEST_HEADER  csi_request_header;
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
} CSI_V2_AUDIT_RESPONSE;

typedef struct {                        /* eject_enter intermediate response */
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    CAPID               cap_id;         /*   CAP for ejecting cartridges     */
    unsigned short      count;          /*   no. of volumes ejected/entered  */
    VOLUME_STATUS       volume_status[MAX_ID];
} CSI_V2_EJECT_ENTER;

/*****************************************************************************
 *                       EJECT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* eject request */
    CSI_V2_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* External tape cartridge label */
} CSI_V2_EJECT_REQUEST;

typedef struct {                           /* xeject request */
    CSI_V2_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;            /* CAP used for ejection */
    unsigned short      count;             /* Number of cartridges */
    VOLRANGE            vol_range[MAX_ID]; /* External tape cartridge label */
} CSI_V2_EXT_EJECT_REQUEST;

typedef CSI_V2_EJECT_ENTER CSI_V2_EJECT_RESPONSE;

/*****************************************************************************
 *                       ENTER REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* eject request */
    CSI_V2_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;         /* CAP used for entry */
} CSI_V2_ENTER_REQUEST;

typedef CSI_V2_EJECT_ENTER CSI_V2_ENTER_RESPONSE;

/*****************************************************************************
 *                       MOUNT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    VOLID               vol_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} CSI_V2_MOUNT_REQUEST;

typedef struct  {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_V2_MOUNT_RESPONSE;
 
/*****************************************************************************
 *                       MOUNT SCRATCH REQUEST/RESPONSE STRUCTURES           *
 *****************************************************************************/
 
typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    POOLID              pool_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} CSI_V2_MOUNT_SCRATCH_REQUEST;
 
typedef struct  {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    DRIVEID             drive_id;
    VOLID               vol_id;
} CSI_V2_MOUNT_SCRATCH_RESPONSE;

/*****************************************************************************
 *                       DISMOUNT REQUEST/RESPONSE STRUCTURES                * 
 *****************************************************************************/

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_V2_DISMOUNT_REQUEST;

typedef struct  {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_V2_DISMOUNT_RESPONSE;

/*****************************************************************************
 *                       LOCK REQUEST/RESPONSE STRUCTURES                *
 *****************************************************************************/

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    TYPE                type;
    unsigned short      count;
    union {
        VOLID           vol_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
    } identifier;
} CSI_V2_LOCK_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        LO_VOL_STATUS   volume_status[MAX_ID];
        LO_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} CSI_V2_LOCK_RESPONSE;

/*****************************************************************************
 *                       CLEAR-LOCK REQUEST/RESPONSE STRUCTURES              *
 *****************************************************************************/

typedef CSI_V2_LOCK_REQUEST    CSI_V2_CLEAR_LOCK_REQUEST;

typedef CSI_V2_LOCK_RESPONSE   CSI_V2_CLEAR_LOCK_RESPONSE;



/*****************************************************************************
 *                       QUERY REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* query_request                      */
    CSI_V2_REQUEST_HEADER  csi_request_header;
    TYPE                type;           /*   type of query                    */
    unsigned short      count;          /*   number of identifiers            */
    union {                             /*   list of homogeneous IDs to query */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        CAPID           cap_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        VOLID           vol_id[MAX_ID];
        MESSAGE_ID      request[MAX_ID];
        PORTID          port_id[MAX_ID];
        POOLID          pool_id[MAX_ID];
    } identifier;
} CSI_V2_QUERY_REQUEST;

typedef struct {                        /* query_response                 */
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;           /*   type of query                */
    unsigned short      count;          /*   number of identifiers        */
    union {                             /*   list of IDs queried w/status */
        QU_SRV_STATUS    server_status[MAX_ID];
        QU_ACS_STATUS    acs_status[MAX_ID];
        QU_LSM_STATUS    lsm_status[MAX_ID];
        QU_CAP_STATUS    cap_status[MAX_ID];
        V2_QU_CLN_STATUS clean_volume_status[MAX_ID];
        V2_QU_DRV_STATUS drive_status[MAX_ID];
        V2_QU_MNT_STATUS mount_status[MAX_ID];
        V2_QU_VOL_STATUS volume_status[MAX_ID];
        QU_PRT_STATUS    port_status[MAX_ID];
        QU_REQ_STATUS    request_status[MAX_ID];
        V2_QU_SCR_STATUS scratch_status[MAX_ID];
        QU_POL_STATUS    pool_status[MAX_ID];
        V2_QU_MSC_STATUS mount_scratch_status[MAX_ID];

    } status_response;
} CSI_V2_QUERY_RESPONSE;

/*****************************************************************************
 *                       QUERY_LOCK REQUEST/RESPONSE STRUCTURES               *
 *****************************************************************************/

typedef CSI_V2_LOCK_REQUEST    CSI_V2_QUERY_LOCK_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        QL_VOL_STATUS   volume_status[MAX_ID];
        QL_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} CSI_V2_QUERY_LOCK_RESPONSE;

/*****************************************************************************
 *                       UNLOCK REQUEST/RESPONSE STRUCTURES                *
 *****************************************************************************/

typedef CSI_V2_LOCK_REQUEST    CSI_V2_UNLOCK_REQUEST;

typedef CSI_V2_LOCK_RESPONSE   CSI_V2_UNLOCK_RESPONSE;


/*****************************************************************************
 *                       VARY REQUEST/RESPONSE STRUCTURES                    * 
 *****************************************************************************/

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                             /*   list of homogeneous IDs to vary */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        CAPID           cap_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        PORTID          port_id[MAX_ID];
    } identifier;
} CSI_V2_VARY_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                             /*   list of IDs varied w/status */
        VA_ACS_STATUS   acs_status[MAX_ID];
        VA_LSM_STATUS   lsm_status[MAX_ID];
        VA_CAP_STATUS   cap_status[MAX_ID];
        VA_DRV_STATUS   drive_status[MAX_ID];
        VA_PRT_STATUS   port_status[MAX_ID];
    } device_status;
} CSI_V2_VARY_RESPONSE;

/*****************************************************************************
 *                       VENTER REQUEST STRUCTURE                            *
 *****************************************************************************/
 
typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* Virtual tape cartridge label */
} CSI_V2_VENTER_REQUEST;
 
/*****************************************************************************
 *                  DEFINE_POOL REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/
 
typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    unsigned long       low_water_mark;
    unsigned long       high_water_mark;
    unsigned long       pool_attributes;
    unsigned short      count;
    POOLID              pool_id[MAX_ID];
} CSI_V2_DEFINE_POOL_REQUEST;
 
typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    unsigned long       low_water_mark;
    unsigned long       high_water_mark;
    unsigned long       pool_attributes;
    unsigned short      count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} CSI_V2_DEFINE_POOL_RESPONSE;
 
/*****************************************************************************
 *                  DELETE_POOL REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/
 
typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    unsigned short      count;
    POOLID              pool_id[MAX_ID];
} CSI_V2_DELETE_POOL_REQUEST;
 
typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    unsigned short      count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} CSI_V2_DELETE_POOL_RESPONSE;
 

/*****************************************************************************
 *                  SET_CAP REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/
typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    CAP_PRIORITY        cap_priority;
    CAP_MODE            cap_mode;
    unsigned short      count;
    CAPID               cap_id[MAX_ID];
} CSI_V2_SET_CAP_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  request_header;
    RESPONSE_STATUS     message_status;
    CAP_PRIORITY        cap_priority;
    CAP_MODE            cap_mode;
    unsigned short      count;
    struct {
        CAPID           cap_id;
        RESPONSE_STATUS status;
    } set_cap_status[MAX_ID];
} CSI_V2_SET_CAP_RESPONSE;

/*****************************************************************************
 *                  SET_CLEAN REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    unsigned short      max_use;
    unsigned short      count;
    VOLRANGE            vol_range[MAX_ID];
} CSI_V2_SET_CLEAN_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    unsigned short      max_use;
    unsigned short      count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } volume_status[MAX_ID];
} CSI_V2_SET_CLEAN_RESPONSE;

/*****************************************************************************
 *                  SET_SCRATCH REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/
 
typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    POOLID              pool_id;
    unsigned short      count;
    VOLRANGE            vol_range[MAX_ID];
} CSI_V2_SET_SCRATCH_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    unsigned short      count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } scratch_status[MAX_ID];
} CSI_V2_SET_SCRATCH_RESPONSE;

/*****************************************************************************
 *                       CANCEL REQUEST/RESPONSE STRUCTURES                  * 
 *****************************************************************************/

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    MESSAGE_ID          request;
} CSI_V2_CANCEL_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          request;
} CSI_V2_CANCEL_RESPONSE;

/*****************************************************************************
 *                       START REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
} CSI_V2_START_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
} CSI_V2_START_RESPONSE;

/*****************************************************************************
 *                       IDLE REQUEST/RESPONSE STRUCTURES                    *
 *****************************************************************************/

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
} CSI_V2_IDLE_REQUEST;

typedef struct {
    CSI_V2_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
} CSI_V2_IDLE_RESPONSE;


/*
 *      Procedure Type Declarations:
 */

#endif /* _CSI_V2_STRUCTS_ */

