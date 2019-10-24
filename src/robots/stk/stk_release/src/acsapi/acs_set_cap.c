#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_set_cap/2.01A %";
#endif
/*
 *
 *                           (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_set_cap()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      set_cap request to the ACSSS software.  A SET_CAP request packet
 *      is constructed (using the parameters given) and sent to the SSI
 *      process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  cap_priority   - Priority for all CAPs specified.
 *  cap_mode       - The mode for all CAPs specified.
 *  capId[MAX_ID]  - Array of ids of CAPs to be modified
 *  count          - The number of ids in the array of CAPs.
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
 *    Jim Montgomery       22-Aug-1990     Original.
 *    Howard Freeman       8-Sep-1991      Updated for R3.0
 *    David A. Myers       19-Oct-1992     Changed bzeros to memsets.
 *    Emanuel Alongi       07-Aug-1992     Assign global packet version
 *                                         number.
 *    Howard Freeman IV    30-Sep-1992     Conformance to R3.0 PFS
 *                                         <count>.
 *    Ken Stickney         04-Jun-1993     ANSI version from RMLS/400
 *                                         (Tom Rethard).
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

/* Header Files: */
#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

/* Defines, Typedefs and Structure Definitions: */
#undef SELF
#define SELF  "acs_set_cap"
#undef ACSMOD
#define ACSMOD 33

/* Global and Static Variable Declarations: */

/* Procedure Type Declarations: */

STATUS acs_set_cap
(
    SEQ_NO seqNumber,
    CAP_PRIORITY capPriority,
    CAP_MODE capMode,
    CAPID capId[MAX_ID],
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsRtn;
    SET_CAP_REQUEST set_cap_req;

    acs_trace_entry ();

    acsRtn = acs_verify_ssi_running ();

    if (acsRtn == STATUS_SUCCESS) {
	acsRtn = acs_build_header ((char *) &set_cap_req,
	    sizeof (SET_CAP_REQUEST),
	    seqNumber,
	    COMMAND_SET_CAP,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID
	    );

	if (acsRtn == STATUS_SUCCESS) {
	    set_cap_req.cap_priority = capPriority;
	    set_cap_req.cap_mode = capMode;
	    set_cap_req.count = count;
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsRtn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (index = 0; index < count; index++) {
		    set_cap_req.cap_id[index] = capId[index];
		}
		acsRtn = acs_send_request (&set_cap_req, sizeof (SET_CAP_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsRtn);
    return acsRtn;
}

