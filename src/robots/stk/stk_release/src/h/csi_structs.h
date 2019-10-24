/* SccsId @(#)csi_structs.h	2.2 10/21/01  */
#ifndef _CSI_STRUCTS_
#define _CSI_STRUCTS_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Definitions of CSI data structures for VERSION 2.
 *
 *      The structures defined here have corresponding definitions for the
 *      ACSLM in lm_structs.h.  any modifications to this file MUST be
 *      reflected in lm_structs.h as well.
 *
 * Modified by:
 *
 *      D. F. Reed       01/29/89       Original.
 *      J. A. Wishner    01/30/89       Added definitions for CSI_HEADER.
 *      J. A. Wishner    05/01/89.      TIME STAMP-POST CUSTOMER INITIAL RELEASE
 *                                      Any changes after this date must appear
 *                                      below this line.
 *      D. F. Reed       05/08/89       Change mount request drive_id dimension
 *                                      to MAX_ID.
 *      R. P. Cushman    04/20/90.      Added CSI_ACSPD_REQUEST 
 *                                      CSI_ACSPD_RESPONSE and 
 *                                      #include pdaemon.h
 *      J. W. Montgomery 15-Jun-90.     Version 2.
 *      J. W. Montgomery 24-Aug-90.     Fixed EXT_EJECT_REQUEST.
 *      J. A. Wishner    26-Sep-1990.   Added CSI_ACSPD_REQUEST_HEADER.
 *      H. I. Grapek     31-Aug-1991    Added for release 3 (version 2)::
 *                                      CAPID suppport to vary req/res,
 *                                      CAP_MODE to      CSI_SET_CAP_REQUEST and
 *                                                       CSI_SET_CAP_RESPONSE,
 *                                      QU_CAP_STATUS to CSI_QUERY_REQUEST   and
 *                                                       CSI_QUERY_RESPONSE,
 *                                      VA_CAP_STATUS to CSI_VARY_REQUEST  a  nd
 *                                                   and CSI_VARY_RESPONSE.
 *      J. A. Wishner    03-Oct-1991.   Completed release 3 (verion 2) mods.
 *	Emanuel Alongi   16-Aug-1993.   Modifications to support release 5,
 *					version 4 query request and response
 *					packets.
 *      S. L. Siao       22-Oct-2001    Added CSI_REGISTER_REQUEST
 *                                            CSI_UNREGISTER_REQUEST,
 *                                            CSI_CHECK_REGISTRATION_REQUEST,
 *                                            CSI_REGISTER_RESPONSE,
 *                                            CSI_UNREGISTER_RESPONSE,
 *                                            CSI_CHECK_REGISTRATION_RESPONSE
 *      S. L. Siao       12-Nov-2001    Added CSI_DISPLAY_REQUEST, CSI_DISPLAY_RESPONSE
 *      S. L. Siao       06-Feb-2002    Added CSI_MOUNT_PINFO_REQUEST,
 *                                            CSI_MOUNT_PINFO_RESPONSE,
 *                                            QU_DRG_REQUEST,
 *                                            QU_DRG__RESPONSE,
 *                                            QU_SPN_REQUEST,
 *                                            QU_SPN_RESPONSE
 *      S. L. Siao       29-Apr-2002    In CSI_REGISTER_REQUEST and CSI_UNREGISTER
 *                                      request changed MAX_ID limit for eventClass
 *                                      from MAX_ID to MAX_REGISTER_STATUS.
 *      S. L. Siao       20-May-2002    Changed MAX_REGISTER_STATUS to
 *                                      MAX_EVENT_CLASS_TYPE.
 *      Wipro (Subhash)  04-Jun-2004    Added event_drive_status to
 *                                      CSI_REGISTER_RESPONSE.
 *	Mitch Black	 01-Dec-2004	Remove pdaemon structures.
 */

/*      Header Files: */

#ifndef _CSI_V0_STRUCTS_
#include "csi_v0_structs.h"
#endif

/*      Defines, Typedefs and Structure Definitions: */

typedef struct {                        /* fixed portion of request_packet */
    CSI_HEADER          csi_header;
    MESSAGE_HEADER      message_header;
} CSI_REQUEST_HEADER;

