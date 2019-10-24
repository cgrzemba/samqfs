#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_query_ms/2.01A %";
#endif
/*
 *
 *                            (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_query_mount_scratch()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      query_mount_scratch request to the ACSSS software.
 *      A QUERY MOUNT_SCRATCH request packet is constructed (using the
 *      parameters given) and sent to the SSI process.
 *      A query_mount_scratch request retrieves mount information for
 *      the pools supplied.
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
 */

#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

#undef SELF
#define SELF "acs_query_mount_scratch"
#undef ACSMOD
#define ACSMOD  25

STATUS acs_query_mount_scratch
(
    SEQ_NO seqNumber,
    POOL pool[MAX_ID],
    unsigned short pool_count,
    MEDIA_TYPE mediaType
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    QUERY_REQUEST queryMountScratchRequest;
    QUERY_REQUEST *qMSR;
    QU_MSC_CRITERIA *q_mnt_scratch_ptr;

    /* just so that we don't have to type or read these long strings */
    qMSR = &queryMountScratchRequest;
    q_mnt_scratch_ptr = &(qMSR->select_criteria.mount_scratch_criteria);

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
        acsReturn = acs_build_header ((char *) &queryMountScratchRequest,
            sizeof (QUERY_REQUEST),
            seqNumber,
            COMMAND_QUERY,
            EXTENDED,
	    VERSION_LAST - (VERSION)1,
            NO_LOCK_ID);

        if (acsReturn == STATUS_SUCCESS) {
            queryMountScratchRequest.type = TYPE_MOUNT_SCRATCH;
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

