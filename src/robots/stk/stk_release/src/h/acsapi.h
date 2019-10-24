
#ifndef _ACSAPI_H_
#define _ACSAPI_H_ 1

/* static char    SccsId[] = "@(#) %full_name:     h/incl/acsapi/2.1.1 %"; */
/*
 *                    (C) Copyright (1992-2002) 
 *                  Storage Technology Corporation 
 *                        All Rights Reserved 
 *
 * Functional Description:
 *
 *   	This header file contains all necessary interface
 *      information for StorageTek and/or third-party use of
 *      the ACSLS API.
 *
 * Considerations:
 *
 *      NONE
 *
 * Modified by:
 *
 *   Tom Rethard         08/01/93       Code ported and restructured 
 *                                      from UNIX (ACSLS V3)
 *   Ken Stickney        08/29/93       Support added for BOS/X and 
 *                                      ACSLS Mixed Media
 *   Howard Grapek       09/08/93       1) Fixed ID KEYWORDS for SCCS
 *                                      2) fixed counts in QUERY to 
 *                                         conform to spec.
 *                                      3) Indented to conform to coding 
 *                                         guidelines.
 *   Ken Stickney        09/15/93       Restructuring 
 *   Ken Stickney        01/18/94       More documentation for
 *                                      ACS_LO_DRV_STATUS and 
 *                                      ACS_LO_VOL_STATUS
 *   Mitch Black         06/20/94       Ken's changes to 
 *                                      ACS_AUDIT_ACS_RESPONSE struct
 *   Scott Siao          09/10/01       Added ACS_REGISTER,
 *                                      ACS_UNREGISTER,
 *                                      ACS_CHECK_REGISTRATION
 *   Scott Siao          11/12/01       Added ACS_DISPLAY
 *   Scott Siao          02/06/02       Added ACS_QU_DRV_GROUP_RESPONSE,
 *                                      ACS_QU_SUBPOOL_NAME_RESPONSE,
 *                                      ACS_MOUNT_PINFO_RESPONSE.
 *
 */

/*
 *      Header Files:
 */
#ifndef _ACSSYS_H_
/*#error "acssys.h" must be #included when using "acsapi.h" */
#endif

#include "inclds.h" /* all the common shared header files from ACSLS */

#include "apidef.h"           /* acsapi specific definitions */
#include "apipro.h"           /* acsapi functional prototypes */
 

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/* ACSAPI lock/clear_lock drive status structure */
typedef struct {
    DRIVEID         drive_id;
    STATUS          status;           
    DRIVEID         dlocked_drive_id; /* valid for acs_lock_drive only */
} ACS_LO_DRV_STATUS;                  

/* ACSAPI lock/clear_lock volume status structure */
typedef struct {
    VOLID           vol_id;
    STATUS          status;
    VOLID           dlocked_vol_id; /* valid for acs_lock_volume only */
} ACS_LO_VOL_STATUS;

/* ACSAPI Audit Intermediate Response */
typedef struct {
    STATUS          audit_int_status;
    CAPID           cap_id;
    unsigned short  count;
    VOLID           vol_id[MAX_ID];
    STATUS          vol_status[MAX_ID];
} ACS_AUDIT_INT_RESPONSE;


/* Audit ACS Final Response */
typedef struct {
    STATUS          audit_acs_status;
    unsigned short  count;
    ACS             acs[MAX_ID];
    STATUS          acs_status[MAX_ID];
} ACS_AUDIT_ACS_RESPONSE;

/* Audit LSM Response */
typedef struct {
    STATUS          audit_lsm_status;
    unsigned short  count;
    LSMID           lsm_id[MAX_ID];
    STATUS          lsm_status[MAX_ID];
} ACS_AUDIT_LSM_RESPONSE;

/* Audit Panel Response */
typedef struct {
    STATUS          audit_pnl_status;
    unsigned short  count;
    PANELID         panel_id[MAX_ID];
    STATUS          panel_status[MAX_ID];
} ACS_AUDIT_PNL_RESPONSE;

/* Audit SubPanel response */
typedef struct {
    STATUS          audit_sub_status;
    unsigned short  count;
    SUBPANELID      subpanel_id[MAX_ID];
    STATUS          subpanel_status[MAX_ID];
} ACS_AUDIT_SUB_RESPONSE;

/* Audit Server Final Response */
typedef struct {
    STATUS          audit_srv_status;
} ACS_AUDIT_SRV_RESPONSE;

/* Cancel response */
typedef struct {
    STATUS          cancel_status;
    REQ_ID          req_id;
} ACS_CANCEL_RESPONSE;

/* Idle response */
typedef struct {
    STATUS          idle_status;
} ACS_IDLE_RESPONSE;

/* Start response */
typedef struct {
    STATUS          start_status;
} ACS_START_RESPONSE;

/* Enter response */
typedef struct {
    STATUS          enter_status;
    CAPID           cap_id;
    unsigned short  count;
    VOLID           vol_id[MAX_ID];
    STATUS          vol_status[MAX_ID];
} ACS_ENTER_RESPONSE;

