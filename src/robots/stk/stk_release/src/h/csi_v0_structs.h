/* SccsId @(#)csi_v0_structs.h	2.0 1/11/94  */
#ifndef _CSI_V0_STRUCTS_
#define _CSI_V0_STRUCTS_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Definitions of CSI data structures for release 1 (version 0).
 *
 *      The structures defined here have corresponding definitions for the
 *      ACSLM in lm_structs.h.  Any modifications to this file MUST be
 *      reflected in lm_structs.h as well.
 *
 * Modified by:
 *
 *      D. F. Reed       01-29-89     Original.
 *      J. A. Wishner    01-30-89     Added definitions for CSI_HEADER.
 *      J. A. Wishner    05-01-89     TIME STAMP-POST CUSTOMER INITIAL RELEASE
 *                                    Any changes after this date must appear
 *                                    below this line.
 *      D. F. Reed       05-08-89     Change mount request drive_id dimension
 *                                    to  MAX_ID.
 *      R. P. Cushman    04-20-90     Added CSI_ACSPD_REQUEST 
 *                                    CSI_ACSPD_RESPONSE and 
 *                                    #include pdaemon.h
 *      J. W. Montgomery 15-Jun-90.   Version 2.
 *      H. I. Grapek     28-Aug-91    Added for release 3 (version 1)
 *                                    compatibiltiy data structures for
 *                                    cap identifiers between version.  This
 *                                    was necessary since new cap identifier
 *                                    was in the identifier (RESPONSE_STATUS)
 *                                    portions of a packet.  Changes are:
 *                                    CAPID -> V0_CAPID,
 *                                    MESSAGE_HEADER->V0_MESSAGE_HEADER.
 *                                    REQUEST_HEADER->V0_REQUEST_HEADER,
 *                                    Definition of MAX_ACS_DRIVES.
 *      J. A. Wishner    03-Oct-91    Completed release 3 (version) mods.
 *	E. A. Alongi	 17-Sep-93.   Eliminated define for MAX_ACS_DRIVES.  The
 *				      analogous define is V0_MAX_ACS_DRIVES 
 *				      located in v0_structs.h.
 *	E. A. Alongi	 20-Oct-93.   Modified V0_QUERY_RESPONSE structure by
 *				      changing the previous structure references
 *				      to V0_QU_DRV_STATUS, V0_QU_MNT_STATUS and
 *				      V0_QU_VOL_STATUS.
 *	Mitch Black	 01-Dec-04    Remove pdaemon structures and H file.
 */

/*      Header Files: */

#ifndef _CSI_HEADER_
#include "csi_header.h"
#endif

#ifndef _V0_STRUCTS_
#include "v0_structs.h"
#endif

/*      Defines, Typedefs and Structure Definitions: */

typedef struct {                        /* fixed portion of request_packet */
    CSI_HEADER          csi_header;
    V0_MESSAGE_HEADER   message_header;
} CSI_V0_REQUEST_HEADER;

typedef struct {                        /* intermediate acknowledgement */
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          message_id;
} CSI_V0_ACKNOWLEDGE_RESPONSE;

/*****************************************************************************
 *                       AUDIT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* audit_request                      */
    CSI_V0_REQUEST_HEADER       csi_request_header;
    V0_CAPID            cap_id;         /*   CAP for ejecting cartridges      */
    TYPE                type;           /*   type of identifiers              */
    unsigned short      count;          /*   number of identifiers            */
    union {                             /*   list of homogeneous IDs to audit */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        PANELID         panel_id[MAX_ID];
        SUBPANELID      subpanel_id[MAX_ID];
    } identifier;
} CSI_V0_AUDIT_REQUEST;

typedef struct {                        /* audit_response                  */
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
    V0_CAPID            cap_id;         /*   CAP for ejecting cartridges   */
    TYPE                type;           /*   type of identifiers           */
    unsigned short      count;          /*   number of audited identifiers */
    union {                             /*   list of IDs audited w/status  */
        AU_ACS_STATUS   acs_status[MAX_ID];
        AU_LSM_STATUS   lsm_status[MAX_ID];
        AU_PNL_STATUS   panel_status[MAX_ID];
        AU_SUB_STATUS   subpanel_status[MAX_ID];
    } identifier_status;
} CSI_V0_AUDIT_RESPONSE;

