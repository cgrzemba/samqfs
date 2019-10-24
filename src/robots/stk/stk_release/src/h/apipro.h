#ifndef _APIPRO_H_
#define _APIPRO_H_ 1

/* static char    SccsId[] = "@(#) %full_name:     1/incl/apipro.h/7 %"; */

/*
 * Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Functional Description:
 *
 *      This header file contains all the function prototype 
 *      definitions for the ACSLS API.
 *
 * Considerations:
 *
 *      NONE
 *
 * Modified by:
 *
 *   Ken Stickney         11/06/93       Original.
 *   Ken Stickney         01/20/94       Added prototypes for 
 *                                       acs_type() & 
 *                                       acs_get_packet_version().
 *   Ken Stickney         05/06/94       Added function prototypes that
 *                                       hide common lib conversion functions
 *                                       in the binary AIX toolkit.
 *   Mitch Black          06/20/94       Change to acs_audit_acs parameters
 *   Scott Siao           10/15/01       Added prototypes for
 *                                       acs_register, acs_unregister,
 *                                       and acs_check_registration.
 *   Scott Siao           11/12/01       Added prototypes for acs_display.
 *   Scott Siao           02/06/02       Added prototypes for 
 *                                         acs_query_subpool_name,
 *                                         acs_mount_pinfo,
 *                                         acs_query_mount_scratch_pinfo.
 *   Scott Siao           04/29/02       In acs_register and acs_unregister
 *                                       changed MAX_ID to MAX_REGISTER_STATUS
 *                                       in eventClass array.
 *   Scott Siao           05/20/02       Changed MAX_REGISTER_STATUS to
 *                                       MAX_EVENT_CLASS_TYPE.
 *   Mike Williams        01/06/2010     Added prototypes for acs_mount_pinfo
 *                                       and acs_query_mount_group.
 *
 */

/* Functional prototypes for the ACSAPI library */

STATUS acs_audit_acs(
    SEQ_NO  seqNumber,
    ACS             acs[MAX_ID],
    CAPID           capId,
    unsigned short  count);

STATUS acs_audit_lsm(
    SEQ_NO          seqNumber,
    LSMID           lsmId[MAX_ID],
    CAPID           capId,
    unsigned short  count);

STATUS acs_audit_panel(
    SEQ_NO          seqNumber,
    PANELID         panelId[MAX_ID],
    CAPID           capId,
    unsigned short  count);

STATUS acs_audit_subpanel(
    SEQ_NO          seqNumber,
    SUBPANELID      subpanelId[MAX_ID],
    CAPID           capId,
    unsigned short  count);

STATUS acs_audit_server
(
    SEQ_NO seqNumber,
    CAPID capId);

STATUS acs_cancel(
    SEQ_NO  seqNumber,
    REQ_ID  reqId);

STATUS acs_idle(
    SEQ_NO   seqNumber,
    BOOLEAN  force);

STATUS acs_start(
  SEQ_NO     seqNumber);

STATUS acs_set_access(
  char *user_id);


STATUS acs_enter(
  SEQ_NO     seqNumber,
  CAPID      capId,
  BOOLEAN    continuous);

STATUS acs_eject(
    SEQ_NO          seqNumber,
    LOCKID          lockId,
    CAPID           capId,
    unsigned short  count,
    VOLID           volumes[MAX_ID]);

STATUS acs_venter(
    SEQ_NO           seqNumber,
    CAPID            capId,
    unsigned short   count,
    VOLID            volId[MAX_ID]);

STATUS acs_xeject(
    SEQ_NO     seqNumber,
    LOCKID     lockId,
    CAPID      capId,
    VOLRANGE   volRange[MAX_ID],
    unsigned short count);

STATUS acs_clear_lock_drive(
    SEQ_NO          seqNumber,
    DRIVEID         driveId[MAX_ID],
    unsigned short  count);

STATUS acs_clear_lock_volume(
    SEQ_NO          seqNumber,
    VOLID           volId[MAX_ID],
    unsigned short  count);

STATUS acs_lock_drive(
    SEQ_NO          seqNumber,
    LOCKID          lockId,
    USERID          userId,
    DRIVEID         driveId[MAX_ID],
    BOOLEAN         wait,
    unsigned short  count);

