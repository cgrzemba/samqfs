#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_define_p/2.01A %";
#endif
/*
 *
 *                             (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_define_pool()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      define_pool request to the ACSSS software.  A DEFINE_POOL
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
 *  lwm            - The low water mark for the pool.
 *  hwm            - The high water mark for the pool.
 *  attributes     - pool attributes (only OVERFLOW applies).
 *  pool           - the array of ids of pools to define.
 *  count          - The number of ids in the drive id array.
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
#define SELF "acs_define_pool"
#undef ACSMOD
#define ACSMOD 7

STATUS acs_define_pool
(
    SEQ_NO seqNumber,
    unsigned long lwm,		/* low water mark */
    unsigned long hwm,		/* high water mark */
    unsigned long attributes,	/* pool attributes */
    POOL pool[MAX_ID],
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    DEFINE_POOL_REQUEST definePoolRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &definePoolRequest,
	    sizeof (DEFINE_POOL_REQUEST),
	    seqNumber,
	    COMMAND_DEFINE_POOL,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);

	if (acsReturn == STATUS_SUCCESS) {
	    definePoolRequest.low_water_mark = lwm;
	    definePoolRequest.high_water_mark = hwm;
	    definePoolRequest.pool_attributes = attributes;
	    definePoolRequest.count = count;

	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsReturn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (index = 0; index < count; index++) {
		    definePoolRequest.pool_id[index].pool = pool[index];
		}
		acsReturn = acs_send_request (&definePoolRequest,
		    sizeof (DEFINE_POOL_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

