/* SccsId @(#)lm_structs_api.h	2.2 10/21/01  */
#ifndef _LM_STRUCTS_API_
#define _LM_STRUCTS_API_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Definitions of VERSION4 specific ACSLM data structures
 *
 * Considerations:
 *
 *      The structures defined here have corresponding definitions for the
 *      CSI/SSI in csi_structs.h.  any modifications to this file SHOULD be
 *      reflected in csi_structs.h as well.
 *
 * Modified by:
 *
 *      K. Stickney             27-Oct-1993     Original.
 *      S. Siao                 09-Oct-2001     Added register, unregister
 *                                              and check_register.  Also
 *                                              synchronized file with 6.0 lm_structs.h
 *      S. Siao                 12-Nov-2001     Added display.
 *      S. Siao                 04-Feb-2002     Added QU_SPN_CRITERIA and
 *                                                    QU_SPN_CRITERIA and
 *                                                    QU_DRG_RESPONSE and
 *                                                    QU_SPN_RESPONSE and
 *                                                    QU_SPN_REQUEST and
 *                                                    QU_SPN_REQUEST and
 *                                                    MOUNT_PINFO_REQUEST and
 *                                                    MOUNT_PINFO_RESPONSE.
 *      Wipro (Subhash)         04-Jun-2004     Added event_drive_status to
 *                                              REGISTER_RESPONSE.
 */


/*
 *      Defines, Typedefs and Structure Definitions:
 */
typedef struct {                        /* fixed portion of request_packet */
    IPC_HEADER      ipc_header;
    MESSAGE_HEADER  message_header;
} REQUEST_HEADER;

typedef struct {                        /* intermediate acknowledgement */
    REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    MESSAGE_ID      message_id;
} ACKNOWLEDGE_RESPONSE;

typedef struct {                        /* audit_request */
    REQUEST_HEADER      request_header;
    CAPID               cap_id;         /*   CAP for ejecting cartridges      */
    TYPE                type;           /*   type of identifiers              */
    unsigned short      count;          /*   number of identifiers            */
    union {                             /*   list of homogeneous IDs to audit */
        ACS             acs_id[MAX_ID];
        LSMID           lsm_id[MAX_ID];
        PANELID         panel_id[MAX_ID];
        SUBPANELID      subpanel_id[MAX_ID];
    } identifier;
} AUDIT_REQUEST;

typedef struct {                        /* audit_response */
    REQUEST_HEADER      request_header;
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
} AUDIT_RESPONSE;

typedef struct {                        /* eject_enter intermediate response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    CAPID               cap_id;         /*   CAP for ejecting cartridges     */
    unsigned short      count;          /*   no. of volumes ejected/entered  */
    VOLUME_STATUS       volume_status[MAX_ID];
} EJECT_ENTER;

typedef EJECT_ENTER     EJECT_RESPONSE;
typedef EJECT_ENTER     ENTER_RESPONSE;

typedef struct {                        /* eject request */
    REQUEST_HEADER      request_header;         
    CAPID               cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* External tape cartridge label */
} EJECT_REQUEST;

typedef struct {                           /* eject request */
    REQUEST_HEADER      request_header;         
    CAPID               cap_id;            /* CAP used for ejection */
    unsigned short      count;             /* Number of cartridges */
    VOLRANGE            vol_range[MAX_ID]; /* External tape cartridge label */
} EXT_EJECT_REQUEST;

typedef struct {                        /* eject request */
    REQUEST_HEADER      request_header;         
    CAPID               cap_id;         /* CAP used for entry */
} ENTER_REQUEST;

typedef struct {                        /* mount request */
    REQUEST_HEADER      request_header;
    VOLID               vol_id;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} MOUNT_REQUEST;

typedef struct  {                       /* mount response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} MOUNT_RESPONSE;

typedef struct {                        /* move request */
    REQUEST_HEADER      request_header;
    VOLID               vol_id[MAX_ID];
    unsigned short      count;
    LSMID               lsm;

} MOVE_REQUEST;

typedef struct  {                       /* move response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    unsigned short      count;
    VOLUME_STATUS       volume_status[MAX_ID];
} MOVE_RESPONSE;

typedef struct {                        /* mount scratch request */
    REQUEST_HEADER      request_header;
    POOLID              pool_id;
    MEDIA_TYPE		media_type;
    unsigned short      count;
    DRIVEID             drive_id[MAX_ID];
} MOUNT_SCRATCH_REQUEST;

