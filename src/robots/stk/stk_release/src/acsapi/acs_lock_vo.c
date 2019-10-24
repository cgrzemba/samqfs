#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_lock_vo/2.01A %";
#endif
/*
 *
 *                           (c)  Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_lock_volume()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      lock request to the ACSSS software.  A LOCK VOLUME request
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
 *  lockId        - Id of the lock to apply to cartridges.
 *  userId        - User identifier associated with the lock.
 *  volId          - Array of ids of tape cartridge volumes to lock.
 *  wait           - Dismount even if the volume is in use in the drive.
 *  count          - The number of ids in the panel id array.
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
#define SELF "acs_lock_volume"
#undef ACSMOD
#define ACSMOD 14

STATUS acs_lock_volume
(
    SEQ_NO seqNumber,
    LOCKID lockId,
    USERID userId,
    VOLID volId[MAX_ID],
    BOOLEAN wait,
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    LOCK_REQUEST lockRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &lockRequest,
	    sizeof (LOCK_REQUEST),
	    seqNumber,
	    COMMAND_LOCK,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    lockId);

	if (acsReturn == STATUS_SUCCESS) {
	    lockRequest.type = TYPE_VOLUME;
	    lockRequest.request_header.message_header.access_id.user_id
		= userId;
	    lockRequest.request_header.message_header.extended_options
		|= (wait ? WAIT : 0);
	    lockRequest.count = count;
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsReturn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (index = 0; index < count; index++) {
		    lockRequest.identifier.vol_id[index] = volId[index];
		}
		acsReturn = acs_send_request (&lockRequest, sizeof (LOCK_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

