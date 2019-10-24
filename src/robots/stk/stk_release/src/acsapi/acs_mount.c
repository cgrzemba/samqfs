#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_mount/2.0A %";
#endif
/*
 *
 *                            (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_mount()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      mount request to the ACSSS software.  A MOUNT request packet
 *      is constructed (using the parameters given) and sent to the SSI
 *      process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE.
 *
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  lockId        - Lock the drive with this id or NO_LOCK. 
 *  volId         - Id of the tape cartridge to be mounted.
 *  driveId       - Id of the drive where the tape cartridge is mounted.
 *  readonly       - If TRUE, the volume will be mounted readonly.
 *  bypass         - If TRUE, bypass volser and media verification.
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
#include <string.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

#undef SELF
#define SELF "acs_mount"
#undef ACSMOD
#define ACSMOD 15

STATUS acs_mount
(
    SEQ_NO seqNumber,
    LOCKID lockId,
    VOLID volId,
    DRIVEID driveId,
    BOOLEAN readonly,
    BOOLEAN bypass
) 
{
    COPYRIGHT;
    STATUS acsReturn;
    MOUNT_REQUEST mountRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &mountRequest,
	    sizeof (MOUNT_REQUEST),
	    seqNumber,
	    COMMAND_MOUNT,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    lockId);

	if (acsReturn == STATUS_SUCCESS) {
	    if (readonly) {
		mountRequest.request_header.message_header.message_options
		    |= READONLY;
	    }
	    if (bypass) {
		mountRequest.request_header.message_header.message_options
		    |= BYPASS;
	    }
	    strncpy (mountRequest.vol_id.external_label, " ",
		EXTERNAL_LABEL_SIZE + 1);
	    mountRequest.vol_id = volId;
	    mountRequest.count = 1;
	    mountRequest.drive_id[0] = driveId;
	    acsReturn = acs_send_request (&mountRequest, sizeof (MOUNT_REQUEST));
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

