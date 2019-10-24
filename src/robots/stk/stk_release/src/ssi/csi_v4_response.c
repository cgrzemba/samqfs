/**********************************************************************
*
*	C Source:		csi_v4_response.c
*	Subsystem:		1
*	Description:	
*	%created_by:	kjs %
*	%date_created:	Fri Jan  6 18:01:23 1995 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %filespec: csi_v4_response.c,1 %  (%full_filespec: 1,csrc,csi_v4_response.c,1 %)";
static char SccsId[]= "@(#) %filespec: csi_v4_response.c,1 %  (%full_filespec: 1,csrc,csi_v4_response.c,1 %)";
#endif
/*
 * Copyright (1995, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_v4_response()
 *
 * Description:
 *      Creates a minimal response to a version 4 request, where the only
 *      important information returned in the request is the response
 *      status. Determines which version 4 request is being passed in,
 *      and dispatches to the appropriate static function, casting the 
 *      generic request and response pointers to specific
 *      request/response pointers.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *
 * Revision History:
 * 	Ken Stickney 	5-Jan-1995	Original.
 *      Scott Siao     12-Oct-2001      Added register, unregister, check_registration
 *      Scott Siao     12-Nov-2002      Added mount_pinfo.
 */
  
  
/* Header Files: */  
#include <malloc.h>
#include "csi.h"  
#include "ssi_pri.h"  
#include "ml_pub.h"  

static char     *st_module = "csi_v4_response()";
 
/* Procedure Type Declarations */ 

