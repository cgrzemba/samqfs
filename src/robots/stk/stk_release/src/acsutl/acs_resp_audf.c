#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsutl/csrc/acs_resp_audf/2.1 %";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      acs_response_audf()
 *
 * Description:
 *   This routine takes a final audit response packet and takes it apart,
 *   returning via bufferForClient an ACS_AUDIT_"XXX"_RESPONSE structure
 *   filled in with the data from the response packet.
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
 *    Ken Stickney         04-Jun-1993    Ansi version from HSC 
 *                                        (Tom Rethard).
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

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "acsrsp_pvt.h"

/* Defines, Typedefs and Structure Definitions: */

#undef  SELF
#define SELF  "acs_audit_fin_response"
#undef ACSMOD
#define ACSMOD 202

/*===================================================================*/
/*                                                                   */
/*                acs_audit_fin_response                             */
/*                                                                   */
/*===================================================================*/

STATUS acs_audit_fin_response
  (
    char *bufferForClient,
    ALIGNED_BYTES rspBfr
) {
    COPYRIGHT;
    ACSFMTREC toClient;
    SVRFMTREC frmServer;

    STATUS acsRtn;

    AU_ACS_STATUS *hACSSTAT;
    AU_LSM_STATUS *hLSMSTAT;
    AU_PNL_STATUS *hPNLSTAT;
    AU_SUB_STATUS *hSUBSTAT;

    unsigned int i;
    ACSMESSAGES msg_num;

    acs_trace_entry ();

    toClient.haAAR = (ACS_AUDIT_ACS_RESPONSE *) bufferForClient;
    frmServer.hAR = (AUDIT_RESPONSE *) rspBfr;
    
    switch (frmServer.hAR->type) {
       /*---------------------------------------------------------------*/
       /*   ACS FINAL RESPONSE                                            */
       /*---------------------------------------------------------------*/
     case TYPE_ACS:
       toClient.haAAR->count
             = frmServer.hAR->count;
       toClient.haAAR->audit_acs_status
             = frmServer.hAR->message_status.status;
       for (i = 0; i < toClient.haAAR->count; i++) {
             hACSSTAT = &frmServer.hAR->identifier_status.acs_status[i];
             toClient.haAAR->acs[i] = hACSSTAT->acs_id;
             toClient.haAAR->acs_status[i] = hACSSTAT->status.status;
       }
       acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*   LSM FINAL RESPONSE                                          */
	 /*---------------------------------------------------------------*/
     case TYPE_LSM:
	 toClient.haALR->count
	     = frmServer.hAR->count;
	 toClient.haALR->audit_lsm_status
	     = frmServer.hAR->message_status.status;
	 for (i = 0; i < toClient.haALR->count; i++) {
	     hLSMSTAT = &frmServer.hAR->identifier_status.lsm_status[i];
	     toClient.haALR->lsm_id[i] = hLSMSTAT->lsm_id;
	     toClient.haALR->lsm_status[i] = hLSMSTAT->status.status;
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*   PANEL FINAL RESPONSE                                        */
	 /*---------------------------------------------------------------*/
     case TYPE_PANEL:
	 toClient.haAPR->audit_pnl_status
	     = frmServer.hAR->message_status.status;
	 toClient.haAPR->count
	     = frmServer.hAR->count;
	 for (i = 0; i < toClient.haAPR->count; i++) {
	     hPNLSTAT = &frmServer.hAR->identifier_status.panel_status[i];
	     toClient.haAPR->panel_id[i] = hPNLSTAT->panel_id;
	     toClient.haAPR->panel_status[i] = hPNLSTAT->status.status;
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*   SUBPANEL FINAL RESPONSE                                     */
	 /*---------------------------------------------------------------*/
     case TYPE_SUBPANEL:
	 toClient.haASPR->audit_sub_status
	     = frmServer.hAR->message_status.status;
	 toClient.haASPR->count
	     = frmServer.hAR->count;
	 for (i = 0; i < toClient.haASPR->count; i++) {
	     hSUBSTAT =
		 &frmServer.hAR->identifier_status.subpanel_status[i];
	     toClient.haASPR->subpanel_id[i] = hSUBSTAT->subpanel_id;
	     toClient.haASPR->subpanel_status[i] = hSUBSTAT->status.status;
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*   UNKNOWN FINAL RESPONSES                                     */
	 /*---------------------------------------------------------------*/
     case TYPE_SERVER:
         toClient.haASRR->audit_srv_status
            = frmServer.hAR->message_status.status;
	 acsRtn = STATUS_SUCCESS;
	 break;

     default:
	 msg_num = ACSMSG_BAD_AUDIT_RESP_TYPE;
	 acs_error_msg ((&msg_num, frmServer.hAR->type));
	 acsRtn = STATUS_PROCESS_FAILURE;
    }
    acs_trace_exit (acsRtn);
    return acsRtn;
}

