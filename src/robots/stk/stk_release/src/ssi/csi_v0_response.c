/**********************************************************************
*
*	C Source:		csi_v0_response.c
*	Subsystem:		1
*	Description:	
*	%created_by:	kjs %
*	%date_created:	Fri Jan  6 17:59:26 1995 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %filespec: csi_v0_response.c,1 %  (%full_filespec: 1,csrc,csi_v0_response.c,1 %)";
static char SccsId[] = "@(#) %filespec: csi_v0_response.c,1 %  (%full_filespec: 1,csrc,csi_v0_response.c,1 %)";
#endif
/*
 * Copyright (1995, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_v0_response()
 *
 * Description:
 *      Creates a minimal response to a version 0 request, where the only
 *      important information returned in the request is the response
 *      status. Determines which version 0 request is being passed in,
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
#include "csi.h"
#include "ssi_pri.h"
#include "ml_pub.h"

static char     *st_module = "csi_v0_response()";

/* Procedure Type Declarations */

static STATUS st_build_start_resp(CSI_V0_START_REQUEST *reqp,
		                  STATUS response_status);
static STATUS st_build_mount_resp(CSI_V0_MOUNT_REQUEST *reqp,
		                  STATUS response_status);
static STATUS st_build_error_resp(CSI_V0_REQUEST_HEADER *reqp,
		                  STATUS response_status);
