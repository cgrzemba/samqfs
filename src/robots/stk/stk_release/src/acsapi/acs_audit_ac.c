#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_audit_ac/2.1 %";
#endif
 /*
 *
 *                          (c)  Copyright  (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_audit_acs()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      audit_acs request to the ACSSS software.  A AUDIT ACS request packet
 *      is constructed (using the parameters given) and sent the to SSI process.
 *
 * Parameters:
 *
 *  seqNumber     - A client defined number returned in the response.
 *  acs[]         - The id(s) of the ACS of interest.
 *  capId         - The id of the CAP used for ejection of cartridges.
 *  count         - The number of ids in the ACS id array.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE.
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
 *    Jim Montgomery       23-Jul-1990    Release 2.
 *    David A. Myers       21-Nov-1991    Version 3.
 *    Scott Siao           19-Oct-1992    Changed bzeros to memsets
 *    Emanuel Alongi       07-Aug-1992    Assign global packet version number.
 *    Howard Freeman IV    30-Sep-1992    Conformance to R3.0 PFS <count>.
 *    Ken Stickney         04-Jun-1993    Ansi version from HSC (Tom Rethard).
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed 
 *    Ken Stickney         06-May-1994    Replaced acs_ipc_write with new
 *                                        function acs_send_request for 
 *                                        down level server support.
 *    Mitch Black          20-Jun-1994    Change to handle multiple ACSs.
 *                                        Includes change to parameter list.
 */

#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

#undef SELF
#define SELF "acs_audit_acs"
#undef ACSMOD
#define ACSMOD 0

STATUS acs_audit_acs
(
    SEQ_NO seqNumber,
    ACS acs[MAX_ID],
    CAPID capId,
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    STATUS acsReturn;
    ACSMESSAGES msg_num;
    AUDIT_REQUEST auditACSRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &auditACSRequest,
	    sizeof (AUDIT_REQUEST),
	    seqNumber,
	    COMMAND_AUDIT,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);

	if (acsReturn == STATUS_SUCCESS) {
	    auditACSRequest.type = TYPE_ACS;
	    auditACSRequest.cap_id = capId;
	    auditACSRequest.count = count;

            /* copy multiple identifiers into the packet */
            if (count > MAX_ID) {
                msg_num = ACSMSG_BAD_COUNT;
                acs_error_msg ((&msg_num, count, MAX_ID));
                acsReturn = STATUS_INVALID_VALUE;
            }
            else {
                for (index = 0; index < count; index++) {
                    auditACSRequest.identifier.acs_id[index] = acs[index];
                }
 
                acsReturn = acs_send_request (&auditACSRequest,
                    sizeof (AUDIT_REQUEST));
            }
	}
    }
    acs_trace_exit (acsReturn);

    return acsReturn;
}

