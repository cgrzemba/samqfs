/**********************************************************************
*
*	C Source:		csi_v2_response.c
*	Subsystem:		1
*	Description:	
*	%created_by:	kjs %
*	%date_created:	Fri Jan  6 18:00:10 1995 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %filespec: csi_v2_response.c,1 %  (%full_filespec: 1,csrc,csi_v2_response.c,1 %)";
static char SccsId[]= "@(#) %filespec: csi_v2_response.c,1 %  (%full_filespec: 1,csrc,csi_v2_response.c,1 %)";
#endif
/*
 * Copyright (1995, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_v2_response()
 *
 * Description:
 *      Creates a minimal response to a version 2/3 request, where the only
 *      important information returned in the request is the response
 *      status. Determines which version 2/3 request is being passed in,
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
 */
  
  
/* Header Files: */  
#include <malloc.h>
#include <string.h>
#include "csi.h"  
#include "ssi_pri.h"  
#include "ml_pub.h"  
 
static char     *st_module = "csi_v2_response()";

/* Procedure Type Declarations */ 

static STATUS st_build_start_resp(CSI_V2_START_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_mount_resp(CSI_V2_MOUNT_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_error_resp(CSI_V2_REQUEST_HEADER *reqp,
                                  STATUS response_status);
static STATUS st_build_idle_resp(CSI_V2_IDLE_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_query_resp( CSI_V2_QUERY_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_dismount_resp(CSI_V2_DISMOUNT_REQUEST *reqp,
                                     STATUS response_status);
static STATUS st_build_cancel_resp(CSI_V2_CANCEL_REQUEST *reqp,
                                   STATUS response_status);
static STATUS st_build_vary_resp(CSI_V2_VARY_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_eject_resp(CSI_V2_EJECT_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_enter_resp(CSI_V2_ENTER_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_audit_resp(CSI_V2_AUDIT_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_lock_resp(CSI_V2_LOCK_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_query_lock_resp(CSI_V2_QUERY_LOCK_REQUEST *reqp,
                                       STATUS response_status);
static STATUS st_build_set_clean_resp(CSI_V2_SET_CLEAN_REQUEST *reqp,
                                      STATUS response_status);
static STATUS st_build_set_scratch_resp(CSI_V2_SET_SCRATCH_REQUEST *reqp,
                                        STATUS response_status);
static STATUS st_build_set_cap_resp(CSI_V2_SET_CAP_REQUEST *reqp,
                                    STATUS response_status);
static STATUS st_build_mount_scratch_resp(CSI_V2_MOUNT_SCRATCH_REQUEST *reqp,
                                   STATUS response_status);
static STATUS st_build_define_pool_resp(CSI_V2_DEFINE_POOL_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_delete_pool_resp(CSI_V2_DELETE_POOL_REQUEST *reqp,
                                        STATUS response_status);

STATUS csi_v2_response(CSI_V2_REQUEST *reqp, 
		       STATUS response_status, int * size)
{
    STATUS status;
    switch (reqp->csi_req_header.message_header.command) {
        case COMMAND_AUDIT:
	    status = st_build_audit_resp((CSI_V2_AUDIT_REQUEST *) reqp,
				         response_status);
            *size= sizeof(CSI_V2_AUDIT_RESPONSE);
            break;

        case COMMAND_ENTER:
	    status = st_build_enter_resp((CSI_V2_ENTER_REQUEST *)reqp,
				         response_status);
            *size= sizeof(CSI_V2_ENTER_RESPONSE);
            break;

        case COMMAND_EJECT:
	    status = st_build_eject_resp((CSI_V2_EJECT_REQUEST *) reqp,
				         response_status);
            *size= sizeof(CSI_V2_EJECT_RESPONSE);
            break;

        case COMMAND_VARY:
	    status = st_build_vary_resp((CSI_V2_VARY_REQUEST *) reqp,
			                response_status);
            *size= sizeof(CSI_V2_VARY_RESPONSE);
            break;

        case COMMAND_MOUNT:
            status = st_build_mount_resp((CSI_V2_MOUNT_REQUEST *) reqp,
		                         response_status);
            *size= sizeof(CSI_V2_MOUNT_RESPONSE);
            break;

        case COMMAND_DISMOUNT:
	    status = st_build_dismount_resp((CSI_V2_DISMOUNT_REQUEST *) reqp,
				            response_status);
            *size= sizeof(CSI_V2_DISMOUNT_RESPONSE);
            break;

        case COMMAND_QUERY:
	    status = st_build_query_resp((CSI_V2_QUERY_REQUEST *) reqp,
				         response_status);
            *size = sizeof(CSI_V2_REQUEST_HEADER) +
                    sizeof(RESPONSE_STATUS   ) +
                    sizeof(   TYPE           );
            break;

        case COMMAND_CANCEL:
	    status = st_build_cancel_resp((CSI_V2_CANCEL_REQUEST *) reqp,
				          response_status);
            *size= sizeof(CSI_V2_CANCEL_RESPONSE);
            break;

        case COMMAND_START:
	    status = st_build_start_resp((CSI_V2_START_REQUEST *) reqp,
			                response_status);
            *size= sizeof(CSI_V2_START_RESPONSE);
            break;

        case COMMAND_IDLE:
	    status = st_build_idle_resp((CSI_V2_IDLE_REQUEST *) reqp,
			                response_status);
            *size= sizeof(CSI_V2_IDLE_RESPONSE);
            break;

        case COMMAND_SET_CLEAN:
            status = st_build_set_clean_resp((CSI_V2_SET_CLEAN_REQUEST *)reqp,
		                             response_status);
            *size= sizeof(CSI_V2_SET_CLEAN_RESPONSE);
            break;

        case COMMAND_SET_CAP:
            status = st_build_set_cap_resp((CSI_V2_SET_CAP_REQUEST *)reqp,
		                           response_status);
            *size= sizeof(CSI_V2_SET_CAP_RESPONSE);
            break;

        case COMMAND_SET_SCRATCH:
            status = st_build_set_scratch_resp(
				  (CSI_V2_SET_SCRATCH_REQUEST *)reqp,
		                  response_status);
            *size= sizeof(CSI_V2_SET_SCRATCH_RESPONSE);
            break;

        case COMMAND_DEFINE_POOL:
            status = st_build_define_pool_resp(
				     (CSI_V2_DEFINE_POOL_REQUEST *)reqp,
		                     response_status);
            *size= sizeof(CSI_V2_DEFINE_POOL_RESPONSE);
            break;

        case COMMAND_DELETE_POOL:
            status = st_build_delete_pool_resp(
				     (CSI_V2_DELETE_POOL_REQUEST *)reqp,
		                     response_status);
            *size= sizeof(CSI_V2_DELETE_POOL_RESPONSE);
            break;

        case COMMAND_MOUNT_SCRATCH:
            status = st_build_mount_scratch_resp(
				    (CSI_V2_MOUNT_SCRATCH_REQUEST *) reqp,
		                    response_status);
            *size= sizeof(CSI_V2_MOUNT_SCRATCH_RESPONSE);
            break;

        case COMMAND_LOCK:
        case COMMAND_UNLOCK: 
        case COMMAND_CLEAR_LOCK:
            status = st_build_lock_resp((CSI_V2_LOCK_REQUEST *)reqp,
		                        response_status);
            *size= sizeof(CSI_V2_LOCK_RESPONSE);
            break;
	     
	case COMMAND_QUERY_LOCK:
            status = st_build_query_lock_resp((CSI_V2_QUERY_LOCK_REQUEST *)reqp,
		                              response_status);
            *size= sizeof(CSI_V2_QUERY_LOCK_RESPONSE);
            break;

        default:
      /* Need to create a response that has a status of invalid command
         here */
 
           status = st_build_error_resp((CSI_V2_REQUEST_HEADER *) reqp,
                                        STATUS_INVALID_COMMAND);

            *size = sizeof(CSI_RESPONSE_HEADER);

    } /* end of switch on command */
    return status;
}

STATUS st_build_audit_resp(CSI_V2_AUDIT_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_V2_AUDIT_RESPONSE *arp;

    arp = (CSI_V2_AUDIT_RESPONSE *) malloc(sizeof(CSI_V2_AUDIT_RESPONSE));
    if (NULL == arp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &arp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    arp->message_status.status = response_status;
    memcpy((char *) &arp->cap_id, (char *) &reqp->cap_id, sizeof(CAPID));
    arp->type = reqp->type;
    arp->count = 0;

    memcpy((char *)reqp, (char *)arp, sizeof(CSI_V2_AUDIT_RESPONSE));
    free(arp);

    return STATUS_SUCCESS;
}

STATUS st_build_enter_resp(CSI_V2_ENTER_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_V2_ENTER_RESPONSE *erp;

    erp = (CSI_V2_ENTER_RESPONSE *) malloc(sizeof(CSI_V2_ENTER_RESPONSE));
    if (NULL == erp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &erp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    erp->message_status.status = response_status;
    memcpy((char *) &erp->cap_id, (char *) &reqp->cap_id, sizeof(CAPID));
    erp->count = 0;

    memcpy((char *)reqp, (char *)erp, sizeof(CSI_V2_ENTER_RESPONSE));
    free(erp);

    return STATUS_SUCCESS;

}

STATUS st_build_eject_resp(CSI_V2_EJECT_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_V2_EJECT_RESPONSE *erp;

    erp = (CSI_V2_EJECT_RESPONSE *) malloc(sizeof(CSI_V2_EJECT_RESPONSE));
    if (NULL == erp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &erp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    erp->message_status.status = response_status;
    memcpy((char *) &erp->cap_id, (char *) &reqp->cap_id, sizeof(CAPID));
    erp->count = 0;

    memcpy((char *)reqp, (char *)erp, sizeof(CSI_V2_EJECT_RESPONSE));
    free(erp);

    return STATUS_SUCCESS;

}

STATUS st_build_vary_resp(CSI_V2_VARY_REQUEST *reqp,
		   STATUS response_status)
{

    CSI_V2_VARY_RESPONSE *vrp;

    vrp = (CSI_V2_VARY_RESPONSE *) malloc(sizeof(CSI_V2_VARY_RESPONSE));
    if (NULL == vrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &vrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    vrp->message_status.status = response_status;
    vrp->type = reqp->type;
    vrp->count = 0;

    memcpy((char *)reqp, (char *)vrp, sizeof(CSI_V2_VARY_RESPONSE));
    free(vrp);

    return STATUS_SUCCESS;

}

STATUS st_build_cancel_resp(CSI_V2_CANCEL_REQUEST *reqp,
		     STATUS response_status)
{

    CSI_V2_CANCEL_RESPONSE *crp;

    crp = (CSI_V2_CANCEL_RESPONSE *) malloc(sizeof(CSI_V2_CANCEL_RESPONSE));
    if (NULL == crp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &crp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    crp->message_status.status = response_status;
    crp->request = reqp->request;

    memcpy((char *)reqp, (char *)crp, sizeof(CSI_V2_CANCEL_RESPONSE));
    free(crp);

    return STATUS_SUCCESS;

}


STATUS st_build_dismount_resp(CSI_V2_DISMOUNT_REQUEST *reqp,
		            STATUS response_status)
{

    CSI_V2_DISMOUNT_RESPONSE *drp;

    drp = (CSI_V2_DISMOUNT_RESPONSE *) malloc(sizeof(CSI_V2_DISMOUNT_RESPONSE));
    if (NULL == drp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &drp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    drp->message_status.status = response_status;
    drp->vol_id = reqp->vol_id;
    drp->drive_id = reqp->drive_id;

    memcpy((char *)reqp, (char *)drp, sizeof(CSI_V2_DISMOUNT_RESPONSE));
    free(drp);

    return STATUS_SUCCESS;

}

STATUS st_build_lock_resp(CSI_V2_LOCK_REQUEST *reqp,
		        STATUS response_status)
{

    CSI_V2_LOCK_RESPONSE *lrp;

    lrp = (CSI_V2_LOCK_RESPONSE *) malloc(sizeof(CSI_V2_LOCK_RESPONSE));
    if (NULL == lrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &lrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    lrp->message_status.status = response_status;
    lrp->type = reqp->type;
    lrp->count = 0;

    memcpy((char *)reqp, (char *)lrp, sizeof(CSI_V2_LOCK_RESPONSE));
    free(lrp);

    return STATUS_SUCCESS;

}

STATUS st_build_query_lock_resp(CSI_V2_QUERY_LOCK_REQUEST *reqp,
		              STATUS response_status)
{

    CSI_V2_QUERY_LOCK_RESPONSE *qlrp;

    qlrp = (CSI_V2_QUERY_LOCK_RESPONSE *) 
	  malloc(sizeof(CSI_V2_QUERY_LOCK_RESPONSE));
    if (NULL == qlrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &qlrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    qlrp->message_status.status = response_status;
    qlrp->type = reqp->type;
    qlrp->count = 0;

    memcpy((char *)reqp, (char *)qlrp, sizeof(CSI_V2_QUERY_LOCK_RESPONSE));
    free(qlrp);

    return STATUS_SUCCESS;

}

STATUS st_build_query_resp( CSI_V2_QUERY_REQUEST *reqp,
                    STATUS response_status)
{

    CSI_V2_QUERY_RESPONSE *qrp;

    int size;
    qrp = (CSI_V2_QUERY_RESPONSE *) malloc(sizeof(CSI_V2_QUERY_RESPONSE));
    if (NULL == qrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
 
    memcpy((char *) &qrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_V2_REQUEST_HEADER));
    qrp->message_status.status = response_status;
    qrp->type = reqp->type;
    qrp->count = 0;
    size = sizeof(CSI_V2_REQUEST_HEADER) +
           sizeof(RESPONSE_STATUS   ) +
           sizeof(   TYPE           );
          
    memcpy((char *)reqp, (char *)qrp, size);

    free(qrp);

    return STATUS_SUCCESS;
}


STATUS st_build_set_clean_resp(CSI_V2_SET_CLEAN_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_V2_SET_CLEAN_RESPONSE *scrp;

    scrp = (CSI_V2_SET_CLEAN_RESPONSE *) 
	  malloc(sizeof(CSI_V2_SET_CLEAN_RESPONSE));
    if (NULL == scrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &scrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    scrp->message_status.status = response_status;
    scrp->max_use = reqp->max_use;
    scrp->count = 0;

    memcpy((char *)reqp, (char *)scrp, sizeof(CSI_V2_SET_CLEAN_RESPONSE));
    free(scrp);

    return STATUS_SUCCESS;

}

STATUS st_build_set_scratch_resp(CSI_V2_SET_SCRATCH_REQUEST *reqp,
		    STATUS response_status)
{
    
    CSI_V2_SET_SCRATCH_RESPONSE *ssrp;

    ssrp = (CSI_V2_SET_SCRATCH_RESPONSE *) 
	  malloc(sizeof(CSI_V2_SET_SCRATCH_RESPONSE));
    if (NULL == ssrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &ssrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    ssrp->message_status.status = response_status;
    memcpy((char *) &ssrp->pool_id, (char *) &reqp->pool_id, sizeof(POOLID));
    ssrp->count = 0;

    memcpy((char *)reqp, (char *)ssrp, sizeof(CSI_V2_SET_SCRATCH_RESPONSE));
    free(ssrp);

    return STATUS_SUCCESS;

}

STATUS st_build_set_cap_resp(CSI_V2_SET_CAP_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_V2_SET_CAP_RESPONSE *scrp;

    scrp = (CSI_V2_SET_CAP_RESPONSE *) 
	  malloc(sizeof(CSI_V2_SET_CAP_RESPONSE));
    if (NULL == scrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &scrp->request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    scrp->message_status.status = response_status;
    scrp->cap_priority = reqp->cap_priority;
    scrp->cap_mode = reqp->cap_mode;
    scrp->count = 0;

    memcpy((char *)reqp, (char *)scrp, sizeof(CSI_V2_SET_CAP_RESPONSE));
    free(scrp);

    return STATUS_SUCCESS;

}

STATUS st_build_define_pool_resp(CSI_V2_DEFINE_POOL_REQUEST *reqp,
		    STATUS response_status) 
{ 
    CSI_V2_DEFINE_POOL_RESPONSE *dprp;

    dprp = (CSI_V2_DEFINE_POOL_RESPONSE *) 
		malloc(sizeof(CSI_V2_DEFINE_POOL_RESPONSE)); 
    if (NULL == dprp) 
    { 
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &dprp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    dprp->message_status.status = response_status;
    dprp->low_water_mark = reqp->low_water_mark;
    dprp->high_water_mark = reqp->high_water_mark;
    dprp->pool_attributes = reqp->pool_attributes;
    dprp->count = 0;

    memcpy((char *)reqp, (char *)dprp, sizeof(CSI_V2_DEFINE_POOL_RESPONSE));
    free(dprp);

    return STATUS_SUCCESS;

}

STATUS st_build_delete_pool_resp(CSI_V2_DELETE_POOL_REQUEST *reqp,
		          STATUS response_status)
{

    CSI_V2_DELETE_POOL_RESPONSE *dprp;

    dprp = (CSI_V2_DELETE_POOL_RESPONSE *) 
	  malloc(sizeof(CSI_V2_DELETE_POOL_RESPONSE));
    if (NULL == dprp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &dprp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    dprp->message_status.status = response_status;
    dprp->count = 0;

    memcpy((char *)reqp, (char *)dprp, sizeof(CSI_V2_DELETE_POOL_RESPONSE));
    free(dprp);

    return STATUS_SUCCESS;

}

STATUS st_build_error_resp(CSI_V2_REQUEST_HEADER *reqp,
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
						sizeof(CSI_V2_REQUEST_HEADER));
    rp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)rp, sizeof(CSI_RESPONSE_HEADER));
    free(rp);

    return STATUS_SUCCESS;

}


STATUS st_build_idle_resp(CSI_V2_IDLE_REQUEST *reqp,
		   STATUS response_status)
{

    CSI_V2_IDLE_RESPONSE *irp;

    irp = (CSI_V2_IDLE_RESPONSE *) malloc(sizeof(CSI_V2_IDLE_RESPONSE));
    if (NULL == irp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &irp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    irp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)irp, sizeof(CSI_V2_IDLE_RESPONSE));
    free(irp);

    return STATUS_SUCCESS;

}


STATUS st_build_mount_resp(CSI_V2_MOUNT_REQUEST *reqp,
		         STATUS response_status)
{

    CSI_V2_MOUNT_RESPONSE *mrp;

    mrp = (CSI_V2_MOUNT_RESPONSE *) malloc(sizeof(CSI_V2_MOUNT_RESPONSE));
    if (NULL == mrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &mrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    mrp->message_status.status = response_status;
    mrp->vol_id = reqp->vol_id;
    mrp->drive_id = reqp->drive_id[0];

    memcpy((char *)reqp, (char *)mrp, sizeof(CSI_V2_MOUNT_RESPONSE));
    free(mrp);

    return STATUS_SUCCESS;

}

STATUS st_build_mount_scratch_resp(CSI_V2_MOUNT_SCRATCH_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_V2_MOUNT_SCRATCH_RESPONSE *msrp;

    int i;
    msrp = (CSI_V2_MOUNT_SCRATCH_RESPONSE *) 
	  malloc(sizeof(CSI_V2_MOUNT_SCRATCH_RESPONSE));
    if (NULL == msrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &msrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    msrp->message_status.status = response_status;
    memcpy((char *) &msrp->pool_id, (char *) &reqp->pool_id, sizeof(POOLID));
    for (i = 0; i < EXTERNAL_LABEL_SIZE; i ++)
    {  
        msrp->vol_id.external_label[i] = 'X';
    }
    msrp->vol_id.external_label[EXTERNAL_LABEL_SIZE + 1] = '\0';

    msrp->drive_id = reqp->drive_id[0];

    memcpy((char *)reqp, (char *)msrp, sizeof(CSI_V2_MOUNT_SCRATCH_RESPONSE));
    free(msrp);

    return STATUS_SUCCESS;

}

STATUS st_build_start_resp(CSI_V2_START_REQUEST *reqp,
		   STATUS response_status)
{

    CSI_V2_START_RESPONSE *srp;

    srp = (CSI_V2_START_RESPONSE *) malloc(sizeof(CSI_V2_START_RESPONSE));
    if (NULL == srp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &srp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V2_REQUEST_HEADER));
    srp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)srp, sizeof(CSI_V2_START_RESPONSE));
    free(srp);

    return STATUS_SUCCESS;

}

