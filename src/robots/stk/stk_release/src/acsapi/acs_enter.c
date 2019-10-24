#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_enter/2.0A %";
#endif
/*
 *
 *                             (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_enter()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      enter request to the ACSSS software.  A ENTER request packet
 *      is constructed (using the parameters given) and sent to the SSI
 *      process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE.
 *   
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  capId         - Id of the CAP where the drives will be entered.
 *  continuous     - If TRUE, CAP will be unlocked for the entry of more
 *                   cartridges.
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
 *    Jim Montgomery       10-May-1989    Original.
 *    Jim Montgomery       23-Jul-1990    Version 2.
 *    David A. Myers       21-Nov-1991    Version 3.
 *    Scott Siao           19-Oct-1992    Changed bzeros to memsets.
 *    Emanuel Alongi       07-Aug-1992    Assign global packet version
 *                                        number.
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

#undef SELF
#define SELF  "acs_enter"
#undef ACSMOD
#define ACSMOD 11

STATUS acs_enter
(
    SEQ_NO seqNumber,
    CAPID capId,
    BOOLEAN continuous
) 
{
    COPYRIGHT;
    STATUS acsRtn;
    ENTER_REQUEST enterRequest;

    acs_trace_entry ();

    acsRtn = acs_verify_ssi_running ();

    if (acsRtn == STATUS_SUCCESS) {
	acsRtn = acs_build_header ((char *) &enterRequest,
	    sizeof (ENTER_REQUEST),
	    seqNumber,
	    COMMAND_ENTER,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);
	if (acsRtn == STATUS_SUCCESS) {
	    if (continuous) {
		enterRequest.request_header.message_header.extended_options
		    = CONTINUOUS;
	    }
	    enterRequest.cap_id = capId;
	    acsRtn = acs_send_request (&enterRequest, sizeof (ENTER_REQUEST));
	    if (acsRtn != STATUS_SUCCESS) {
		acsRtn = STATUS_IPC_FAILURE;
	    }
	}
    }
    acs_trace_exit (acsRtn);
    return acsRtn;
}