typedef struct {                        /* eject_enter intermediate response */
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
    V0_CAPID            cap_id;         /*   CAP for ejecting cartridges     */
    unsigned short      count;          /*   no. of volumes ejected/entered  */
    VOLUME_STATUS       volume_status[MAX_ID];
} CSI_V0_EJECT_ENTER;

/*****************************************************************************
 *                       EJECT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* eject request */
    CSI_V0_REQUEST_HEADER       csi_request_header;
    V0_CAPID            cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* External tape cartridge label */
} CSI_V0_EJECT_REQUEST;

typedef CSI_V0_EJECT_ENTER      CSI_V0_EJECT_RESPONSE;

/*****************************************************************************
 *                       ENTER REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* eject request */
    CSI_V0_REQUEST_HEADER       csi_request_header;
    V0_CAPID                    cap_id;         /* CAP used for entry */
} CSI_V0_ENTER_REQUEST;

typedef CSI_V0_EJECT_ENTER      CSI_V0_ENTER_RESPONSE;

/*****************************************************************************
 *                       MOUNT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    VOLID               vol_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} CSI_V0_MOUNT_REQUEST;

typedef struct  {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_V0_MOUNT_RESPONSE;

/*****************************************************************************
 *                       DISMOUNT REQUEST/RESPONSE STRUCTURES                * 
 *****************************************************************************/

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_V0_DISMOUNT_REQUEST;

typedef struct  {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_V0_DISMOUNT_RESPONSE;

/*****************************************************************************
 *                       QUERY REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* query_request                      */
    CSI_V0_REQUEST_HEADER       csi_request_header;
    TYPE                type;           /*   type of query                    */
    unsigned short      count;          /*   number of identifiers            */
    union {                             /*   list of homogeneous IDs to query */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        V0_CAPID        cap_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        VOLID           vol_id[MAX_ID];
        MESSAGE_ID      request[MAX_ID];
        PORTID          port_id[MAX_ID];
    } identifier;
} CSI_V0_QUERY_REQUEST;

typedef struct {                        /* query_response                 */
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;           /*   type of query                */
    unsigned short      count;          /*   number of identifiers        */
    union {                             /*   list of IDs queried w/status */
        QU_SRV_STATUS      server_status[MAX_ID];
        QU_ACS_STATUS      acs_status[MAX_ID];
        QU_LSM_STATUS      lsm_status[MAX_ID];
        V0_QU_CAP_STATUS   cap_status[MAX_ID];
        V0_QU_DRV_STATUS   drive_status[MAX_ID];
        V0_QU_MNT_STATUS   mount_status[MAX_ID];
        V0_QU_VOL_STATUS   volume_status[MAX_ID];
        QU_PRT_STATUS      port_status[MAX_ID];
        QU_REQ_STATUS      request_status[MAX_ID];
    } status_response;
} CSI_V0_QUERY_RESPONSE;

/*****************************************************************************
 *                       VARY REQUEST/RESPONSE STRUCTURES                    * 
 *****************************************************************************/

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                             /*   list of homogeneous IDs to vary */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
        PORTID          port_id[MAX_ID];
    } identifier;
} CSI_V0_VARY_REQUEST;

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                             /*   list of IDs varied w/status */
        VA_ACS_STATUS   acs_status[MAX_ID];
        VA_LSM_STATUS   lsm_status[MAX_ID];
        VA_DRV_STATUS   drive_status[MAX_ID];
        VA_PRT_STATUS   port_status[MAX_ID];
    } device_status;
} CSI_V0_VARY_RESPONSE;

/*****************************************************************************
 *                       CANCEL REQUEST/RESPONSE STRUCTURES                  * 
 *****************************************************************************/

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    MESSAGE_ID          request;
} CSI_V0_CANCEL_REQUEST;

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          request;
} CSI_V0_CANCEL_RESPONSE;

/*****************************************************************************
 *                       START REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
} CSI_V0_START_REQUEST;

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
} CSI_V0_START_RESPONSE;

/*****************************************************************************
 *                       IDLE REQUEST/RESPONSE STRUCTURES                    *
 *****************************************************************************/

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
} CSI_V0_IDLE_REQUEST;

typedef struct {
    CSI_V0_REQUEST_HEADER       csi_request_header;
    RESPONSE_STATUS     message_status;
} CSI_V0_IDLE_RESPONSE;


/*
 *      Procedure Type Declarations:
 */

#endif /* _CSI_V0_STRUCTS_ */

