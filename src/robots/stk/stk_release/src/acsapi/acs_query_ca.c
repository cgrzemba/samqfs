#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_query_ca/2.01A %";
#endif
/*
 *
 *                         (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_query_cap()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      query_cap request to the ACSSS software.  A QUERY CAP request
 *      packet is constructed (using the parameters given) and sent to
 *      the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  capId          - Array of ids of CAPs to be queried.
 *  count          - The number of CAPs to be queried.
 *                   If count = 0, all CAPs are queried.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:       10-May-1989    Original.
 *    Jim Montgomery       23-Jul-1990    Version 2.
 *    Howard Freeman       08-Sep-1991    Version 3.
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
#define SELF "acs_query_cap"
#undef ACSMOD
#define ACSMOD 23

STATUS acs_query_cap
(
    SEQ_NO seqNumber,
    CAPID capId[MAX_ID],
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short i;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    QUERY_REQUEST queryCAPRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &queryCAPRequest,
	    sizeof (QUERY_REQUEST),
	    seqNumber,
	    COMMAND_QUERY,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);

	if (acsReturn == STATUS_SUCCESS) {
	    queryCAPRequest.type = TYPE_CAP;
	    queryCAPRequest.select_criteria.cap_criteria.cap_count =
		count;
	    /* copy multiple identifiers into the packet */
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsReturn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (i = 0; i < count; i++) {
		    queryCAPRequest.select_criteria.cap_criteria.cap_id[i] =
			capId[i];
		}
		acsReturn = acs_send_request (&queryCAPRequest,
		    sizeof (QUERY_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