static STATUS st_build_idle_resp(CSI_V0_IDLE_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_query_resp( CSI_V0_QUERY_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_dismount_resp(CSI_V0_DISMOUNT_REQUEST *reqp,
                                     STATUS response_status);
static STATUS st_build_cancel_resp(CSI_V0_CANCEL_REQUEST *reqp,
                                   STATUS response_status);
static STATUS st_build_vary_resp(CSI_V0_VARY_REQUEST *reqp,
                                 STATUS response_status);
static STATUS st_build_eject_resp(CSI_V0_EJECT_REQUEST *reqp,
                                  STATUS response_status);
static STATUS st_build_enter_resp(CSI_V0_ENTER_REQUEST *reqp,
                           STATUS response_status);
static STATUS st_build_audit_resp(CSI_V0_AUDIT_REQUEST *reqp,
                    STATUS response_status);


STATUS csi_v0_response(CSI_V0_REQUEST *reqp, 
		       STATUS response_status, int * size)
{
   
    STATUS status;
    COMMAND command;

    command = reqp->csi_req_header.message_header.command;

    /* Determine the specific request made */
    switch (command) {
        case COMMAND_AUDIT:
	    status = st_build_audit_resp((CSI_V0_AUDIT_REQUEST *) reqp,
				         response_status);
            *size = sizeof(CSI_V0_AUDIT_RESPONSE);
            break;

        case COMMAND_ENTER:
	    status = st_build_enter_resp((CSI_V0_ENTER_REQUEST *)reqp,
				         response_status);
            *size = sizeof(CSI_V0_ENTER_RESPONSE);
            break;

        case COMMAND_EJECT:
	    status = st_build_eject_resp((CSI_V0_EJECT_REQUEST *) reqp,
				         response_status);
            *size = sizeof(CSI_V0_EJECT_RESPONSE);
            break;

        case COMMAND_VARY:
	    status = st_build_vary_resp((CSI_V0_VARY_REQUEST *) reqp,
			                response_status);
            *size = sizeof(CSI_V0_VARY_RESPONSE);
            break;

        case COMMAND_MOUNT:
            status = st_build_mount_resp((CSI_V0_MOUNT_REQUEST *) reqp,
		                         response_status);
            *size = sizeof(CSI_V0_MOUNT_RESPONSE);
            break;

        case COMMAND_QUERY:
	    status = st_build_query_resp((CSI_V0_QUERY_REQUEST *) reqp,
				         response_status);
            *size = sizeof(CSI_V0_REQUEST_HEADER) +
                    sizeof(RESPONSE_STATUS   ) +
                    sizeof(   TYPE           );
            break;

        case COMMAND_DISMOUNT:
	    status = st_build_dismount_resp((CSI_V0_DISMOUNT_REQUEST *) reqp,
				            response_status);
            *size = sizeof(CSI_V0_DISMOUNT_RESPONSE);
            break;

        case COMMAND_CANCEL:
	    status = st_build_cancel_resp((CSI_V0_CANCEL_REQUEST *) reqp,
				          response_status);
            *size = sizeof(CSI_V0_CANCEL_RESPONSE);
            break;

        case COMMAND_START:
	    status = st_build_start_resp((CSI_V0_START_REQUEST *) reqp,
			                 response_status);
            *size = sizeof(CSI_V0_START_RESPONSE);
            break;

        case COMMAND_IDLE:
	    status = st_build_idle_resp((CSI_V0_IDLE_REQUEST *) reqp,
			                response_status);
            *size = sizeof(CSI_V0_AUDIT_RESPONSE);
            break;


        default:
      /* Need to create a response that has a status of invalid command 
         here */

           status = st_build_error_resp((CSI_V0_REQUEST_HEADER *) reqp,
                                        STATUS_INVALID_COMMAND);
    
            *size = sizeof(CSI_V0_RESPONSE_HEADER);

    } /* end of switch on command */
    return status;
}

STATUS st_build_audit_resp(CSI_V0_AUDIT_REQUEST *reqp,
		    STATUS response_status)
{
    CSI_V0_AUDIT_RESPONSE *arp;

    arp = (CSI_V0_AUDIT_RESPONSE *) malloc(sizeof(CSI_V0_AUDIT_RESPONSE));
    if (NULL == arp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &arp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    arp->message_status.status = response_status;
    memcpy((char *) &arp->cap_id, (char *) &reqp->cap_id, sizeof(V0_CAPID));
    arp->type = reqp->type;
    arp->count = 0;

    memcpy((char *)reqp, (char *)arp, sizeof(CSI_V0_AUDIT_RESPONSE));
    free(arp);

    return STATUS_SUCCESS;
}

STATUS st_build_enter_resp(CSI_V0_ENTER_REQUEST *reqp,
		    STATUS response_status)
{
    CSI_V0_ENTER_RESPONSE *erp;

    erp = (CSI_V0_ENTER_RESPONSE *) malloc(sizeof(CSI_V0_ENTER_RESPONSE));
    if (NULL == erp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &erp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    erp->message_status.status = response_status;
    memcpy((char *) &erp->cap_id, (char *) &reqp->cap_id, sizeof(V0_CAPID));
    erp->count = 0;

    memcpy((char *)reqp, (char *)erp, sizeof(CSI_V0_ENTER_RESPONSE));
    free(erp);

    return STATUS_SUCCESS;

}

STATUS st_build_eject_resp(CSI_V0_EJECT_REQUEST *reqp,
		    STATUS response_status)
{
    CSI_V0_EJECT_RESPONSE *erp;

    erp = (CSI_V0_EJECT_RESPONSE *) malloc(sizeof(CSI_V0_EJECT_RESPONSE));
    if (NULL == erp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &erp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    erp->message_status.status = response_status;
    memcpy((char *) &erp->cap_id, (char *) &reqp->cap_id, sizeof(V0_CAPID));
    erp->count = 0;

    memcpy((char *)reqp, (char *)erp, sizeof(CSI_V0_EJECT_RESPONSE));
    free(erp);

    return STATUS_SUCCESS;

}

STATUS st_build_vary_resp(CSI_V0_VARY_REQUEST *reqp,
		   STATUS response_status)
{

    CSI_V0_VARY_RESPONSE *vrp;

    vrp = (CSI_V0_VARY_RESPONSE *) malloc(sizeof(CSI_V0_VARY_RESPONSE));
    if (NULL == vrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &vrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    vrp->message_status.status = response_status;
    vrp->state = reqp->state;
    vrp->type = reqp->type;
    vrp->count = 0;

    memcpy((char *)reqp, (char *)vrp, sizeof(CSI_V0_VARY_RESPONSE));
    free(vrp);

    return STATUS_SUCCESS;

}

STATUS st_build_cancel_resp(CSI_V0_CANCEL_REQUEST *reqp,
		     STATUS response_status)
{
    
    CSI_V0_CANCEL_RESPONSE *crp;

    crp = (CSI_V0_CANCEL_RESPONSE *) malloc(sizeof(CSI_V0_CANCEL_RESPONSE));
    if (NULL == crp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &crp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    crp->message_status.status = response_status;
    crp->request = reqp->request;

    memcpy((char *)reqp, (char *)crp, sizeof(CSI_V0_CANCEL_RESPONSE));
    free(crp);

    return STATUS_SUCCESS;

}


STATUS st_build_dismount_resp(CSI_V0_DISMOUNT_REQUEST *reqp,
		       STATUS response_status)
{

    CSI_V0_DISMOUNT_RESPONSE *drp;

    drp = (CSI_V0_DISMOUNT_RESPONSE *) malloc(sizeof(CSI_V0_DISMOUNT_RESPONSE));
    if (NULL == drp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &drp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    drp->message_status.status = response_status;
    drp->vol_id = reqp->vol_id;
    drp->drive_id = reqp->drive_id;

    memcpy((char *)reqp, (char *)drp, sizeof(CSI_V0_DISMOUNT_RESPONSE));
    free(drp);

    return STATUS_SUCCESS;

}

STATUS st_build_query_resp( CSI_V0_QUERY_REQUEST *reqp,
                    STATUS response_status)
{

     CSI_V0_QUERY_RESPONSE *qrp;

    int size;
    qrp = (CSI_V0_QUERY_RESPONSE *) malloc(sizeof(CSI_V0_QUERY_RESPONSE));
    if (NULL == qrp) {
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
 
    memcpy((char *) &qrp->csi_request_header,
                        (const char *) &reqp->csi_request_header,
                                             sizeof(CSI_V0_REQUEST_HEADER));
    qrp->message_status.status = response_status;
    qrp->type = reqp->type;
    qrp->count = 0;

    size = sizeof(CSI_V0_REQUEST_HEADER) +
           sizeof(RESPONSE_STATUS   ) +
           sizeof(   TYPE           );
          
    memcpy((char *)reqp, (char *)qrp, size);

    free(qrp);

    return STATUS_SUCCESS;

}


STATUS st_build_idle_resp(CSI_V0_IDLE_REQUEST *reqp,
		   STATUS response_status)
{

    CSI_V0_IDLE_RESPONSE *irp;

    irp = (CSI_V0_IDLE_RESPONSE *) malloc(sizeof(CSI_V0_IDLE_RESPONSE));
    if (NULL == irp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &irp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    irp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)irp, sizeof(CSI_V0_IDLE_RESPONSE));
    free(irp);

    return STATUS_SUCCESS;

}

STATUS st_build_error_resp(CSI_V0_REQUEST_HEADER *reqp,
		   STATUS response_status)
{

    CSI_V0_RESPONSE_HEADER *rp;

    rp = (CSI_V0_RESPONSE_HEADER *) malloc(sizeof(CSI_V0_RESPONSE_HEADER));
    if (NULL == rp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &rp->csi_request_header,
			(const char *) &reqp->csi_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    rp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)rp, sizeof(CSI_V0_RESPONSE_HEADER));
    free(rp);

    return STATUS_SUCCESS;

}

STATUS st_build_mount_resp(CSI_V0_MOUNT_REQUEST *reqp,
		    STATUS response_status)
{

    CSI_V0_MOUNT_RESPONSE *mrp;

    mrp = (CSI_V0_MOUNT_RESPONSE *) malloc(sizeof(CSI_V0_MOUNT_RESPONSE));
    if (NULL == mrp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &mrp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    mrp->message_status.status = response_status;
    mrp->vol_id = reqp->vol_id;
    mrp->drive_id = reqp->drive_id[0];

    memcpy((char *)reqp, (char *)mrp, sizeof(CSI_V0_MOUNT_RESPONSE));
    free(mrp);

    return STATUS_SUCCESS;

}

STATUS st_build_start_resp(CSI_V0_START_REQUEST *reqp,
		   STATUS response_status)
{

    CSI_V0_START_RESPONSE *srp;

    srp = (CSI_V0_START_RESPONSE *) malloc(sizeof(CSI_V0_START_RESPONSE));
    if (NULL == srp) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()",
                 MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    memcpy((char *) &srp->csi_request_header,
			(const char *) &reqp->csi_request_header,
						sizeof(CSI_V0_REQUEST_HEADER));
    srp->message_status.status = response_status;

    memcpy((char *)reqp, (char *)srp, sizeof(CSI_V0_START_RESPONSE));
    free(srp);

    return STATUS_SUCCESS;

}

