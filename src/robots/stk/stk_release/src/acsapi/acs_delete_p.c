#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_delete_p/2.01A %";
#endif
/*
 *
 *                            (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_delete_pool()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      delete_pool request to the ACSSS software.  A DELETE_POOL
 *      request packet is constructed (using the parameters given) and
 *      sent to the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber      - A client defined number returned in the response.
 *  pool[MAX_ID]   - An array of ids of pools to be locked.
 *  count          - The number of ids in the pool id array.
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
#define SELF "acs_delete_pool"
#undef ACSMOD
#define ACSMOD 8

STATUS acs_delete_pool
(
    SEQ_NO seqNumber,
    POOL pool[MAX_ID],
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    DELETE_POOL_REQUEST deletePoolRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &deletePoolRequest,
	    sizeof (DELETE_POOL_REQUEST),
	    seqNumber,
	    COMMAND_DELETE_POOL,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);

	if (acsReturn == STATUS_SUCCESS) {
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsReturn = STATUS_INVALID_VALUE;
	    }
	    else if (count) {
		for (index = 0; index < count; index++) {
		    deletePoolRequest.pool_id[index].pool = pool[index];
		}
		deletePoolRequest.count = count;
	    }
	    else {
		deletePoolRequest.pool_id[0].pool = (POOL) 0;
		deletePoolRequest.count = 0;
	    }

	    acsReturn = acs_send_request (&deletePoolRequest,
		sizeof (DELETE_POOL_REQUEST));
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

