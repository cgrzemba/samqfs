#include "acssys.h"
/**********************************************************************
*
*       C Source:               acs_query_ms
*       Subsystem:              acs_query_ms
*       Description:
*       %created_by:    awp %
*       %date_created:  Wed Oct 25 09:41:41 2000 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %full_name:  1/csrc/acs_query_mnt_scratch_pinfo %";
#endif
/*
 *
 *                         (c) Copyright (2002)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_query_mount_scratch_pinfo
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      query_mnt_scratch_pinfo request to the LibraryStation software.
 *      A QUERY TYPE_MOUNT_SCRATCH_ENH request packet is constrtucted
 *      (using the parameters given) and sent to the SSI process.
 *      A query_mnt_scratch_pinfo request retrieves device information
 *      for the pool and management class supplied.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  pool           - Array of ids of tape cartridge pools to be queried.
 *  count          - The number of tape cartridge pools to be queried.
 *  mediaType      - Integer representing the preferred tape cartridge.
 *                   Also, ANY_MEDIA and ALL_MEDIA are valid values.
 *  mgmt_clas      - character string which is the name of a Management
 *                   Class.
 *  virt_aware     - If TRUE indicates a "virtual tape aware" client
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations: Created as a means of passing Management Class in
 *                 a query_mount_scratch request.  Used only with
 *                 LibraryStation and currently only in support for
 *                 virtual tape requests.
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Scott Siao           28-Feb-2002    Original
 */
 
#include <stddef.h>
#include <stdio.h>
 
#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
 
#undef SELF
#define SELF "acs_query_mount_scratch_pinfo"
#undef ACSMOD
#define ACSMOD  53
 
STATUS acs_query_mount_scratch_pinfo
(
    SEQ_NO seqNumber,
    POOL pool[MAX_ID],
    unsigned short pool_count,
    MEDIA_TYPE mediaType,
    MGMT_CLAS mgmt_clas
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    QUERY_REQUEST queryMountScratchRequest;
    QUERY_REQUEST *qMSR;
    QU_MSC_PINFO_CRITERIA *q_mnt_scratch_ptr;
 
    /* just so that we don't have to type or read these long strings */
    qMSR = &queryMountScratchRequest;
    q_mnt_scratch_ptr = &(qMSR->select_criteria.mount_scratch_pinfo_criteria);
 
    acs_trace_entry ();
 
    acsReturn = acs_verify_ssi_running ();
 
    if (acsReturn == STATUS_SUCCESS) {
        acsReturn = acs_build_header ((char *) &queryMountScratchRequest,
            sizeof (QUERY_REQUEST),
            seqNumber,
            COMMAND_QUERY,
            EXTENDED,
	    VERSION_LAST - 1,
            NO_LOCK_ID);
 
        if (acsReturn == STATUS_SUCCESS) {
            queryMountScratchRequest.type = TYPE_MOUNT_SCRATCH_PINFO;
            q_mnt_scratch_ptr->pool_count = pool_count;
 
            /* copy multiple identifiers into the packet */
            if (pool_count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
                acs_error_msg ((&msg_num, pool_count, MAX_ID));
                acsReturn = STATUS_INVALID_VALUE;
            }
            else {
                for (index = 0; index < pool_count; index++) {
                    q_mnt_scratch_ptr->pool_id[index].pool = pool[index];
                }
                q_mnt_scratch_ptr->media_type = mediaType;
                q_mnt_scratch_ptr->mgmt_clas = mgmt_clas;
 
                /*
                if (media_count > MAX_ID) {
                    acs_error_msg ((ACSMSG_BAD_COUNT, media_count, MAX_ID));
                    acsReturn = STATUS_INVALID_VALUE;
                }
                else {
                    for (index = 0; index < media_count; index++) {
                       q_mnt_scratch_ptr->media_type[index] = mediaType[index];
                    }
                }
                */
 
                /* Send the QUERY MOUNT_SCRATCH request to the SSI */
                acsReturn = acs_send_request (&queryMountScratchRequest,
                    sizeof (QUERY_REQUEST));
            }
        }
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}
 
