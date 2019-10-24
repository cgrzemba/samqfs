#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_audit_se/2.0A %";
#endif
/*
 *
 *                           (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_audit_server()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      audit_server request to the ACSSS software.  An AUDIT_SERVER
 *      request packet is constructed (using the parameters given) and
 *      sent the to SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE.
 *
 * Parameters:
 *
 *  seqNumber    - A client defined number returned in the response.
 *  capId        - The id of the CAP used for ejection of cartridges.
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
 *    David A. Myers       21-Nov-1991    Original.
 *    Scott Siao           16-Oct-1992    Changed bzeros to memsets.
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
#define SELF "acs_audit_server"
#undef ACSMOD
#define ACSMOD 47

STATUS acs_audit_server
(
    SEQ_NO seqNumber,
    CAPID capId
) 
{
    COPYRIGHT;

    STATUS acsReturn;

    AUDIT_REQUEST auditServRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &auditServRequest,
	    sizeof (AUDIT_REQUEST),
	    seqNumber,
	    COMMAND_AUDIT,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);

	if (acsReturn == STATUS_SUCCESS) {
	    auditServRequest.type = TYPE_SERVER;
	    auditServRequest.cap_id = capId;
	    auditServRequest.count = 0;

	    /* Send the AUDIT ACS request to the SSI */

	    acsReturn = acs_send_request (&auditServRequest, sizeof (AUDIT_REQUEST));
	}
    }

    acs_trace_exit (acsReturn);

    return acsReturn;

}