typedef struct  {                       /* mount scratch response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    POOLID              pool_id;
    DRIVEID             drive_id;
    VOLID               vol_id;
} MOUNT_SCRATCH_RESPONSE;

typedef struct {                        /* dismount request */
    REQUEST_HEADER      request_header;
    VOLID               vol_id;
    DRIVEID             drive_id;
} DISMOUNT_REQUEST;

typedef struct  {                       /* dismount response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    VOLID               vol_id;
    DRIVEID             drive_id;
} DISMOUNT_RESPONSE;

typedef struct {                        /* lock request */
    REQUEST_HEADER      request_header;
    TYPE                type;
    unsigned short      count;
    union {
        VOLID           vol_id[MAX_ID];
        DRIVEID         drive_id[MAX_ID];
    } identifier;
} LOCK_REQUEST;

typedef LOCK_REQUEST    CLEAR_LOCK_REQUEST;
typedef LOCK_REQUEST    QUERY_LOCK_REQUEST;
typedef LOCK_REQUEST    UNLOCK_REQUEST;

typedef struct {                        /* lock response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        LO_VOL_STATUS   volume_status[MAX_ID];
        LO_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} LOCK_RESPONSE;

typedef LOCK_RESPONSE   CLEAR_LOCK_RESPONSE;
typedef LOCK_RESPONSE   UNLOCK_RESPONSE;

typedef struct {                        /* query_request */
    REQUEST_HEADER      request_header;
    TYPE                type;           /* type of query */
    union {                             /* list of homogeneous IDs to query */
        QU_ACS_CRITERIA		acs_criteria;
        QU_LSM_CRITERIA		lsm_criteria;
        QU_CAP_CRITERIA		cap_criteria;
        QU_DRV_CRITERIA		drive_criteria;
        QU_VOL_CRITERIA		vol_criteria;
        QU_REQ_CRITERIA		request_criteria;
        QU_PRT_CRITERIA		port_criteria;
        QU_POL_CRITERIA		pool_criteria;
        QU_MSC_CRITERIA 	mount_scratch_criteria;
	QU_LMU_CRITERIA         lmu_criteria;
	QU_DRG_CRITERIA         drive_group_criteria;
	QU_SPN_CRITERIA         subpl_name_criteria;
	QU_MSC_PINFO_CRITERIA   mount_scratch_pinfo_criteria;
    } select_criteria;
} QUERY_REQUEST;

typedef struct {                        /* query_response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;           /* type of query */
    union {                             /* list of IDs queried w/status */
        QU_SRV_RESPONSE		server_response;
        QU_ACS_RESPONSE		acs_response;
        QU_LSM_RESPONSE		lsm_response;
        QU_CAP_RESPONSE		cap_response;
        QU_CLN_RESPONSE		clean_volume_response;
        QU_DRV_RESPONSE		drive_response;
        QU_MNT_RESPONSE		mount_response;
        QU_VOL_RESPONSE		volume_response;
        QU_PRT_RESPONSE		port_response;
        QU_REQ_RESPONSE		request_response;
        QU_SCR_RESPONSE		scratch_response;
        QU_POL_RESPONSE		pool_response;
        QU_MSC_RESPONSE		mount_scratch_response;
        QU_MMI_RESPONSE		mm_info_response;
	QU_LMU_RESPONSE         lmu_response;
	QU_DRG_RESPONSE         drive_group_response;
	QU_SPN_RESPONSE         subpl_name_response;
    } status_response;
} QUERY_RESPONSE;

typedef struct {                        /* query lock response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {
        QL_VOL_STATUS   volume_status[MAX_ID];
        QL_DRV_STATUS   drive_status[MAX_ID];
    } identifier_status;
} QUERY_LOCK_RESPONSE;

typedef struct {                        /* vary request */
    REQUEST_HEADER      request_header;
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
} VARY_REQUEST;

typedef struct {                        /* vary response */
    REQUEST_HEADER      request_header;
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
} VARY_RESPONSE;

typedef struct {                                /* venter request */
    REQUEST_HEADER      request_header;         
    CAPID               cap_id;         /* CAP used for ejection */
    unsigned short      count;          /* Number of cartridges */
    VOLID               vol_id[MAX_ID]; /* Virtual tape cartridge label */
} VENTER_REQUEST;