STATUS acs_lock_volume (
    SEQ_NO          seqNumber,
    LOCKID          lockId,
    USERID          userId,
    VOLID           volId[MAX_ID],
    BOOLEAN         wait,
    unsigned short  count);

STATUS acs_unlock_drive(
    SEQ_NO          seqNumber,
    LOCKID          lockId,
    DRIVEID         driveId[MAX_ID],
    unsigned short  count);

STATUS acs_unlock_volume(
    SEQ_NO          seqNumber,
    LOCKID          lockId,
    VOLID           volId[MAX_ID],
    unsigned short  count);

STATUS acs_dismount(
    SEQ_NO     seqNumber,
    LOCKID     lockId,
    VOLID      volId,
    DRIVEID    driveId,
    BOOLEAN    force);

STATUS acs_mount(
    SEQ_NO     seqNumber,
    LOCKID     lockId,
    VOLID      volId,
    DRIVEID    driveId,
    BOOLEAN    readonly,
    BOOLEAN    bypass);

STATUS acs_mount_pinfo(
    SEQ_NO seqNumber,
    LOCKID lockId,
    VOLID volid,
    POOLID poolid,
    MGMT_CLAS mgmt_cl,
    MEDIA_TYPE media_type,
    BOOLEAN scratch,
    BOOLEAN readonly,
    BOOLEAN bypass,
    JOB_NAME jobname,
    DATASET_NAME dsnname,
    STEP_NAME stepname,
    DRIVEID driveid);

STATUS acs_mount_scratch(
    SEQ_NO   seqNumber,
    LOCKID   lockId,
    POOL     pool,
    DRIVEID  driveId,
    MEDIA_TYPE mtype);

STATUS acs_query_mount_scratch_pinfo(
    SEQ_NO          seqNumber,
    POOL            pool[MAX_ID],
    unsigned short  count,
    MEDIA_TYPE      media_type,
    MGMT_CLAS       mgmt_clas);

STATUS acs_query_acs(
    SEQ_NO          seqNumber,
    ACS             acs[MAX_ID],
    unsigned short  count);

STATUS acs_query_cap(
    SEQ_NO          seqNumber,
    CAPID           capId[MAX_ID],
    unsigned short  count);

STATUS acs_query_clean(
    SEQ_NO           seqNumber,
    VOLID            volId[MAX_ID],
    unsigned short   count);

STATUS acs_query_drive(
    SEQ_NO           seqNumber,
    DRIVEID          driveId[MAX_ID],
    unsigned short   count);

STATUS acs_query_drive_group
(
    SEQ_NO seqNumber,
    GROUP_TYPE groupType,
    unsigned short count,
    GROUPID groupid[MAX_DRG]
);

STATUS acs_query_lock_drive(
    SEQ_NO           seqNumber,
    DRIVEID          driveId[MAX_ID],
    LOCKID           lockId,
    unsigned short   count);

STATUS acs_query_lock_volume(
    SEQ_NO          seqNumber,
    VOLID           volId[MAX_ID],
    LOCKID          lockId,
    unsigned short  count);

STATUS acs_query_lsm(
    SEQ_NO           seqNumber,
    LSMID            lsmId[MAX_ID],
    unsigned short   count);

STATUS acs_query_mm_info(SEQ_NO seqNumber);

STATUS acs_query_mount(
    SEQ_NO          seqNumber,
    VOLID           volId[MAX_ID],
    unsigned short  count);

STATUS acs_query_mount_scratch(
    SEQ_NO          seqNumber,
    POOL            pool[MAX_ID],
    unsigned short  count,
    MEDIA_TYPE      media_type);

STATUS acs_query_pool(
    SEQ_NO          seqNumber,
    POOL            pool[MAX_ID],
    unsigned short  count);

STATUS acs_query_port(
    SEQ_NO           seqNumber,
    PORTID           portId[MAX_ID],
    unsigned short   count);

STATUS acs_query_request(
    SEQ_NO           seqNumber,
    REQ_ID           reqId[MAX_ID],
    unsigned short   count);

STATUS acs_query_scratch(
    SEQ_NO          seqNumber,
    POOL            pool[MAX_ID],
    unsigned short  count);

STATUS acs_query_server(
    SEQ_NO  seqNumber);