static STATUS st_build_start_resp(CSI_START_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_mount_resp(CSI_MOUNT_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_error_resp(CSI_REQUEST_HEADER *reqp,
                                  STATUS response_status);
static STATUS st_build_idle_resp(CSI_IDLE_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_query_resp( CSI_QUERY_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_dismount_resp(CSI_DISMOUNT_REQUEST *reqp,
                                     STATUS response_status);
static STATUS st_build_cancel_resp(CSI_CANCEL_REQUEST *reqp,
                                   STATUS response_status);
static STATUS st_build_vary_resp(CSI_VARY_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_eject_resp(CSI_EJECT_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_enter_resp(CSI_ENTER_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_audit_resp(CSI_AUDIT_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_lock_resp(CSI_LOCK_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_query_lock_resp(CSI_QUERY_LOCK_REQUEST *reqp,
                                       STATUS response_status);
static STATUS st_build_set_clean_resp(CSI_SET_CLEAN_REQUEST *reqp,
                                      STATUS response_status);
static STATUS st_build_set_scratch_resp(CSI_SET_SCRATCH_REQUEST *reqp,
                                        STATUS response_status);
static STATUS st_build_set_cap_resp(CSI_SET_CAP_REQUEST *reqp,
                                    STATUS response_status);
static STATUS st_build_mount_scr_resp(CSI_MOUNT_SCRATCH_REQUEST *reqp,
                                   STATUS response_status);
static STATUS st_build_define_pool_resp(CSI_DEFINE_POOL_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_delete_pool_resp(CSI_DELETE_POOL_REQUEST *reqp,
                                        STATUS response_status);
static STATUS st_build_register_resp(CSI_REGISTER_REQUEST *reqp,
                                        STATUS response_status);
static STATUS st_build_unregister_resp(CSI_UNREGISTER_REQUEST *reqp,
                                        STATUS response_status);
static STATUS st_build_check_registration_resp(CSI_CHECK_REGISTRATION_REQUEST *reqp,
                                        STATUS response_status);
static STATUS st_build_display_resp(CSI_DISPLAY_REQUEST *reqp,
                                        STATUS response_status);
static STATUS st_build_mount_pinfo_resp(CSI_MOUNT_PINFO_REQUEST *reqp,
                                        STATUS response_status);


STATUS csi_v4_response(CSI_REQUEST *reqp, STATUS response_status, 
		       int * size)
{
    STATUS status;
    switch (reqp->csi_req_header.message_header.command) {
        case COMMAND_AUDIT:
	    status = st_build_audit_resp((CSI_AUDIT_REQUEST *) reqp,
				         response_status);
            *size = sizeof(CSI_AUDIT_RESPONSE);
            break;

        case COMMAND_CANCEL:
	    status = st_build_cancel_resp((CSI_CANCEL_REQUEST *) reqp,
				          response_status);
            *size = sizeof(CSI_CANCEL_RESPONSE);
            break;

        case COMMAND_DISMOUNT:
	    status = st_build_dismount_resp((CSI_DISMOUNT_REQUEST *) reqp,
				            response_status);
            *size = sizeof(CSI_DISMOUNT_RESPONSE);
            break;

        case COMMAND_EJECT:
            status = st_build_eject_resp((CSI_EJECT_REQUEST *) reqp,
                                         response_status);
            *size = sizeof(CSI_EJECT_RESPONSE);
            break;

        case COMMAND_ENTER:
            status = st_build_enter_resp((CSI_ENTER_REQUEST *)reqp,
                                         response_status);
            *size = sizeof(CSI_ENTER_RESPONSE);
            break;

        case COMMAND_IDLE:
	    status = st_build_idle_resp((CSI_IDLE_REQUEST *) reqp,
			                response_status);
            *size = sizeof(CSI_IDLE_RESPONSE);
            break;

        case COMMAND_MOUNT:
	    status = st_build_mount_resp((CSI_MOUNT_REQUEST *) reqp,
				         response_status);
            *size = sizeof(CSI_MOUNT_RESPONSE);
            break;

        case COMMAND_MOUNT_SCRATCH:
	    status = st_build_mount_scr_resp((CSI_MOUNT_SCRATCH_REQUEST*) reqp,
				              response_status);
            *size = sizeof(CSI_MOUNT_SCRATCH_RESPONSE);
            break;

        case COMMAND_LOCK:
        case COMMAND_UNLOCK:
        case COMMAND_CLEAR_LOCK:
            status = st_build_lock_resp((CSI_LOCK_REQUEST *)reqp,
                                        response_status);
            *size = sizeof(CSI_LOCK_RESPONSE);
            break;

        case COMMAND_QUERY_LOCK:
            status = st_build_query_lock_resp((CSI_QUERY_LOCK_REQUEST *)reqp,
                                              response_status);
            *size = sizeof(CSI_QUERY_LOCK_RESPONSE);
            break;

        case COMMAND_QUERY:
            status = st_build_query_resp((CSI_QUERY_REQUEST *) reqp,
                                         response_status);
            *size = sizeof(CSI_REQUEST_HEADER) +
		    sizeof(RESPONSE_STATUS   ) +
		    sizeof(   TYPE           ); 
            break;

        case COMMAND_START:
            status = st_build_start_resp((CSI_START_REQUEST *) reqp, 
                                        response_status);
            *size = sizeof(CSI_START_RESPONSE);
            break;

        case COMMAND_VARY:
            status = st_build_vary_resp((CSI_VARY_REQUEST *) reqp,
                                        response_status);
            *size = sizeof(CSI_VARY_RESPONSE);
            break;

        case COMMAND_DEFINE_POOL:
            status = st_build_define_pool_resp((CSI_DEFINE_POOL_REQUEST *)reqp,
                                                response_status);
            *size = sizeof(CSI_DEFINE_POOL_RESPONSE);
            break;

        case COMMAND_DELETE_POOL:
            status = st_build_delete_pool_resp((CSI_DELETE_POOL_REQUEST *)reqp,
                                                response_status);
            *size = sizeof(CSI_DELETE_POOL_RESPONSE);
            break;

        case COMMAND_SET_CAP:
            status = st_build_set_cap_resp((CSI_SET_CAP_REQUEST *)reqp,
                                           response_status);
            *size = sizeof(CSI_SET_CAP_RESPONSE);
            break;

        case COMMAND_SET_CLEAN:
            status = st_build_set_clean_resp((CSI_SET_CLEAN_REQUEST *)reqp,
                                             response_status);
            *size = sizeof(CSI_SET_CLEAN_RESPONSE);
            break;

        case COMMAND_SET_SCRATCH:
            status = st_build_set_scratch_resp((CSI_SET_SCRATCH_REQUEST *)reqp,
                                               response_status);
            *size = sizeof(CSI_SET_SCRATCH_RESPONSE);
            break;

        case COMMAND_REGISTER:
            status = st_build_register_resp((CSI_REGISTER_REQUEST *)reqp,
                                               response_status);
            *size = sizeof(CSI_REGISTER_RESPONSE);
            break;

        case COMMAND_UNREGISTER:
            status = st_build_unregister_resp((CSI_UNREGISTER_REQUEST *)reqp,
                                               response_status);
            *size = sizeof(CSI_UNREGISTER_RESPONSE);
            break;

        case COMMAND_CHECK_REGISTRATION:
            status = st_build_check_registration_resp((CSI_CHECK_REGISTRATION_REQUEST *)reqp,
                                               response_status);
            *size = sizeof(CSI_CHECK_REGISTRATION_RESPONSE);
            break;

        case COMMAND_DISPLAY:
            status = st_build_display_resp((CSI_DISPLAY_REQUEST *)reqp,
                                               response_status);
            *size = sizeof(CSI_DISPLAY_RESPONSE);
            break;

        case COMMAND_MOUNT_PINFO:
            status = st_build_mount_pinfo_resp((CSI_MOUNT_PINFO_REQUEST *)reqp,
                                               response_status);
            *size = sizeof(CSI_MOUNT_PINFO_RESPONSE);
            break;

        default:

      /* Need to create a response that has a status of invalid command
         here */
 
           status = st_build_error_resp((CSI_REQUEST_HEADER *) reqp,
                                        STATUS_INVALID_COMMAND);

           *size = sizeof(CSI_RESPONSE_HEADER);

        break;
   } /* end of switch on command */
   return status;
}


STATUS st_build_audit_resp(
		    CSI_AUDIT_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_AUDIT_RESPONSE *arp;

    arp = (CSI_AUDIT_RESPONSE *) malloc(sizeof(CSI_AUDIT_RESPONSE));
    if (NULL == arp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &arp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_REQUEST_HEADER));
    arp->message_status.status = response_status;
    memcpy((char *) &arp->cap_id, (char *) &reqp->cap_id, sizeof(CAPID));
    arp->type = reqp->type;
    arp->count = 0;

    memcpy((char *)reqp, (char *)arp, sizeof(CSI_AUDIT_RESPONSE));
    free(arp);

    return STATUS_SUCCESS;
}


STATUS st_build_cancel_resp(CSI_CANCEL_REQUEST *reqp,
		     STATUS response_status)
{

    CSI_CANCEL_RESPONSE *crp;

    crp = (CSI_CANCEL_RESPONSE *) malloc(sizeof(CSI_CANCEL_RESPONSE));
    if (NULL == crp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &crp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_REQUEST_HEADER));
    crp->message_status.status = response_status;
    crp->request = reqp->request;

    memcpy((char *)reqp, (char *)crp, sizeof(CSI_CANCEL_RESPONSE));
    free(crp);

    return STATUS_SUCCESS;
}


STATUS st_build_dismount_resp(CSI_DISMOUNT_REQUEST *reqp,
		       STATUS response_status)
{

    CSI_DISMOUNT_RESPONSE *drp;

    drp = (CSI_DISMOUNT_RESPONSE *) malloc(sizeof(CSI_DISMOUNT_RESPONSE));
    if (NULL == drp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &drp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_REQUEST_HEADER));
    drp->message_status.status = response_status;
    drp->vol_id = reqp->vol_id;
    drp->drive_id = reqp->drive_id;

    memcpy((char *)reqp, (char *)drp, sizeof(CSI_DISMOUNT_RESPONSE));
    free(drp);

    return STATUS_SUCCESS;
}

STATUS st_build_lock_resp(CSI_LOCK_REQUEST *reqp,
                        STATUS response_status)
{

    CSI_LOCK_RESPONSE *lrp;

    lrp = (CSI_LOCK_RESPONSE *) malloc(sizeof(CSI_LOCK_RESPONSE));
    if (NULL == lrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &lrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                                sizeof(CSI_REQUEST_HEADER));
    lrp->message_status.status = response_status;
    lrp->type = reqp->type;
    lrp->count = 0;

    memcpy((char *)reqp, (char *)lrp, sizeof(CSI_LOCK_RESPONSE));
    free(lrp);

    return STATUS_SUCCESS;
}

STATUS st_build_query_lock_resp(CSI_QUERY_LOCK_REQUEST *reqp,
                              STATUS response_status)
{

    CSI_QUERY_LOCK_RESPONSE *qlrp;

    qlrp = (CSI_QUERY_LOCK_RESPONSE *) malloc(sizeof(CSI_QUERY_LOCK_RESPONSE));
    if (NULL == qlrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
 
    memcpy((char *) &qlrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                              sizeof(CSI_REQUEST_HEADER));
    qlrp->message_status.status = response_status;
    qlrp->type = reqp->type;
    qlrp->count = 0;

    memcpy((char *)reqp, (char *)qlrp, sizeof(CSI_QUERY_LOCK_RESPONSE));
    free(qlrp);

    return STATUS_SUCCESS;
}

STATUS st_build_query_resp(CSI_QUERY_REQUEST *reqp,
                              STATUS response_status)
{

    CSI_QUERY_RESPONSE *qrp;

    int size;
    qrp = (CSI_QUERY_RESPONSE *) malloc(sizeof(CSI_QUERY_RESPONSE));
    if (NULL == qrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
       
    memcpy((char *) &qrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                               sizeof(CSI_REQUEST_HEADER));
    qrp->message_status.status = response_status;
    qrp->type = reqp->type;
    size = sizeof(CSI_REQUEST_HEADER) +
	   sizeof(RESPONSE_STATUS   ) +
	   sizeof(   TYPE           ); 
       
    memcpy((char *)reqp, (char *)qrp, size);
    free(qrp);
       
    return STATUS_SUCCESS;
}

STATUS st_build_enter_resp(CSI_ENTER_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_ENTER_RESPONSE *erp;

    erp = (CSI_ENTER_RESPONSE *) malloc(sizeof(CSI_ENTER_RESPONSE));
    if (NULL == erp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
 
    memcpy((char *) &erp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                       sizeof(CSI_REQUEST_HEADER));
    erp->message_status.status = response_status;
    memcpy((char *) &erp->cap_id, (char *) &reqp->cap_id, sizeof(CAPID));
    erp->count = 0;

    memcpy((char *)reqp, (char *)erp, sizeof(CSI_ENTER_RESPONSE));
    free(erp);

    return STATUS_SUCCESS;
}

STATUS st_build_eject_resp(CSI_EJECT_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_EJECT_RESPONSE *erp;

    erp = (CSI_EJECT_RESPONSE *) malloc(sizeof(CSI_EJECT_RESPONSE));
    if (NULL == erp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &erp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_REQUEST_HEADER));
    erp->message_status.status = response_status;
    memcpy((char *) &erp->cap_id, (char *) &reqp->cap_id, sizeof(CAPID));
    erp->count = 0;

    memcpy((char *)reqp, (char *)erp, sizeof(CSI_ENTER_RESPONSE));
    free(erp);

    return STATUS_SUCCESS;
}


STATUS st_build_error_resp(CSI_REQUEST_HEADER *reqp,
		   STATUS response_status)
{

    CSI_RESPONSE_HEADER *rp;

    rp = (CSI_RESPONSE_HEADER *) malloc(sizeof(CSI_RESPONSE_HEADER));
    if (NULL == rp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &rp->csi_request_header,
			(const char *) &reqp->csi_header,
						sizeof(CSI_REQUEST_HEADER));
    rp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)rp, sizeof(CSI_IDLE_RESPONSE));
    free(rp);

    return STATUS_SUCCESS;
}


STATUS st_build_idle_resp(CSI_IDLE_REQUEST *reqp,
		   STATUS response_status)
{

    CSI_IDLE_RESPONSE *irp;

    irp = (CSI_IDLE_RESPONSE *) malloc(sizeof(CSI_IDLE_RESPONSE));
    if (NULL == irp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &irp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_REQUEST_HEADER));
    irp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)irp, sizeof(CSI_IDLE_RESPONSE));
    free(irp);

    return STATUS_SUCCESS;
}


STATUS st_build_mount_resp(CSI_MOUNT_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_MOUNT_RESPONSE *mrp;

    mrp = (CSI_MOUNT_RESPONSE *) malloc(sizeof(CSI_MOUNT_RESPONSE));
    if (NULL == mrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &mrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_REQUEST_HEADER));
    mrp->message_status.status = response_status;
    mrp->vol_id = reqp->vol_id;
    mrp->drive_id = reqp->drive_id[0];

    memcpy((char *)reqp, (char *)mrp, sizeof(CSI_MOUNT_RESPONSE));
    free(mrp);

    return STATUS_SUCCESS;
}


STATUS st_build_mount_scr_resp(CSI_MOUNT_SCRATCH_REQUEST *reqp,
			STATUS response_status)
{

    CSI_MOUNT_SCRATCH_RESPONSE *msrp;

    int i;
    msrp = (CSI_MOUNT_SCRATCH_RESPONSE *) 
	   malloc(sizeof(CSI_MOUNT_SCRATCH_RESPONSE));
    if (NULL == msrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &msrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_REQUEST_HEADER));
    msrp->message_status.status = response_status;
    msrp->pool_id = reqp->pool_id;
    msrp->drive_id = reqp->drive_id[0];
    for (i = 0; i < EXTERNAL_LABEL_SIZE; i ++)
    {
        msrp->vol_id.external_label[i] = 'X';
    }
    msrp->vol_id.external_label[EXTERNAL_LABEL_SIZE + 1] = '\0';

    memcpy((char *)reqp, (char *)msrp, sizeof(CSI_MOUNT_SCRATCH_RESPONSE));
    free(msrp);

    return STATUS_SUCCESS;
}

STATUS st_build_vary_resp(CSI_VARY_REQUEST *reqp,
                   STATUS response_status)
{

    CSI_VARY_RESPONSE *vrp;

    vrp = (CSI_VARY_RESPONSE *) malloc(sizeof(CSI_VARY_RESPONSE));
    if (NULL == vrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &vrp->csi_request_header,
                        (const char *) &reqp->csi_request_header, 
                                               sizeof(CSI_REQUEST_HEADER));
    vrp->message_status.status = response_status;
    vrp->state = reqp->state;
    vrp->type = reqp->type;
    vrp->count = 0;

    memcpy((char *)reqp, (char *)vrp, sizeof(CSI_VARY_RESPONSE));
    free(vrp);

    return STATUS_SUCCESS;
}

STATUS st_build_define_pool_resp(CSI_DEFINE_POOL_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_DEFINE_POOL_RESPONSE *dprp;

    dprp = (CSI_DEFINE_POOL_RESPONSE *)
                malloc(sizeof(CSI_DEFINE_POOL_RESPONSE));
    if (NULL == dprp)
    { 
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &dprp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                       sizeof(CSI_REQUEST_HEADER));
    dprp->message_status.status = response_status;
    dprp->low_water_mark = reqp->low_water_mark;
    dprp->high_water_mark = reqp->high_water_mark;
    dprp->pool_attributes = reqp->pool_attributes;
    dprp->count = 0;
 
    memcpy((char *)reqp, (char *)dprp, sizeof(CSI_DEFINE_POOL_RESPONSE));
    free(dprp);

    return STATUS_SUCCESS;
}

STATUS st_build_delete_pool_resp(CSI_DELETE_POOL_REQUEST *reqp,
                          STATUS response_status)
{

    CSI_DELETE_POOL_RESPONSE *dprp;

    dprp = (CSI_DELETE_POOL_RESPONSE *)
          malloc(sizeof(CSI_DELETE_POOL_RESPONSE));
    if (NULL == dprp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
 
    memcpy((char *) &dprp->csi_request_header,
                        (const char *) &reqp->csi_request_header, 
                                       sizeof(CSI_REQUEST_HEADER));
    dprp->message_status.status = response_status;
    dprp->count = 0;
 
    memcpy((char *)reqp, (char *)dprp, sizeof(CSI_DELETE_POOL_RESPONSE));
    free(dprp);

    return STATUS_SUCCESS;
}

STATUS st_build_set_clean_resp(CSI_SET_CLEAN_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_SET_CLEAN_RESPONSE *scrp;

    scrp = (CSI_SET_CLEAN_RESPONSE *)
          malloc(sizeof(CSI_SET_CLEAN_RESPONSE));
    if (NULL == scrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }  
 
    memcpy((char *) &scrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                           sizeof(CSI_REQUEST_HEADER));
    scrp->message_status.status = response_status;
    scrp->max_use = reqp->max_use;
    scrp->count = 0;
 
    memcpy((char *)reqp, (char *)scrp, sizeof(CSI_SET_CLEAN_RESPONSE));
    free(scrp);

    return STATUS_SUCCESS;
}

STATUS st_build_set_scratch_resp(CSI_SET_SCRATCH_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_SET_SCRATCH_RESPONSE *ssrp;

    ssrp = (CSI_SET_SCRATCH_RESPONSE *)
          malloc(sizeof(CSI_SET_SCRATCH_RESPONSE));
    if (NULL == ssrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
 
    memcpy((char *) &ssrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_REQUEST_HEADER));
    ssrp->message_status.status = response_status;
    memcpy((char *) &ssrp->pool_id, (char *) &reqp->pool_id, sizeof(POOLID));
    ssrp->count = 0;
 
    memcpy((char *)reqp, (char *)ssrp, sizeof(CSI_SET_SCRATCH_RESPONSE));
    free(ssrp);

    return STATUS_SUCCESS;
}

STATUS st_build_set_cap_resp(CSI_SET_CAP_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_SET_CAP_RESPONSE *scrp;

    scrp = (CSI_SET_CAP_RESPONSE *)
          malloc(sizeof(CSI_SET_CAP_RESPONSE));
    if (NULL == scrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
 
    memcpy((char *) &scrp->request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_REQUEST_HEADER));
    scrp->message_status.status = response_status;
    scrp->cap_priority = reqp->cap_priority;
    scrp->cap_mode = reqp->cap_mode;
    scrp->count = 0;
 
    memcpy((char *)reqp, (char *)scrp, sizeof(CSI_SET_CAP_RESPONSE));
    free(scrp);
 
    return STATUS_SUCCESS;
}

STATUS st_build_start_resp(CSI_START_REQUEST *reqp,
                   STATUS response_status)
{

    CSI_START_RESPONSE *srp;

    srp = (CSI_START_RESPONSE *) malloc(sizeof(CSI_START_RESPONSE));
    if (NULL == srp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &srp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                       sizeof(CSI_REQUEST_HEADER));
    srp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)srp, sizeof(CSI_START_RESPONSE));
    free(srp);
 
    return STATUS_SUCCESS;
}

STATUS st_build_register_resp(CSI_REGISTER_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_REGISTER_RESPONSE *rrp;

    rrp = (CSI_REGISTER_RESPONSE *) malloc(sizeof(CSI_REGISTER_RESPONSE));
    if (NULL == rrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &rrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_REQUEST_HEADER));
    rrp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)rrp, sizeof(CSI_REGISTER_RESPONSE));
    free(rrp);

    return STATUS_SUCCESS;
}