typedef struct {                        /* intermediate acknowledgement */
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          message_id;
} CSI_ACKNOWLEDGE_RESPONSE;

/*****************************************************************************
 *                       AUDIT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* audit_request                      */
    CSI_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;         /*   CAP for ejecting cartridges      */
    TYPE                type;           /*   type of identifiers              */
    unsigned short      count;          /*   number of identifiers            */
    union {                             /*   list of homogeneous IDs to audit */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        PANELID         panel_id[MAX_ID];
        SUBPANELID      subpanel_id[MAX_ID];
    } identifier;
} CSI_AUDIT_REQUEST;

typedef struct {                        /* audit_response                  */
    CSI_REQUEST_HEADER  csi_request_header;
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
} CSI_AUDIT_RESPONSE;

typedef struct {                        /* eject_enter intermediate response */
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    CAPID               cap_id;         /*   CAP for ejecting cartridges     */
    unsigned short      count;          /*   no. of volumes ejected/entered  */
    VOLUME_STATUS       volume_status[MAX_ID];
} CSI_EJECT_ENTER;

/*****************************************************************************
 *                       EJECT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* eject request */
    CSI_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* External tape cartridge label */
} CSI_EJECT_REQUEST;

typedef struct {                           /* xeject request */
    CSI_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;            /* CAP used for ejection */
    unsigned short      count;             /* Number of cartridges */
    VOLRANGE            vol_range[MAX_ID]; /* External tape cartridge label */
} CSI_EXT_EJECT_REQUEST;

typedef CSI_EJECT_ENTER CSI_EJECT_RESPONSE;

/*****************************************************************************
 *                       ENTER REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* eject request */
    CSI_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;         /* CAP used for entry */
} CSI_ENTER_REQUEST;

typedef CSI_EJECT_ENTER CSI_ENTER_RESPONSE;

/*****************************************************************************
 *                       MOUNT REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    VOLID               vol_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} CSI_MOUNT_REQUEST;

typedef struct  {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_MOUNT_RESPONSE;
 
/*****************************************************************************
 *                       MOUNT SCRATCH REQUEST/RESPONSE STRUCTURES           *
 *****************************************************************************/
 
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    POOLID              pool_id;
    MEDIA_TYPE		media_type;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} CSI_MOUNT_SCRATCH_REQUEST;
 
typedef struct  {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    DRIVEID             drive_id;
    VOLID               vol_id;
} CSI_MOUNT_SCRATCH_RESPONSE;

/*****************************************************************************
 *                      MOUNT PINFO REQUEST/RESPONSE STRUCTURES            *
 *****************************************************************************/
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    VOLID               vol_id;
    POOLID              pool_id;
    MGMT_CLAS           mgmt_clas;
    MEDIA_TYPE          media_type;
    JOB_NAME            job_name;
    DATASET_NAME        dataset_name;
    STEP_NAME           step_name;
    DRIVEID             drive_id;
} CSI_MOUNT_PINFO_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    DRIVEID             drive_id;
    VOLID               vol_id;
} CSI_MOUNT_PINFO_RESPONSE;

/*****************************************************************************
 *                       DISMOUNT REQUEST/RESPONSE STRUCTURES                * 
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_DISMOUNT_REQUEST;

typedef struct  {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} CSI_DISMOUNT_RESPONSE;

/*****************************************************************************
 *                       LOCK REQUEST/RESPONSE STRUCTURES                *
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    TYPE                type;
    unsigned short      count;
    union {
        VOLID           vol_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
    } identifier;
} CSI_LOCK_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        LO_VOL_STATUS   volume_status[MAX_ID];
        LO_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} CSI_LOCK_RESPONSE;

/*****************************************************************************
 *                       CLEAR-LOCK REQUEST/RESPONSE STRUCTURES              *
 *****************************************************************************/

typedef CSI_LOCK_REQUEST    CSI_CLEAR_LOCK_REQUEST;

typedef CSI_LOCK_RESPONSE   CSI_CLEAR_LOCK_RESPONSE;



