#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_set_scra/2.2 %";
#endif
/*
 *
 *                           (c) Copyright (1993-2002)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_set_scratch()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      set_scratch request to the ACSSS software.  A SET_SCRATCH
 *      request packet is constructed (using the parameters given) and
 *      sent to the SSI process. A SET_SCRATCH request sets the scratch
 *      attributes for tape cartridges.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber   - A client defined number returned in the response.
 *  lockId      - What id to lock the volumes with, or NO_LOCKID.
 *  pool        - Id of the pool which the volumes will be assigned to or
                  de-assigned from.
 *  volRange    - Array of ranges of cleaning cartridge volumes to use.
 *  on          - If TRUE, volumes will be marked as scratch volumes.
 *  count       - The number of volume ranges in the array.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *                         22-Aug-1990    Original.
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
 *    Scott Siao           19-Apr-2002    Changed ACSMOD from 36 to 35
 */

/* Header Files: */
#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

/* Defines, Typedefs and Structure Definitions: */
#undef SELF
#define SELF  "acs_set_scratch"
#undef ACSMOD
#define ACSMOD 36

/* Global and Static Variable Declarations: */

/* Procedure Type Declarations: */

STATUS acs_set_scratch
(
    SEQ_NO seqNumber,
    LOCKID lockId,
    POOL pool,
    VOLRANGE volRange[MAX_ID],
    BOOLEAN on,
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsRtn;
    SET_SCRATCH_REQUEST set_scratch_req;

    acs_trace_entry ();

    acsRtn = acs_verify_ssi_running ();

    if (acsRtn == STATUS_SUCCESS) {
	acsRtn = acs_build_header ((char *) &set_scratch_req,
	    sizeof (SET_SCRATCH_REQUEST),
	    seqNumber,
	    COMMAND_SET_SCRATCH,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    lockId);
	if (acsRtn == STATUS_SUCCESS) {
	    if (!on) {
		set_scratch_req.request_header.message_header.extended_options
		    = RESET;
	    }
	    set_scratch_req.pool_id.pool = pool;
	    set_scratch_req.count = count;
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsRtn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (index = 0; index < count; index++) {
		    set_scratch_req.vol_range[index] = volRange[index];
		}
		acsRtn = acs_send_request (&set_scratch_req,
		    sizeof (SET_SCRATCH_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsRtn);
    return acsRtn;
}