/* Eject response */
typedef struct {
    STATUS          eject_status;
    CAPID           cap_id;
    unsigned short  count;
    CAPID           cap_used[MAX_ID];
    VOLID           vol_id[MAX_ID];
    STATUS          vol_status[MAX_ID];
} ACS_EJECT_RESPONSE;

/* clear Lock Drive response */
typedef struct {
    STATUS          clear_lock_drv_status;
    unsigned short  count;
    ACS_LO_DRV_STATUS   drv_status[MAX_ID];
} ACS_CLEAR_LOCK_DRV_RESPONSE;

/* Clear Lock Volume Response */
typedef struct {
    STATUS          clear_lock_vol_status;
    unsigned short  count;
    ACS_LO_VOL_STATUS   vol_status[MAX_ID];
} ACS_CLEAR_LOCK_VOL_RESPONSE;

/* Lock Drive Response */
typedef struct {
    STATUS          lock_drv_status;
    LOCKID          lock_id;
    unsigned short  count;
    ACS_LO_DRV_STATUS   drv_status[MAX_ID];
} ACS_LOCK_DRV_RESPONSE;

/* Lock Volume Response */
typedef struct {
    STATUS          lock_vol_status;
    LOCKID          lock_id;
    unsigned short  count;
    ACS_LO_VOL_STATUS   vol_status[MAX_ID];
} ACS_LOCK_VOL_RESPONSE;

/* unlock Drive Response */
typedef struct {
    STATUS          unlock_drv_status;
    unsigned short  count;
    ACS_LO_DRV_STATUS   drv_status[MAX_ID];
} ACS_UNLOCK_DRV_RESPONSE;

/* Unlock Volume Response */
typedef struct {
    STATUS          unlock_vol_status;
    unsigned short  count;
    ACS_LO_VOL_STATUS   vol_status[MAX_ID];
} ACS_UNLOCK_VOL_RESPONSE;

/* Dismount Resp */
typedef struct {
    STATUS          dismount_status;
    VOLID           vol_id;
    DRIVEID         drive_id;
} ACS_DISMOUNT_RESPONSE;

/* Mount Response */
typedef struct {
    STATUS          mount_status;
    VOLID           vol_id;
    DRIVEID         drive_id;
} ACS_MOUNT_RESPONSE;

/* Mount Scratch Response */
typedef struct {
    STATUS          mount_scratch_status;
    VOLID           vol_id;
    POOL            pool;
    DRIVEID         drive_id;
} ACS_MOUNT_SCRATCH_RESPONSE;

typedef struct {
    STATUS          query_acs_status;
    unsigned short  count;
    QU_ACS_STATUS   acs_status[MAX_ID];
} ACS_QUERY_ACS_RESPONSE;

typedef struct {
    STATUS          query_cap_status;
    unsigned short  count;
    QU_CAP_STATUS   cap_status[MAX_ID];
} ACS_QUERY_CAP_RESPONSE;

typedef struct {
    STATUS          query_cln_status;
    unsigned short  count;
    QU_CLN_STATUS   cln_status[MAX_ID];
} ACS_QUERY_CLN_RESPONSE;

typedef struct {
    STATUS          query_drv_status;
    unsigned short  count;
    QU_DRV_STATUS   drv_status[MAX_ID];
} ACS_QUERY_DRV_RESPONSE;

typedef struct {
    STATUS          query_drv_group_status;
    GROUPID         group_id;
    GROUP_TYPE      group_type;
    unsigned short  count;
    QU_VIRT_DRV_MAP virt_drv_map[MAX_VTD_MAP];
} ACS_QU_DRV_GROUP_RESPONSE;

typedef struct {
    STATUS          query_lock_drv_status;
    unsigned short  count;
    QL_DRV_STATUS   drv_status[MAX_ID];
} ACS_QUERY_LOCK_DRV_RESPONSE;

typedef struct {
    STATUS          query_lock_vol_status;
    unsigned short  count;
    QL_VOL_STATUS   vol_status[MAX_ID];
} ACS_QUERY_LOCK_VOL_RESPONSE;

typedef struct {
    STATUS          query_lsm_status;
    unsigned short  count;
    QU_LSM_STATUS   lsm_status[MAX_ID];
} ACS_QUERY_LSM_RESPONSE;

typedef struct {
    STATUS          query_mmi_status;
    QU_MMI_RESPONSE mixed_media_info_status;
} ACS_QUERY_MMI_RESPONSE;

typedef struct {
    STATUS          query_mnt_status;
    unsigned short  count;
    QU_MNT_STATUS   mnt_status[MAX_ID];
} ACS_QUERY_MNT_RESPONSE;

typedef struct {
    STATUS          query_msc_status;
    unsigned short  count;
    QU_MSC_STATUS   msc_status[MAX_ID];
} ACS_QUERY_MSC_RESPONSE;

typedef struct {
    STATUS          query_pol_status;
    unsigned short  count;
    QU_POL_STATUS   pool_status[MAX_ID];
} ACS_QUERY_POL_RESPONSE;

