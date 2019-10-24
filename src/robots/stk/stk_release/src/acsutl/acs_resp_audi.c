#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsutl/csrc/acs_resp_audi/2.0 %";
#endif
/*
 *
 *                            (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_response_audi()
 *
 * Description:
 *   This routine takes a intermediate audit response packet and 
 *   takes it apart, returning via bufferForClient an 
 *   ACS_AUDIT_"XXX"_RESPONSE structure filled in with the data 
 *   from the response packet.
 *
 * Return Values:
 *    STATUS_SUCCESS.
 *
 * Parameters:
 *    bufferForClient - pointer to space for the
 *                      ACS_VARY_"XXX"_RESPONSE data.
 *    rspBfr - the buffer containing the response packet (global).
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
 *    Ken Stickney         04-Jun-1993    ANSI version from
 *                                        RMLS/400 (Tom Rethard).
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright. Changes for new error
 *                                        messaging. Changes to support
 *                                        version 3 packet enhancements.
 */

 /* Header Files: */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "acsrsp_pvt.h"

 /* Defines, Typedefs and Structure Definitions: */

#undef  SELF
#define SELF  "acs_aud_int_response"
#undef ACSMOD
#define ACSMOD 203

/*===================================================================*/
/*                                                                   */
/*                acs_audit_int_response                             */
/*                                                                   */
/*===================================================================*/

STATUS acs_audit_int_response
  (
    char *bufferForClient,
    ALIGNED_BYTES rspBfr) {
    COPYRIGHT;
    unsigned int i;
    STATUS acsRtn;
    ACSFMTREC toClient;
    SVRFMTREC frmServer;

    acs_trace_entry ();

    toClient.haAAR = (ACS_AUDIT_ACS_RESPONSE *) bufferForClient;

    frmServer.hAR = (AUDIT_RESPONSE *) rspBfr;

    memset ((char *)toClient.haAIR, '\00', sizeof (ACS_AUDIT_INT_RESPONSE));

    toClient.haAIR->audit_int_status
	= frmServer.hAIR->message_status.status;

    toClient.haAIR->cap_id = frmServer.hAIR->cap_id;

    toClient.haAIR->count = frmServer.hAIR->count;

    for (i = 0; i < frmServer.hAIR->count; i++) {
	/*acs_trace_point(SELF ## _LINE_);*/
	toClient.haAIR->vol_id[i]
	    = frmServer.hAIR->volume_status[i].vol_id;
	toClient.haAIR->vol_status[i]
	    = frmServer.hAIR->volume_status[i].status.status;
    }

    acsRtn = STATUS_SUCCESS;

    acs_trace_exit (acsRtn);

    return acsRtn;

}

