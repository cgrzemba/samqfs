static char CCM_Id[]="@(#) %full_name: 1/csrc/acs_query_subpool_name.c/janeen1 % %release: CSC400 % %date_modified: Thu Sep  3 11:09:24 1998 % (c) 1998 StorageTek";
#include "acssys.h"
#ifndef lint
static char *_csrc = "@(#) %full_name:  1/csrc/acs_query_subpool_name.c/janeen1 %";
#endif
/****************************************************************************
 *
 *                        (C) Copyright (2002)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_query_subpool_name()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      query_subpool_name request to the ACSSS software.  A QUERY REQUEST     
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
 *  count          - The number of subpool names to be queried.
 *                   If count = 0, all subpool names are queried.
 *  SUBPOOL_NAME   - Array of subpool names to be queried.
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
 *     J. E. Jahn          08-Jul-1998    Original.        E141
 *     S. L. Siao          19-Apr-2002    Changed ACSMOD from 56 to 57
 *     S. L. Siao          26-Apr-2002    Changed querySubpoolRequest to
 *                                        querySubpoolNameRequest.
 ***************************************************************************/
 
#include <stddef.h>
#include <stdio.h>
 
#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
 
#undef SELF
#define SELF "acs_query_subpool_name"
#undef ACSMOD
#define ACSMOD 57
 
STATUS acs_query_subpool_name
(
    SEQ_NO seqNumber,
    unsigned short count,
    SUBPOOL_NAME subpoolName[MAX_SPN]
) 
{
    COPYRIGHT;
    STATUS acsReturn;
    unsigned short i;
    ACSMESSAGES msg_num;
    QUERY_REQUEST querySubpoolNameRequest;
 
    acs_trace_entry ();
 
    acsReturn = acs_verify_ssi_running ();
 
    if (acsReturn == STATUS_SUCCESS) {
        acsReturn = acs_build_header ((char *) &querySubpoolNameRequest,
            sizeof (QUERY_REQUEST),
            seqNumber,
            COMMAND_QUERY,
            EXTENDED,
            VERSION_LAST - 1,
            NO_LOCK_ID);
 
        if (acsReturn == STATUS_SUCCESS) {
            querySubpoolNameRequest.type = TYPE_SUBPOOL_NAME;
            querySubpoolNameRequest.select_criteria.subpl_name_criteria.spn_count
                = count;
            if (count > MAX_SPN) {
                msg_num = ACSMSG_BAD_COUNT;
                acs_error_msg ((&msg_num, count, MAX_SPN));
                acsReturn = STATUS_INVALID_VALUE;
            }
            else {
 
                for (i = 0; i < count; i++) {
                    querySubpoolNameRequest.select_criteria.subpl_name_criteria.
                        subpl_name[i] = subpoolName[i];
                }
                acsReturn = acs_send_request (&querySubpoolNameRequest,
                    sizeof (QUERY_REQUEST));
            }
        }
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}
 