typedef struct {                                /* define pool request */
    REQUEST_HEADER  request_header;
    unsigned long   low_water_mark;
    unsigned long   high_water_mark;
    unsigned long   pool_attributes;
    unsigned short  count;
    POOLID          pool_id[MAX_ID];
} DEFINE_POOL_REQUEST;

typedef struct {                                /* define pool response */
    REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    unsigned long   low_water_mark;
    unsigned long   high_water_mark;
    unsigned long   pool_attributes;
    unsigned short  count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} DEFINE_POOL_RESPONSE;

typedef struct {                                /* delete pool request */
    REQUEST_HEADER  request_header;
    unsigned short  count;
    POOLID          pool_id[MAX_ID];
} DELETE_POOL_REQUEST;

typedef struct {                                /* delete pool response */
    REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    unsigned short  count;
    struct {
        POOLID          pool_id;
        RESPONSE_STATUS status;
    } pool_status[MAX_ID];
} DELETE_POOL_RESPONSE;

typedef struct {                                /* set cap request */
    REQUEST_HEADER  request_header;
    CAP_PRIORITY    cap_priority;
    CAP_MODE        cap_mode;
    unsigned short  count;
    CAPID           cap_id[MAX_ID];
} SET_CAP_REQUEST;

typedef struct {                                /* set cap response */
    REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    CAP_PRIORITY    cap_priority;
    CAP_MODE        cap_mode;
    unsigned short  count;
    struct {
        CAPID           cap_id;
        RESPONSE_STATUS status;
    } set_cap_status[MAX_ID];
} SET_CAP_RESPONSE;

typedef struct {                                /* set cap request */
    REQUEST_HEADER  request_header;
    unsigned short  max_use;
    unsigned short  count;
    VOLRANGE        vol_range[MAX_ID];
} SET_CLEAN_REQUEST;

typedef struct {                                /* set cap response */
    REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    unsigned short  max_use;
    unsigned short  count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } volume_status[MAX_ID];
} SET_CLEAN_RESPONSE;

typedef struct {                                /* set owner request */
    REQUEST_HEADER  request_header;
    USERID          owner_id;
    TYPE            type;
    unsigned short  count;
    VOLRANGE        vol_range[MAX_ID];
} SET_OWNER_REQUEST;

typedef struct {                                /* set owner response */
    REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    USERID          owner_id;
    TYPE            type;
    unsigned short  count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } volume_status[MAX_ID];
} SET_OWNER_RESPONSE;

typedef struct {                                /* set scratch request */
    REQUEST_HEADER  request_header;
    POOLID          pool_id;
    unsigned short  count;
    VOLRANGE        vol_range[MAX_ID];
} SET_SCRATCH_REQUEST;

typedef struct {                                /* set scratch response */
    REQUEST_HEADER  request_header;
    RESPONSE_STATUS message_status;
    POOLID          pool_id;
    unsigned short  count;
    struct {
        VOLID           vol_id;
        RESPONSE_STATUS status;
    } scratch_status[MAX_ID];
} SET_SCRATCH_RESPONSE;

typedef struct {                                /* cancel request */
    REQUEST_HEADER      request_header;
    MESSAGE_ID          request;
} CANCEL_REQUEST;

typedef struct {                                /* cancel response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    MESSAGE_ID          request;
} CANCEL_RESPONSE;

typedef struct {                                /* start request */
    REQUEST_HEADER      request_header;
} START_REQUEST;

typedef struct {                                /* start response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
} START_RESPONSE;

typedef struct {                                /* idle request */
     REQUEST_HEADER     request_header;
} IDLE_REQUEST;

typedef struct {                                /* idle response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
} IDLE_RESPONSE;

typedef struct {                                /* initialization request */
    REQUEST_HEADER      request_header;
} INIT_REQUEST;

typedef struct {                        /* switch request */
    REQUEST_HEADER      request_header;
    STATE               state;
    TYPE                type;
    unsigned short      count;
    union {                             /* list of homogeneous IDs to vary */
        ACS             lmu_id[MAX_ID];
    } identifier;
} SWITCH_REQUEST;

typedef struct {                        /* switch response */
    REQUEST_HEADER      request_header;
    RESPONSE_STATUS     message_status;
    TYPE                type;
    unsigned short      count;
    union {                             /* list of IDs switched w/status */
        SW_LMU_STATUS   lmu_status[MAX_ID];
    } device_status;
} SWITCH_RESPONSE;

