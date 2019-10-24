#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_venter/2.01A %";
#endif
/*
 *
 *                           (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_venter()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate
 *      a venter request to the ACSSS software.  A VENTER request packet
 *      is constructed (using the parameters given) and sent to the SSI
 *      process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber    - A client defined number returned in the response.
 *  capId        - The id of the CAP where the volumes will be entered.
 *  count        - The number of volumes in the array.
 *  volId        - Array of ids of the volumes to be entered.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:       22_Aug-1990    Original.
 *    David A. Myers       21-Nov-1991    Version 3.
 *    Scott Siao           19-Oct-1992    Changed bzeros to memsets.
 *    Emanuel Alongi       07-Aug-1992    Assign global packet version
 *                                        number.
 *    Howard Freeman IV    30-Sep-1992    Conformance to R3.0 PFS
 *                                        <count>.
 *    Ken Stickney         04_Jun_1993    ANSI version from RMLS/400
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
#define SELF "acs_venter"
#undef ACSMOD
#define ACSMOD 43

STATUS acs_venter
(
    SEQ_NO seqNumber,
    CAPID capId,
    unsigned short count,
    VOLID volId[MAX_ID]
) 
{
    COPYRIGHT;
    STATUS acsReturn;
    ACSMESSAGES msg_num;
    VENTER_REQUEST venterRequest;
    unsigned int i;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();
    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &venterRequest,
	    sizeof (VENTER_REQUEST),
	    seqNumber,
	    COMMAND_ENTER,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);
	if (acsReturn == STATUS_SUCCESS) {
	    venterRequest.request_header.message_header.extended_options
		|= VIRTUAL;
	    venterRequest.cap_id = capId;
	    venterRequest.count = count;

	    /*     copy multiple identifiers into the packet */
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsReturn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (i = 0; i < count; i++) {
		    venterRequest.vol_id[i] = volId[i];
		}
		acsReturn = acs_send_request (&venterRequest, sizeof (VENTER_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsReturn);

    return acsReturn;
}

