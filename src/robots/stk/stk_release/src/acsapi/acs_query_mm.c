#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_query_mm/2.2 %";
#endif
/*
 *
 *                           (c) Copyright (1993-2002)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_query_mm_info()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      query_mixed_media_info request to the ACSSS software.
 *      A QUERY_MIXED_MEDIA_INFO request packet is constructed (using the
 *      parameters given) and sent to the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber    - A client defined number returned in the response.
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
 *    Ken Stickney         08-AUG-1993    Original
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened 
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler, 
 *                                        added defines ACSMOD and SELF for 
 *                                        trace and error messages, fixed  
 *                                        copyright.
 *    Ken Stickney         06-May-1994    Replaced acs_ipc_write with new
 *                                        function acs_send_request for 
 *                                        down level server support.
 *    Scott Siao           19-Apr-2002    Added ACSMOD value of 45 for debug trace.
 *                                        and cleaned up definition of SELF
 */

#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

#undef SELF
#define SELF "acs_query_mixed_media_info"
#undef ACSMOD
#define ACSMOD 45

STATUS acs_query_mm_info 
(
    SEQ_NO seqNumber
)
{
    STATUS acsReturn;
    QUERY_REQUEST queryMixedMediaInfoRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &queryMixedMediaInfoRequest,
	    sizeof (QUERY_REQUEST),
	    seqNumber,
	    COMMAND_QUERY,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);

	if (acsReturn == STATUS_SUCCESS) {
	    queryMixedMediaInfoRequest.type = TYPE_MIXED_MEDIA_INFO;

	    /* Send the QUERY MIXED_MEDIA_INFO request to the SSI */
	    acsReturn = acs_send_request (&queryMixedMediaInfoRequest,
		sizeof (QUERY_REQUEST));

	}
    }
    acs_trace_exit (acsReturn);

    return acsReturn;
}

