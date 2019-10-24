#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_query_pl/2.01A %";
#endif
/*
 *
 *                          (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_query_pool()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      query_pool request to the ACSSS software.  A QUERY POOL request
 *      packet is constructed (using the parameters given) and sent to
 *      the SSI process. A QUERY POOL request retrieves status inform-
 *      ation for each pool specified.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  pool           - Array of ids of tape cartridge pools to be queried.
 *  count          - The number of tape cartridge pools to be queried.
 *                   If count = 0, all tape cartridge pools are queried.
 *
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
 *    Jim Montgomery       22-Aug 1990    Original.
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
#define SELF "acs_query_pool"
#undef ACSMOD
#define ACSMOD 26

STATUS acs_query_pool
(
    SEQ_NO seqNumber,
    POOL pool[MAX_ID],
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short i;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    QUERY_REQUEST queryPoolRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &queryPoolRequest,
	    sizeof (QUERY_REQUEST),
	    seqNumber,
	    COMMAND_QUERY,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);

	if (acsReturn == STATUS_SUCCESS) {
	    queryPoolRequest.type = TYPE_POOL;
	    queryPoolRequest.select_criteria.pool_criteria.pool_count =
		count;
	    /* copy multiple identifiers into the packet */
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsReturn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (i = 0; i < count; i++) {
		    queryPoolRequest.select_criteria.pool_criteria.
			pool_id[i].pool = pool[i];
		}
		acsReturn = acs_send_request (&queryPoolRequest, sizeof (QUERY_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

