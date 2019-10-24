#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_audit_pa/2.01A %";
#endif
/*
*
*                           (c) Copyright (1993-1994)
*                      Storage Technology Corporation
*                            All Rights Reserved
*
* Name:
*      acs_audit_panel()
*
* Description:
*      This procedure is called by an Application Program to initiate a
*      audit_panel request to the ACSSS software.  A AUDIT PANEL
*      request packet is constructed (using the parameters given) and
*      sent to the SSI process.
*
* Parameters:
* 
*  seqNumber         - A client defined number returned in the response.
*  panelId[MAX_ID]  - An array of ids of panels to be audited.
*  capId             - The id of the CAP used for ejection of cartridges.
*  count             - The number of ids in the panel id array.
*
* Return Values:
*     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
*                   STATUS_INVALID_VALUE.
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
*    Scott Siao           19-Oct-1992    Changed bzeros to memsets
*    Emanuel Alongi       07-Aug-1992    Assign global packet version
                                         number.
*    Howard Freeman IV    30-Sep-1992    Conformance to R3.0 PFS
                                         <count>.
*    Ken Stickney         04-Jun-1993    Portablized ANSI version from
                                         RMLS/400 (Tom Rethard).
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
#define SELF "acs_audit_panel"
#undef ACSMOD
#define ACSMOD 2

STATUS acs_audit_panel
(
    SEQ_NO seqNumber,
    PANELID panelId[MAX_ID],
    CAPID capId,
    unsigned short count
) 
{
    COPYRIGHT;
    unsigned short index;
    ACSMESSAGES msg_num;
    STATUS acsReturn;
    AUDIT_REQUEST auditPanelRequest;

    acs_trace_entry ();

    acsReturn = acs_verify_ssi_running ();

    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &auditPanelRequest,
	    sizeof (AUDIT_REQUEST),
	    seqNumber,
	    COMMAND_AUDIT,
	    EXTENDED,
	    VERSION_LAST - (VERSION)1,
	    NO_LOCK_ID);
	if (acsReturn == STATUS_SUCCESS) {
	    auditPanelRequest.type = TYPE_PANEL;
	    auditPanelRequest.cap_id = capId;
	    auditPanelRequest.count = count;
	    if (count > MAX_ID) {
		msg_num = ACSMSG_BAD_COUNT;
		acs_error_msg ((&msg_num, count, MAX_ID));
		acsReturn = STATUS_INVALID_VALUE;
	    }
	    else {
		for (index = 0; index < count; index++) {
		    auditPanelRequest.identifier.panel_id[index]
			= panelId[index];
		}
		acsReturn = acs_send_request (&auditPanelRequest,
		    sizeof (AUDIT_REQUEST));
	    }
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}