typedef struct {
    STATUS          query_prt_status;
    unsigned short  count;
    QU_PRT_STATUS   prt_status[MAX_ID];
} ACS_QUERY_PRT_RESPONSE;

typedef struct {
    STATUS          query_req_status;
    unsigned short  count;
    QU_REQ_STATUS   req_status[MAX_ID];
} ACS_QUERY_REQ_RESPONSE;

typedef struct {
    STATUS          query_scr_status;
    unsigned short  count;
    QU_SCR_STATUS   scr_status[MAX_ID];
} ACS_QUERY_SCR_RESPONSE;

typedef struct {
    STATUS          query_srv_status;
    unsigned short  count;
    QU_SRV_STATUS   srv_status[MAX_ID];
} ACS_QUERY_SRV_RESPONSE;

typedef struct {
    STATUS          query_subpool_name_status;
    unsigned short  count;
    QU_SUBPOOL_NAME_STATUS subpool_name_status[MAX_ID];
} ACS_QU_SUBPOOL_NAME_RESPONSE;

typedef struct {
    STATUS          query_vol_status;
    unsigned short  count;
    QU_VOL_STATUS   vol_status[MAX_ID];
} ACS_QUERY_VOL_RESPONSE;

typedef struct {
    STATUS          set_cap_status;
    CAP_PRIORITY    cap_priority;
    CAP_MODE        cap_mode;
    unsigned short  count;
    CAPID           cap_id[MAX_ID];
    STATUS          cap_status[MAX_ID];
} ACS_SET_CAP_RESPONSE;

typedef struct {
    STATUS          set_clean_status;
    unsigned short  max_use;
    unsigned short  count;
    VOLID           vol_id[MAX_ID];
    STATUS          vol_status[MAX_ID];
} ACS_SET_CLEAN_RESPONSE;

typedef struct {
    STATUS          set_scratch_status;
    POOL            pool;
    unsigned short  count;
    VOLID           vol_id[MAX_ID];
    STATUS          vol_status[MAX_ID];
} ACS_SET_SCRATCH_RESPONSE;

typedef struct {
    STATUS          define_pool_status;
    unsigned long   lwm;
    unsigned long   hwm;
    unsigned long   attributes;
    unsigned short  count;
    POOL            pool[MAX_ID];
    STATUS          pool_status[MAX_ID];
} ACS_DEFINE_POOL_RESPONSE;

typedef struct {
    STATUS          delete_pool_status;
    unsigned short  count;
    POOL            pool[MAX_ID];
    STATUS          pool_status[MAX_ID];
} ACS_DELETE_POOL_RESPONSE;

typedef struct {
    STATUS          vary_acs_status;
    STATE           acs_state;
    unsigned short  count;
    ACS             acs[MAX_ID];
    STATUS          acs_status[MAX_ID];
} ACS_VARY_ACS_RESPONSE;

typedef struct {
    STATUS          vary_cap_status;
    STATE           cap_state;
    unsigned short  count;
    CAPID           cap_id[MAX_ID];
    STATUS          cap_status[MAX_ID];
} ACS_VARY_CAP_RESPONSE;

typedef struct {
    STATUS          vary_drv_status;
    STATE           drive_state;
    unsigned short  count;
    DRIVEID         drive_id[MAX_ID];
    STATUS          drive_status[MAX_ID];
} ACS_VARY_DRV_RESPONSE;

typedef struct {
    STATUS          vary_lsm_status;
    STATE           lsm_state;
    unsigned short  count;
    LSMID           lsm_id[MAX_ID];
    STATUS          lsm_status[MAX_ID];
} ACS_VARY_LSM_RESPONSE;

typedef struct {
    STATUS          vary_prt_status;
    STATE           port_state;
    unsigned short  count;
    PORTID          port_id[MAX_ID];
    STATUS          port_status[MAX_ID];
} ACS_VARY_PRT_RESPONSE;

typedef struct {
    STATUS           register_status;
    EVENT_REPLY_TYPE event_reply_type;
    EVENT_SEQUENCE   event_sequence;
    EVENT            event;
} ACS_REGISTER_RESPONSE;

typedef struct {
    STATUS                   unregister_status;
    EVENT_REGISTER_STATUS    event_register_status;
} ACS_UNREGISTER_RESPONSE;

typedef struct {
    STATUS                   check_registration_status;
    EVENT_REGISTER_STATUS    event_register_status;
} ACS_CHECK_REGISTRATION_RESPONSE;

typedef struct {
    STATUS                   display_status;
    TYPE                     display_type;
    DISPLAY_XML_DATA         display_xml_data;
} ACS_DISPLAY_RESPONSE;

typedef struct {
    STATUS          mount_pinfo_status;
    POOLID          pool_id;
    DRIVEID         drive_id;
    VOLID           vol_id;
} ACS_MOUNT_PINFO_RESPONSE;

#endif	/* ifndef _ACSAPI_H_ */