/*****************************************************************************
 *                       QUERY REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {                        /* query_request                      */
    CSI_REQUEST_HEADER  csi_request_header;
    TYPE                type;           /*   type of query                    */
    union {                             /*   list of homogeneous IDs to query */
        QU_ACS_CRITERIA		acs_criteria;
        QU_LSM_CRITERIA         lsm_criteria;
        QU_CAP_CRITERIA         cap_criteria;
        QU_DRV_CRITERIA         drive_criteria;
        QU_VOL_CRITERIA         vol_criteria;
        QU_REQ_CRITERIA         request_criteria;
        QU_PRT_CRITERIA         port_criteria;
        QU_POL_CRITERIA         pool_criteria;
	QU_MSC_CRITERIA	   	mount_scratch_criteria;
	QU_DRG_CRITERIA	   	drive_group_criteria;
	QU_SPN_CRITERIA	   	subpl_name_criteria;
	QU_MSC_PINFO_CRITERIA	mount_scratch_pinfo_criteria;
    } select_criteria;
} CSI_QUERY_REQUEST;

typedef struct {                        /* query_response                 */
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;           /*   type of query                */
    union {                             /*   list of IDs queried w/status */
        QU_SRV_RESPONSE   server_response;
        QU_ACS_RESPONSE   acs_response;
        QU_LSM_RESPONSE   lsm_response;
        QU_CAP_RESPONSE   cap_response;
        QU_CLN_RESPONSE   clean_volume_response;
        QU_DRV_RESPONSE   drive_response;
        QU_MNT_RESPONSE   mount_response;
        QU_VOL_RESPONSE   volume_response;
        QU_PRT_RESPONSE   port_response;
        QU_REQ_RESPONSE   request_response;
        QU_SCR_RESPONSE   scratch_response;
        QU_POL_RESPONSE   pool_response;
        QU_MSC_RESPONSE   mount_scratch_response;
        QU_MMI_RESPONSE   mm_info_response;
        QU_DRG_RESPONSE   drive_group_response;
        QU_SPN_RESPONSE   subpl_name_response;
    } status_response;
} CSI_QUERY_RESPONSE;

/*****************************************************************************
 *                       QUERY_LOCK REQUEST/RESPONSE STRUCTURES               *
 *****************************************************************************/

typedef CSI_LOCK_REQUEST    CSI_QUERY_LOCK_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        QL_VOL_STATUS   volume_status[MAX_ID];
        QL_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} CSI_QUERY_LOCK_RESPONSE;

/*****************************************************************************
 *                       UNLOCK REQUEST/RESPONSE STRUCTURES                *
 *****************************************************************************/

typedef CSI_LOCK_REQUEST    CSI_UNLOCK_REQUEST;

typedef CSI_LOCK_RESPONSE   CSI_UNLOCK_RESPONSE;


/*****************************************************************************
 *                       VARY REQUEST/RESPONSE STRUCTURES                    * 
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
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
} CSI_VARY_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
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
} CSI_VARY_RESPONSE;

/*****************************************************************************
 *                       VENTER REQUEST STRUCTURE                            *
 *****************************************************************************/
 
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    CAPID               cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* Virtual tape cartridge label */
} CSI_VENTER_REQUEST;
 
/*****************************************************************************
 *                  DEFINE_POOL REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/
 
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    unsigned long       low_water_mark;
    unsigned long       high_water_mark;
    unsigned long       pool_attributes;
    unsigned short      count;
    POOLID              pool_id[MAX_ID];
} CSI_DEFINE_POOL_REQUEST;
 
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    unsigned long       low_water_mark;
    unsigned long       high_water_mark;
    unsigned long       pool_attributes;
    unsigned short      count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} CSI_DEFINE_POOL_RESPONSE;
 
/*****************************************************************************
 *                  DELETE_POOL REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/
 
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    unsigned short      count;
    POOLID              pool_id[MAX_ID];
} CSI_DELETE_POOL_REQUEST;
 
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    unsigned short      count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} CSI_DELETE_POOL_RESPONSE;
 

/*****************************************************************************
 *                  SET_CAP REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    CAP_PRIORITY        cap_priority;
    CAP_MODE            cap_mode;
    unsigned short      count;
    CAPID               cap_id[MAX_ID];
} CSI_SET_CAP_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  request_header;
    RESPONSE_STATUS     message_status;
    CAP_PRIORITY        cap_priority;
    CAP_MODE            cap_mode;
    unsigned short      count;
    struct {
        CAPID           cap_id;
        RESPONSE_STATUS status;
    } set_cap_status[MAX_ID];
} CSI_SET_CAP_RESPONSE;

/*****************************************************************************
 *                  SET_CLEAN REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    unsigned short      max_use;
    unsigned short      count;
    VOLRANGE            vol_range[MAX_ID];
} CSI_SET_CLEAN_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    unsigned short      max_use;
    unsigned short      count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } volume_status[MAX_ID];
} CSI_SET_CLEAN_RESPONSE;

/*****************************************************************************
 *                  SET_SCRATCH REQUEST/RESPONSE STRUCTURES                  *
 *****************************************************************************/
 
typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    POOLID              pool_id;
    unsigned short      count;
    VOLRANGE            vol_range[MAX_ID];
} CSI_SET_SCRATCH_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    unsigned short      count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } scratch_status[MAX_ID];
} CSI_SET_SCRATCH_RESPONSE;

/*****************************************************************************
 *                       CANCEL REQUEST/RESPONSE STRUCTURES                  * 
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    MESSAGE_ID          request;
} CSI_CANCEL_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          request;
} CSI_CANCEL_RESPONSE;

/*****************************************************************************
 *                       START REQUEST/RESPONSE STRUCTURES                   * 
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
} CSI_START_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
} CSI_START_RESPONSE;

/*****************************************************************************
 *                       IDLE REQUEST/RESPONSE STRUCTURES                    *
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
} CSI_IDLE_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
} CSI_IDLE_RESPONSE;


/*****************************************************************************
 *                   REGISTER REQUEST/RESPONSE STRUCTURES                    *
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    REGISTRATION_ID     registration_id;
    unsigned short      count;
    EVENT_CLASS_TYPE    eventClass[MAX_EVENT_CLASS_TYPE];
} CSI_REGISTER_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    RESPONSE_STATUS     message_status;
    EVENT_REPLY_TYPE    event_reply_type;
    EVENT_SEQUENCE      event_sequence;
    union {
	EVENT_RESOURCE_STATUS    event_resource_status;
	EVENT_REGISTER_STATUS    event_register_status;
	EVENT_VOLUME_STATUS      event_volume_status;
    EVENT_DRIVE_STATUS       event_drive_status;
    } event;
} CSI_REGISTER_RESPONSE;


/*****************************************************************************
 *                 UNREGISTER REQUEST/RESPONSE STRUCTURES                    *
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    REGISTRATION_ID     registration_id;
    unsigned short      count;
    EVENT_CLASS_TYPE    eventClass[MAX_EVENT_CLASS_TYPE];
} CSI_UNREGISTER_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER      csi_request_header;
    RESPONSE_STATUS         message_status;
    EVENT_REGISTER_STATUS   event_register_status;
} CSI_UNREGISTER_RESPONSE;

/*****************************************************************************
 *          CHECK REGISTRATION REQUEST/RESPONSE STRUCTURES                   *
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    REGISTRATION_ID     registration_id;
} CSI_CHECK_REGISTRATION_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER      csi_request_header;
    RESPONSE_STATUS         message_status;
    EVENT_REGISTER_STATUS   event_register_status;
} CSI_CHECK_REGISTRATION_RESPONSE;

/*****************************************************************************
 *          DISPLAY REQUEST/RESPONSE STRUCTURES                              *
 *****************************************************************************/

typedef struct {
    CSI_REQUEST_HEADER  csi_request_header;
    TYPE                display_type;
    DISPLAY_XML_DATA    display_xml_data;
} CSI_DISPLAY_REQUEST;

typedef struct {
    CSI_REQUEST_HEADER      csi_request_header;
    RESPONSE_STATUS         message_status;
    TYPE                    display_type;
    DISPLAY_XML_DATA        display_xml_data;
} CSI_DISPLAY_RESPONSE;

/*
 *      Procedure Type Declarations:
 */

#endif /* _CSI_STRUCTS_ */

