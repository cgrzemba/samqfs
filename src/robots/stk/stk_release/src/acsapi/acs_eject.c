#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_eject/2.01A %";
#endif
/*
 *
 *                            (c) Copyright (C) (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_eject()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      eject request to the ACSSS software.  A EJECT request packet
 *      is constructed (using the parameters given) and sent to the SSI
 *      process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  lockId         - Id of the lock on the cartridge volume.
 *  capId          - The id of the CAP into which the volumes are ejected.
 *  count          - The number of volume to be rejected.
 *  volumes        - Pointer to an array of ids of volumes to be rejected.
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
#define SELF "acs_eject"
#undef ACSMOD
#define ACSMOD 10

STATUS acs_eject
(
    SEQ_NO seqNumber,
    LOCKID lockId,
    CAPID capId,
    unsigned short count,
    VOLID   volumes[MAX_ID]
) 
{
    COPYRIGHT;
    STATUS acsReturn;
    ACSMESSAGES msg_num;
    EJECT_REQUEST ejectRequest;
    VOLID *vp;
    unsigned int i;

    acs_trace_entry ();

    if (count > MAX_ID) {
	/*sprintf (msg, "%acsReturn: count:%d must"
                    "be less or equal to %d\n",
               SELF, count, MAX_ID); */
        msg_num = ACSMSG_BAD_COUNT;
	acs_error_msg ((&msg_num, count, MAX_ID));
	acsReturn = STATUS_INVALID_VALUE;
	acs_trace_exit (acsReturn);
	return acsReturn;
    }
    acsReturn = acs_verify_ssi_running ();
    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &ejectRequest,
	    sizeof (EJECT_REQUEST),
	    seqNumber,
	    COMMAND_EJECT,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    lockId);

	if (acsReturn == STATUS_SUCCESS) {
	    ejectRequest.cap_id = capId;
	    ejectRequest.count = count;
	    vp = &volumes[0];
	    for (i = 0; i < ejectRequest.count; i++, vp++) {
		ejectRequest.vol_id[i] = *vp;
	    }
	    acsReturn = acs_send_request (&ejectRequest, sizeof (EJECT_REQUEST));
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