STATUS acs_query_subpool_name(
    SEQ_NO          seqNumber,
    unsigned short  count,
    SUBPOOL_NAME    subpoolName[MAX_SPN]);

STATUS acs_query_volume(
    SEQ_NO          seqNumber,
    VOLID           volId[MAX_ID],
    unsigned short  count);

STATUS acs_response(
    int                   timeout,
    SEQ_NO *              seqNumber,
    REQ_ID *              reqId,
    ACS_RESPONSE_TYPE *   type,
    ALIGNED_BYTES         buffer);

STATUS acs_set_cap(
    SEQ_NO           seqNumber,
    CAP_PRIORITY     capPriority,
    CAP_MODE         capMode,
    CAPID            capId[MAX_ID],
    unsigned         short count);

STATUS acs_set_clean(
    SEQ_NO           seqNumber,
    LOCKID           lockId,
    unsigned short   maxUse,
    VOLRANGE         volRange[MAX_ID],
    BOOLEAN          on,
    unsigned short   count);

STATUS acs_set_scratch(
    SEQ_NO          seqNumber,
    LOCKID          lockId,
    POOL            pool,
    VOLRANGE        volRange[MAX_ID],
    BOOLEAN         on,
    unsigned short  count);

STATUS acs_define_pool(
    SEQ_NO           seqNumber,
    unsigned long    lwm,         /* low water mark */
    unsigned long    hwm,         /* high water mark */
    unsigned long    attributes,  /* pool attributes */
    POOL             pool[MAX_ID],
    unsigned short   count);

STATUS acs_delete_pool(
    SEQ_NO          seqNumber,
    POOL            pool[MAX_ID],
    unsigned short  count);

STATUS acs_vary_acs(
    SEQ_NO           seqNumber,
    ACS              acs[MAX_ID],
    STATE            state,
    BOOLEAN          force,
    unsigned short   count);

STATUS acs_vary_cap(
    SEQ_NO          seqNumber,
    CAPID           capId[MAX_ID],
    STATE           state,
    unsigned short  count);

STATUS acs_vary_drive(
    SEQ_NO          seqNumber,
    LOCKID          lockId,
    DRIVEID         driveId[MAX_ID],
    STATE           state,
    unsigned short  count);

STATUS acs_vary_lsm(
    SEQ_NO         seqNumber,
    LSMID          lsmId[MAX_ID],
    STATE          state,
    BOOLEAN         force,
    unsigned short  count);

STATUS acs_vary_port(
    SEQ_NO    seqNumber,
    PORTID    portId[MAX_ID],
    STATE     state,
    unsigned short count);

STATUS acs_register(
    SEQ_NO             seqNumber,
    REGISTRATION_ID    registration_id,
    EVENT_CLASS_TYPE   eventClass[MAX_EVENT_CLASS_TYPE],
    unsigned short     count);

STATUS acs_unregister(
    SEQ_NO             seqNumber,
    REGISTRATION_ID    registration_id,
    EVENT_CLASS_TYPE   eventClass[MAX_EVENT_CLASS_TYPE],
    unsigned short     count);

STATUS acs_check_registration(
    SEQ_NO             seqNumber,
    REGISTRATION_ID    registration_id);

STATUS acs_display(
    SEQ_NO             seqNumber,
    TYPE               display_type,
    DISPLAY_XML_DATA   display_xml_data);

char *acs_type_response(
    ACS_RESPONSE_TYPE rtype);

STATUS acs_virtual_mount(
    SEQ_NO       seqNumber,
    LOCKID       lockId,
    VOLID        volId,
    POOLID       pool_id,
    MGMT_CLAS    mgmtClas,
    MEDIA_TYPE   mtype,
    BOOLEAN      scratch,
    BOOLEAN      readonly,
    BOOLEAN      bypass,
    JOB_NAME     jobName,
    DATASET_NAME datasetName,
    STEP_NAME    stepName,
    DRIVEID      driveId);

STATUS acs_virtual_query_drive(
    SEQ_NO           seqNumber,
    DRIVEID          driveId[MAX_ID],
    unsigned short   count,
    BOOLEAN          virt_aware);

char *acs_type(
    TYPE type);

char *acs_status(
    STATUS status);

char *acs_state(
    STATE state);

char *acs_command(
    COMMAND cmd);

VERSION acs_get_packet_version(
    void);

#endif
