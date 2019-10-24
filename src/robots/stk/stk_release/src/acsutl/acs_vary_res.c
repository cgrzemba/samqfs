#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsutl/csrc/acs_vary_res/2.01A %";
#endif
/*
 *
 *                           (c) Copyright (1993-2000)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_vary_response()
 *
 * Description:
 *   This routine takes a vary response packet and takes it apart,
 *   returning via bufferForClient an ACS_VARY_"XXX"_RESPONSE structure
 *   filled in with the data from the response packet.
 *
 * Return Values:
 *    STATUS_SUCCESS
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
 *    Ken Stickney         04-Jun-1993    Ansi version from HSC (Tom Rethard).
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright. Changes for new error
 *                                        messaging. Changes to support 
 *                                        version 3 packet enhancements.
 *   Scott Siao            26-Sep-2000    Modified status return for CAP
 *                                        LSM, and PORT.  They had been
 *                                        hardcoded for array member 0, now
 *                                        they will have real data.
 */

/* File was formerly named acs200.c */

/* Header Files: */

#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "acsrsp_pvt.h"

/* Defines, Typedefs and Structure Definitions: */

#undef  SELF
#define SELF  "acs_vary_response"
#undef ACSMOD
#define ACSMOD 204

/* Procedure Type Declarations: */
/*===================================================================*/
/*                                                                   */
/*                acs_vary_response                                  */
/*                                                                   */
/*===================================================================*/

STATUS acs_vary_response (char *bufferForClient,
    ALIGNED_BYTES rspBfr)
{
    ACSFMTREC toClient;
    SVRFMTREC frmServer;

    unsigned short i;
    ACSMESSAGES msg_num;

    STATUS acsRtn;

    acs_trace_entry ();

    toClient.haAAR = (ACS_AUDIT_ACS_RESPONSE *) bufferForClient;
    frmServer.hAR = (AUDIT_RESPONSE *) rspBfr;

    switch (frmServer.hVR->type) {
         /*---------------------------------------------------------------*/
         /*      VARY  ACS                                                */
         /*---------------------------------------------------------------*/
     case TYPE_ACS:
         toClient.haVACSR->vary_acs_status
             = frmServer.hVR->message_status.status;
         toClient.haVACSR->count
             = frmServer.hVR->count;
         toClient.haVACSR->acs_state = frmServer.hVR->state;
         for(i = 0; i < frmServer.hVR->count; i++) {
             toClient.haVACSR->acs[i]
                 = frmServer.hVR->device_status.acs_status[i].acs_id;
             toClient.haVACSR->acs_status[i]
             = frmServer.hVR->device_status.acs_status[i].status.status;
             acsRtn = STATUS_SUCCESS;
         }
         break;

         /*---------------------------------------------------------------*/
         /*      VARY  CAP                                                */
         /*---------------------------------------------------------------*/
     case TYPE_CAP:
         toClient.haVCPR->vary_cap_status
             = frmServer.hVR->message_status.status;
         toClient.haVCPR->cap_state = frmServer.hVR->state;
         toClient.haVCPR->count
             = frmServer.hVR->count;
         for(i = 0; i < frmServer.hVR->count; i++) {
             toClient.haVCPR->cap_id[i]
                 = frmServer.hVR->device_status.cap_status[i].cap_id;
             toClient.haVCPR->cap_status[i]
                 = frmServer.hVR->device_status.cap_status[i].status.
                   status;
         }
         acsRtn = STATUS_SUCCESS;
         break;

         /*---------------------------------------------------------------*/
         /*      VARY DRIVE                                               */
         /*---------------------------------------------------------------*/
     case TYPE_DRIVE:
         toClient.haVDR->vary_drv_status
             = frmServer.hVR->message_status.status;
         toClient.haVDR->drive_state = frmServer.hVR->state;
         toClient.haVDR->count
             = frmServer.hVR->count;
         for(i = 0; i < frmServer.hVR->count; i++) {
             toClient.haVDR->drive_id[i]
                 = frmServer.hVR->device_status.drive_status[i].drive_id;
             toClient.haVDR->drive_status[i]
                 = frmServer.hVR->device_status.drive_status[i].status.
                   status;
         }
         acsRtn = STATUS_SUCCESS;
        
         break;

         /*---------------------------------------------------------------*/
         /*      VARY LSM                                                 */
         /*---------------------------------------------------------------*/
     case TYPE_LSM:
         toClient.haVLR->vary_lsm_status
             = frmServer.hVR->message_status.status;
         toClient.haVLR->lsm_state = frmServer.hVR->state;
         toClient.haVLR->count
             = frmServer.hVR->count;
         for(i = 0; i < frmServer.hVR->count; i++) {
             toClient.haVLR->lsm_id[i]
                 = frmServer.hVR->device_status.lsm_status[i].lsm_id;
             toClient.haVLR->lsm_status[i]
                 = frmServer.hVR->device_status.lsm_status[i].status.
                   status;
         }
         acsRtn = STATUS_SUCCESS;
         break;

         /*---------------------------------------------------------------*/
         /*      VARY PORT                                                */
         /*---------------------------------------------------------------*/
     case TYPE_PORT:
         toClient.haVPRR->vary_prt_status
             = frmServer.hVR->message_status.status;
         toClient.haVPRR->port_state = frmServer.hVR->state;
         toClient.haVLR->count
             = frmServer.hVR->count;
         for(i = 0; i < frmServer.hVR->count; i++) {
             toClient.haVPRR->port_id[i]
                 = frmServer.hVR->device_status.port_status[i].port_id;
             toClient.haVPRR->port_status[i]
                 = frmServer.hVR->device_status.port_status[i].status.
                   status;
         }
         acsRtn = STATUS_SUCCESS;
         break;

         /*---------------------------------------------------------------*/
         /*      UNKNOWN VARY TYPES                                       */
         /*---------------------------------------------------------------*/
     default:
         /*sprintf(msg, "%s: Bad VARY response type:%d\n", SELF,
         frmServer.hVR -> type);*/
	 msg_num = ACSMSG_BAD_VARY_RESP_TYPE;
         acs_error_msg ((&msg_num,
             frmServer.hVR->type));
         acsRtn = STATUS_PROCESS_FAILURE;
    }
    acsRtn = STATUS_SUCCESS;
    acs_trace_exit (acsRtn);
    return acsRtn;
}

