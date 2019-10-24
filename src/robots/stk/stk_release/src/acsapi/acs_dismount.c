#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_dismount/2.0A %";
#endif
/*
 *
 *                            (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_dismount()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      dismount request to the ACSSS software.  A DISMOUNT request
 *      packet is constructed (using the parameters given) and sent to
 *      the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE.
 *
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  LOCKID lockId  - Id of the lock on the drive or cartridge. 
 *  VOLID volId    - External label of cartridge to be dismounted.
 *  DRIVEID driveId - The library drive containing the tape cartridge.
 *  BOOLEAN force - Dismount even if the volume is in use in the drive.
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
#define SELF "acs_dismount"
#undef ACSMOD
#define ACSMOD 9

STATUS acs_dismount
(
    SEQ_NO seqNumber,
    LOCKID lockId,
    VOLID volId,
    DRIVEID driveId,
    BOOLEAN force
) 
{
    COPYRIGHT;
    STATUS acsReturn;
    DISMOUNT_REQUEST dismountRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &dismountRequest,
	    sizeof (DISMOUNT_REQUEST),
	    seqNumber,
	    COMMAND_DISMOUNT,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    lockId);

	if (acsReturn == STATUS_SUCCESS) {
	    if (force) {
		dismountRequest.request_header.message_header.message_options
		    |= FORCE;
	    }
	    dismountRequest.vol_id = volId;
	    dismountRequest.drive_id = driveId;
	    /* Send the DISMOUNT request to the SSI */
	    acsReturn = acs_send_request (&dismountRequest,
		sizeof (DISMOUNT_REQUEST));
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

