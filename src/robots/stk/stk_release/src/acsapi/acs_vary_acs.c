#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_vary_acs/2.01A %";
#endif
/*
 *
 *                           (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_vary_acs()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate
 *      a vary_acs request to the ACSSS software.  A VARY ACS request
 *      packet is constructed (using the parameters given) and sent to
 *      the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 * Parameters:
 *
 *  seqNumber    - A client defined number returned in the response.
 *  acs          - Array of ids of ACSs to be operated on.
 *  state        - STATE_DIAGNOSTIC, STATE_OFFLINE, and STATE_ONLINE.
 *  force        - If state not STATE_OFFLINE, force is ignored.
 *                 If state STATE_OFFLINE, TRUE means abort all current
 *                 and pending requests.
 *  count        - The number of ACSs in the array.
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

/* Header Files: */
#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

/* Defines, Typedefs and Structure Definitions: */
#undef SELF
#define SELF  "acs_vary_acs"
#undef ACSMOD
#define ACSMOD 39

/* Global and Static Variable Declarations: */

/* Procedure Type Declarations: */

STATUS acs_vary_acs
(
    SEQ_NO seqNumber,
    ACS acs[MAX_ID],
    STATE state,
    BOOLEAN force,
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsRtn;
    VARY_REQUEST vary_acs_req;

    acs_trace_entry ();

    acsRtn = acs_verify_ssi_running ();
    if (acsRtn == STATUS_SUCCESS) {
	acsRtn = acs_build_header ((char *) &vary_acs_req,
	    sizeof (VARY_REQUEST),
	    seqNumber,
	    COMMAND_VARY,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);
	if (acsRtn == STATUS_SUCCESS) {
	    if (force) {
		vary_acs_req.request_header.message_header.message_options
		    |= FORCE;
	    }
	    vary_acs_req.type = TYPE_ACS;
	    vary_acs_req.state = state;
	    vary_acs_req.count = count;

	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsRtn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (index = 0; index < count; index++) {
		    vary_acs_req.identifier.acs_id[index] = acs[index];
		}
		acsRtn = acs_send_request (&vary_acs_req, sizeof (VARY_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsRtn);
    return acsRtn;
}

