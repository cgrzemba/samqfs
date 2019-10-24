#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_query_lv/2.01A %";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      acs_query_lock_volume()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      query_lock_volume request to the ACSSS software.  A QUERY_LOCK
 *      request packet is constructed (using the parameters given) and
 *      sent to the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  volId          - Array of ids of volumes to be queried.
 *  lockId         - The id of the lock associated with the volumes.
 *  count          - Number of volumes to be queried.
 *                   If count = 0, all volumes with the given lock id
 *                   are queried.
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
 *    Jim Montgomery       22-Aug-1990    Original.
 *    David A. Myers       21-Nov-1991    Version 3.
 *    Scott Siao           19-Oct-1992    Changed bzeros to memsets.
 *    Emanuel Alongi       07-Aug-1992    Assign global packet version
 *                                        number.
 *    Howard Freeman IV    30-Sep-1992    Conformance to R3.0 PFS
 *                                        <count>.
 *    Ken Stickney         04-Jun-1993    ANSI version from RMLS/400
 *                                        (Tom Rethard).
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened 
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler, 
 *                                        added defines ACSMOD and SELF for 
 *                                        trace and error messages, fixed  
 *                                        copyright.
 *    Ken Stickney         06-May-1994    Replaced acs_ipc_write with new
 *                                        function acs_send_request for 
 *                                        down level server support.
 *    Joseph Nofi          15-Sep-2011    Allow non-0 lockid to be input 
 *                                        when volume count > 0.
*/

#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

#undef SELF
#define SELF "acs_query_lock_volume"
#undef ACSMOD
#define ACSMOD 22

STATUS acs_query_lock_volume
(
    SEQ_NO seqNumber,
    VOLID volId[MAX_ID],
    LOCKID lockId,
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    QUERY_LOCK_REQUEST queryLockRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
    acsReturn = acs_build_header ((char *) &queryLockRequest,
        sizeof (QUERY_LOCK_REQUEST),
        seqNumber,
        COMMAND_QUERY_LOCK,
        EXTENDED,
        VERSION_LAST - (VERSION)1,
        lockId);

    if (acsReturn == STATUS_SUCCESS) {
        queryLockRequest.type = TYPE_VOLUME;
        queryLockRequest.count = count;

        /* copy multiple identifiers into the packet */
        if (count > MAX_ID) {
        msg_num = ACSMSG_BAD_COUNT;
        acs_error_msg ((&msg_num, count, MAX_ID));
        acsReturn = STATUS_INVALID_VALUE;
        }
        else {
        for (index = 0; index < count; index++) {
            queryLockRequest.identifier.vol_id[index] = volId[index];
        }
        acsReturn = acs_send_request (&queryLockRequest,
            sizeof (QUERY_REQUEST));
        }
    }
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