typedef struct {                        /* Cartridge Recovery request */
    REQUEST_HEADER      request_header;
    TYPE                request_type;       /* TYPE_VOLUME, TYPE_MISSING,   */
                                            /* TYPE_CELL, TYPE_DRIVE        */
	VOLID               vol_id;             /* volume identifier            */
	TYPE                location_type;      /* TYPE_<CELL|DRIVE|NONE>       */
	union {                                 /* location identifier:         */
	    CELLID          cell_id;            /*   if TYPE_CELL               */
	    DRIVEID         drive_id;           /*   if TYPE_DRIVE              */
	} location;                             /*   undefined if none of above */
	char                file_name[25];      /* caller_id (source file)      */
	char                routine_name[25];   /* caller_id (routine name)     */
    } RCVY_REQUEST;

    typedef struct  {                       /* Cartridge Recovery Response */
	REQUEST_HEADER      request_header;
	RESPONSE_STATUS     message_status;
	VOLUME_STATUS       volume_status;
    } RCVY_RESPONSE;

    typedef struct {                            /* register request */
	REQUEST_HEADER      request_header;         
	REGISTRATION_ID     registration_id;    /* registration id */
	unsigned short      count;             /*Number of events registering for */
	EVENT_CLASS_TYPE    eventClass[MAX_ID];/*array of event classes */
    } REGISTER_REQUEST;

    typedef struct {                            /* register response */
	REQUEST_HEADER      request_header;         
	RESPONSE_STATUS     message_status;
	EVENT_REPLY_TYPE    event_reply_type;
	EVENT_SEQUENCE      event_sequence;
	union {
	    EVENT_RESOURCE_STATUS    event_resource_status;
	    EVENT_REGISTER_STATUS    event_register_status;
	    EVENT_VOLUME_STATUS      event_volume_status;
        EVENT_DRIVE_STATUS       event_drive_status;
	} event;
    } REGISTER_RESPONSE;

    typedef struct {                            /* unregister request */
	REQUEST_HEADER      request_header;         
	REGISTRATION_ID     registration_id;    /* registration id */
	unsigned short      count;              /* Number of events unregistering for */
	EVENT_CLASS_TYPE    eventClass[MAX_ID]; /* array of event classes */
    } UNREGISTER_REQUEST;

    typedef struct {                            /* unregister response */
	REQUEST_HEADER         request_header;         
	RESPONSE_STATUS        message_status;
	EVENT_REGISTER_STATUS  event_register_status;
    } UNREGISTER_RESPONSE;

    typedef struct {                            /* check register request */
	REQUEST_HEADER      request_header;         
	REGISTRATION_ID     registration_id;    /* registration id */
    } CHECK_REGISTRATION_REQUEST;

    typedef struct {                            /* check register response */
	REQUEST_HEADER         request_header;         
	RESPONSE_STATUS        message_status;
	EVENT_REGISTER_STATUS  event_register_status;
    } CHECK_REGISTRATION_RESPONSE;

    typedef struct {                            /* display request */
	REQUEST_HEADER      request_header;         
	TYPE                display_type;
	DISPLAY_XML_DATA    display_xml_data;           /* xml data stream */
    } DISPLAY_REQUEST;

    typedef struct {                            /* display response */
	REQUEST_HEADER         request_header;         
	RESPONSE_STATUS        message_status;
	TYPE                   display_type;
	DISPLAY_XML_DATA       display_xml_data;           /* xml data stream */
    } DISPLAY_RESPONSE;

    typedef struct {                            /* virtual mount request */
	REQUEST_HEADER  request_header;         
	VOLID           vol_id;             /* volume identifier    */
	POOLID          pool_id;
	MGMT_CLAS	mgmt_clas;
	MEDIA_TYPE	media_type;
	JOB_NAME	job_name;
	DATASET_NAME	dataset_name;
	STEP_NAME	step_name;
	DRIVEID         drive_id;
    } MOUNT_PINFO_REQUEST;

    typedef struct {                            /* virtual mount response */
	REQUEST_HEADER         request_header;         
	RESPONSE_STATUS        message_status;
	POOLID          pool_id;
	DRIVEID         drive_id;
	VOLID               vol_id;             /* volume identifier    */
    } MOUNT_PINFO_RESPONSE;
#endif /* _LM_STRUCTS_ */

