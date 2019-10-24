#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_mount_sc/2.0A %";
#endif
/*
 *
 *                            (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_mount_scratch()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      mount_scratch request to the ACSSS software.  A MOUNT_SCRATCH
 *      request packet is constructed (using the parameters given) and
 *      sent to the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE.
 *
 * Parameters:
 *
 *  seqNumber     - A client defined number returned in the response.
 *  lockId        - Lock the drive with this id or NO_LOCK.
 *  pool          - Pool from which to get the scratch tape.
 *  driveId       - Id of the drive where the tape cartridge is mounted.
 *  mtype         - A numerical value corresponding to a tape cartridge
 *                   media type, or ANY_MEDIA_TYPE, or ALL_MEDIA_TYPE.
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
#define SELF "acs_mount_scratch"
#undef ACSMOD
#define ACSMOD 16

STATUS acs_mount_scratch
(
    SEQ_NO seqNumber,
    LOCKID lockId,
    POOL pool,
    DRIVEID driveId,
    MEDIA_TYPE mtype
) 
{
    COPYRIGHT;
    STATUS acsReturn;
    MOUNT_SCRATCH_REQUEST mountScratchRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &mountScratchRequest,
	    sizeof (MOUNT_SCRATCH_REQUEST),
	    seqNumber,
	    COMMAND_MOUNT_SCRATCH,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    lockId);

	if (acsReturn == STATUS_SUCCESS) {
	    mountScratchRequest.pool_id.pool = pool;
	    mountScratchRequest.count = 1;
	    mountScratchRequest.media_type = mtype;
	    mountScratchRequest.drive_id[0] = driveId;
	    acsReturn = acs_send_request (&mountScratchRequest,
		sizeof (MOUNT_SCRATCH_REQUEST));
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

