#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_unlock_dr/2.01A %";
#endif
/*
 *
 *                           (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_unlock_drive()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      unlock request to the ACSSS software.  A UNLOCK DRIVE request
 *      packet is constructed (using the parameters given) and sent to
 *      the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber    - A client defined number returned in the response.
 *  lockId       - The id of the lock on the drives specified.
 *  driveId      - Array of ids of drives to unlock.
 *  count        - The number of drives in the array.
 *
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
 * Revision History:       22-Aug-1990    Original.
 *    Scott Siaomery       19-Oct-1992    Changed bzeros to memsets.
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
#define SELF  "acs_unlock_drive"
#undef ACSMOD
#define ACSMOD 37

/* Global and Static Variable Declarations: */

/* Procedure Type Declarations: */

STATUS acs_unlock_drive
(
    SEQ_NO seqNumber,
    LOCKID lockId,
    DRIVEID driveId[MAX_ID],
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsRtn;
    UNLOCK_REQUEST unlock_req;

    acs_trace_entry ();

    acsRtn = acs_verify_ssi_running ();

    if (acsRtn == STATUS_SUCCESS) {
	acsRtn = acs_build_header ((char *) &unlock_req,
	    sizeof (UNLOCK_REQUEST),
	    seqNumber,
	    COMMAND_UNLOCK,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    lockId);
	if (acsRtn == STATUS_SUCCESS) {
	    unlock_req.type = TYPE_DRIVE;
	    unlock_req.count = count;
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsRtn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (index = 0; index < count; index++) {
		    unlock_req.identifier.drive_id[index] = driveId[index];
		}
		acsRtn = acs_send_request (&unlock_req, sizeof (UNLOCK_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsRtn);
    return acsRtn;
}

