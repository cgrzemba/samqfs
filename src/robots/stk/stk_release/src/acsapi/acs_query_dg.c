static char CCM_Id[]="@(#) %full_name: % %release: % %date_modified: % (c) 1998 StorageTek";
#include "acssys.h"
#ifndef lint
static char *_csrc = "@(#) %full_name:  acsapi/csrc/acs_query_/2.2 %";
#endif
/****************************************************************************
 *
 *                           (C) Copyright (2002)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_query_drive_group()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      query_drive_group request to the ACSSS software.  A QUERY REQUEST     
 *      request packet is constructed (using the parameters given) and
 *      sent to the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE, or STATUS_PROCESS_FAILURE
 *
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  grouptype      - Enum value indicating what type of drive group information
 *                   is being requested.
 *                   For NCS 4.0, VTSS will be the only drive group supported.
 *  count          - The number of GROUPIDs (VTSS names) to be queried.
 *                   If count = 0, all GROUPIDs (VTSS names are queried.
 *  groupid[]      - Array of GROUPIDs (VTSS names) to be queried.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *     J. E. Jahn          28-Jul-1998    Original.        E59
 *     S. L. Siao          26-Feb-2002    Fixed group_type, it was not being
 *                                        passed.
 *     S. L. Siao          19-Apr-2002    Changed ACSMOD from 55 to 56
 *     S. L. Siao          26-Apr-2002    Changed queryDriveRequest to 
 *                                        queryDriveGroupRequest.
 ***************************************************************************/
 
#include <stddef.h>
#include <stdio.h>
 
#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
 
#undef SELF
#define SELF "acs_query_drive_group"
#undef ACSMOD
#define ACSMOD 56
 
STATUS acs_query_drive_group
(
    SEQ_NO seqNumber,
    GROUP_TYPE groupType,
    unsigned short count,
    GROUPID groupid[MAX_DRG]
) 
{
    COPYRIGHT;
    STATUS acsReturn;
    unsigned short i;
    ACSMESSAGES msg_num;
    QUERY_REQUEST queryDriveGroupRequest;
 
    acs_trace_entry ();
 
    acsReturn = acs_verify_ssi_running ();
 
    if (acsReturn == STATUS_SUCCESS) {
        acsReturn = acs_build_header ((char *) &queryDriveGroupRequest,
            sizeof (QUERY_REQUEST),
            seqNumber,
            COMMAND_QUERY,
            EXTENDED,
            VERSION_LAST - 1,
            NO_LOCK_ID);
 
        if (acsReturn == STATUS_SUCCESS) {
            queryDriveGroupRequest.type = TYPE_DRIVE_GROUP;
            queryDriveGroupRequest.select_criteria.drive_group_criteria.group_type
		= groupType;
            queryDriveGroupRequest.select_criteria.drive_group_criteria.drg_count
                = count;
            if (count > MAX_DRG) {
                msg_num = ACSMSG_BAD_COUNT;
                acs_error_msg ((&msg_num, count, MAX_DRG));
                acsReturn = STATUS_INVALID_VALUE;
            }
            else {
 
                for (i = 0; i < count; i++) {
                    queryDriveGroupRequest.select_criteria.drive_group_criteria.
                        group_id[i] = groupid[i];
                }
                acsReturn = acs_send_request (&queryDriveGroupRequest,
                    sizeof (QUERY_REQUEST));
            }
        }
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}
 