STATUS st_build_unregister_resp(CSI_UNREGISTER_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_UNREGISTER_RESPONSE *unrrp;

    unrrp = (CSI_UNREGISTER_RESPONSE *) malloc(sizeof(CSI_UNREGISTER_RESPONSE));
    if (NULL == unrrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &unrrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_REQUEST_HEADER));
    unrrp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)unrrp, sizeof(CSI_UNREGISTER_RESPONSE));
    free(unrrp);

    return STATUS_SUCCESS;
}

STATUS st_build_check_registration_resp(CSI_CHECK_REGISTRATION_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_CHECK_REGISTRATION_RESPONSE *crrp;

    crrp = (CSI_CHECK_REGISTRATION_RESPONSE *) malloc(sizeof(CSI_CHECK_REGISTRATION_RESPONSE));
    if (NULL == crrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &crrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_REQUEST_HEADER));
    crrp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)crrp, sizeof(CSI_CHECK_REGISTRATION_RESPONSE));
    free(crrp);

    return STATUS_SUCCESS;
}

STATUS st_build_display_resp(CSI_DISPLAY_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_DISPLAY_RESPONSE *drrp;
    int size;

    drrp = (CSI_DISPLAY_RESPONSE *) malloc(sizeof(CSI_DISPLAY_RESPONSE));
    if (NULL == drrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &drrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_REQUEST_HEADER));
    drrp->message_status.status = response_status;

    drrp->display_type = reqp->display_type;

    /*size = sizeof(CSI_REQUEST_HEADER) +
	   sizeof(RESPONSE_STATUS   ) +
	   sizeof(   TYPE           ); 
       
    memcpy((char *)reqp, (char *)drrp, size);*/
    memcpy((char *)reqp, (char *)drrp, sizeof(CSI_DISPLAY_RESPONSE));
    free(drrp);
       
    return STATUS_SUCCESS;
}

STATUS st_build_mount_pinfo_resp(CSI_MOUNT_PINFO_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_MOUNT_PINFO_RESPONSE *drrp;
    int size;

    drrp = (CSI_MOUNT_PINFO_RESPONSE *) malloc(sizeof(CSI_MOUNT_PINFO_RESPONSE));
    if (NULL == drrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &drrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_REQUEST_HEADER));
    drrp->message_status.status = response_status;

    drrp->pool_id = reqp->pool_id;
    drrp->drive_id = reqp->drive_id;
    drrp->vol_id = reqp->vol_id;

    memcpy((char *)reqp, (char *)drrp, sizeof(CSI_MOUNT_PINFO_RESPONSE));
    free(drrp);
       
    return STATUS_SUCCESS;
}
