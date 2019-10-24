#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  1/csrc/acs_query_res.c/3 %";
#endif
/*
 *
 * Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *      acs_query_response()
 *
 * Description:
 *    This routine takes a query response packet and takes it apart,
 *    returning via bufferForClient an ACS_QUERY_"XXX"_RESPONSE structure
 *    filled in with the data from the response packet.
 *
 * Return Values:
 *     STATUS_SUCCESS.
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
 *                                        messaging. Numerous changes to
 *                                        support enhanced version 3 and
 *                                        4 packets.
 *    Ken Stickney         26-May-1994    Added code to handle STATUS_INVALID
 *                                        _TYPE response from downlevel server
 *                                        to a query mixed media request.
 *    Scott Siao           06-Feb-2002    Added QUERY_DRIVE_GROUP,
 *                                              QUERY_SUBPOOL_NAME
 *    Scott Siao           28-Feb-2002    Added QUERY_MOUNT_SCRATCH_PINFO,
 *    Mike Williams        27-May-2010    Included string.h and strings.h to
 *                                        get the prototype for strcpy
 *
 */

/* Header Files: */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "acsrsp_pvt.h"

/* Defines, Typedefs and Structure Definitions: */

#undef  SELF
#define SELF  "acs_query_response"
#undef ACSMOD
#define ACSMOD 201

/* Procedure Type Declarations: */
/*===================================================================*/
/*                                                                   */
/*                acs_query_response                                 */
/*                                                                   */
/*===================================================================*/

STATUS acs_query_response (
    char *bufferForClient,
    ALIGNED_BYTES rspBfr)
{
    COPYRIGHT;
    ACSFMTREC toClient;
    SVRFMTREC frmServer;

    unsigned int i;
    unsigned int cnt;
    ACSMESSAGES msg_num;

    STATUS acsRtn;

    acs_trace_entry ();

    toClient.haQACSR = (ACS_QUERY_ACS_RESPONSE *) bufferForClient;
    frmServer.hQR = (QUERY_RESPONSE *) rspBfr;

    if(frmServer.hQR->message_status.status == STATUS_INVALID_TYPE) {
        switch (frmServer.hQR->message_status.type) {
          case TYPE_MIXED_MEDIA_INFO:
	      toClient.haQMMR->query_mmi_status
	          = frmServer.hQR->message_status.status;
              toClient.haQMMR->mixed_media_info_status.media_type_count = 0;
              toClient.haQMMR->mixed_media_info_status.drive_type_count = 0;
	      acsRtn = STATUS_SUCCESS;
            break;
          default:
	      msg_num = ACSMSG_BAD_QUERY_RESP_TYPE;
	      acs_error_msg ((&msg_num, frmServer.hQR->type));
	      acsRtn = STATUS_PROCESS_FAILURE;
        }
        acs_trace_exit (acsRtn);
        return acsRtn;
    }

    switch (frmServer.hQR->type) {
	 /*---------------------------------------------------------------*/
	 /*      QUERY ACS                                                */
	 /*---------------------------------------------------------------*/
     case TYPE_ACS:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQACSR->query_acs_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQACSR->query_acs_status != STATUS_SUCCESS) {
             toClient.haQACSR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.acs_response.acs_count;
	 toClient.haQACSR->count = cnt;

	 for (i = 0; i < cnt; i++) {
	     toClient.haQACSR->acs_status[i]
		 = frmServer.hQR->status_response.acs_response.acs_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY CAP                                                */
	 /*---------------------------------------------------------------*/
     case TYPE_CAP:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQCR->query_cap_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQCR->query_cap_status != STATUS_SUCCESS) {
             toClient.haQCR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.cap_response.cap_count;
	 toClient.haQCR->count = cnt;

	 for (i = 0; i < cnt; i++) {
	     toClient.haQCR->cap_status[i]
		 = frmServer.hQR->status_response.cap_response.cap_status[i];
	 }

	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY CLEAN                                              */
	 /*---------------------------------------------------------------*/
     case TYPE_CLEAN:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQCLR->query_cln_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQCLR->query_cln_status != STATUS_SUCCESS) {
             toClient.haQCLR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.clean_volume_response.
	     volume_count;
	 toClient.haQCLR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQCLR->cln_status[i]
		 = frmServer.hQR->status_response.clean_volume_response.
		 clean_volume_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY DRIVE                                              */
	 /*---------------------------------------------------------------*/
     case TYPE_DRIVE:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQDR->query_drv_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQDR->query_drv_status != STATUS_SUCCESS) {
             toClient.haQDR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.drive_response.drive_count;
	 toClient.haQDR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQDR->drv_status[i]
		 = frmServer.hQR->status_response.drive_response.
		 drive_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

         /*---------------------------------------------------------------*/
         /*      QUERY DRIVE GROUP                                    E59 */
         /*---------------------------------------------------------------*/
     case TYPE_DRIVE_GROUP:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQDRG->query_drv_group_status
	      = frmServer.hQR->message_status.status;
         if (toClient.haQDRG->query_drv_group_status != STATUS_SUCCESS) {
	     toClient.haQDRG->count = 0;
	     break;
	 }
	 toClient.haQDRG->group_type = frmServer.hQR->status_response.
	    drive_group_response.group_type;
	 strcpy (toClient.haQDRG->group_id.groupid,
	         frmServer.hQR->status_response.drive_group_response.group_id.groupid);
	 cnt = frmServer.hQR->status_response.drive_group_response.
		     vir_drv_map_count;
	 toClient.haQDRG->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQDRG->virt_drv_map[i] = 
	       frmServer.hQR->status_response.drive_group_response.virt_drv_map[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY LSM                                                */
	 /*---------------------------------------------------------------*/
     case TYPE_LSM:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQLR->query_lsm_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQLR->query_lsm_status != STATUS_SUCCESS) {
             toClient.haQLR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.lsm_response.lsm_count;
	 toClient.haQLR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQLR->lsm_status[i]
		 = frmServer.hQR->status_response.lsm_response.lsm_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY MIXED MEDIA                                        */
	 /*---------------------------------------------------------------*/
     case TYPE_MIXED_MEDIA_INFO:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQMMR->query_mmi_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQMMR->query_mmi_status != STATUS_SUCCESS) {
            toClient.haQMMR->mixed_media_info_status.media_type_count = 0;
            toClient.haQMMR->mixed_media_info_status.drive_type_count = 0;
            break;
         }
	 toClient.haQMMR->mixed_media_info_status
	     = frmServer.hQR->status_response.mm_info_response;
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY MOUNT                                              */
	 /*---------------------------------------------------------------*/
     case TYPE_MOUNT:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQMR->query_mnt_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQMR->query_mnt_status != STATUS_SUCCESS) {
             toClient.haQMR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.pool_response.pool_count;
	 toClient.haQMR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQMR->mnt_status[i]
		 = frmServer.hQR->status_response.mount_response.
		 mount_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY MOUNT SCRATCH and QUERY_MOUNT_SCRATCH_PINFO        */
	 /*---------------------------------------------------------------*/
     case TYPE_MOUNT_SCRATCH:
     case TYPE_MOUNT_SCRATCH_PINFO:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQMSR->query_msc_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQMSR->query_msc_status != STATUS_SUCCESS) {
             toClient.haQMSR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.pool_response.pool_count;
	 toClient.haQMSR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQMSR->msc_status[i]
		 = frmServer.hQR->status_response.mount_scratch_response.
		 mount_scratch_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY POOL                                               */
	 /*---------------------------------------------------------------*/
     case TYPE_POOL:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQPR->query_pol_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQPR->query_pol_status != STATUS_SUCCESS) {
             toClient.haQPR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.pool_response.pool_count;
	 toClient.haQPR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQPR->pool_status[i]
		 = frmServer.hQR->status_response.pool_response.
		 pool_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY PORT                                               */
	 /*---------------------------------------------------------------*/
     case TYPE_PORT:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQPRR->query_prt_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQPRR->query_prt_status != STATUS_SUCCESS) {
             toClient.haQPRR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.port_response.port_count;
	 toClient.haQPRR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQPRR->prt_status[i]
		 = frmServer.hQR->status_response.port_response.
		 port_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY REQUEST                                            */
	 /*---------------------------------------------------------------*/
     case TYPE_REQUEST:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQRR->query_req_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQRR->query_req_status != STATUS_SUCCESS) {
             toClient.haQRR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.request_response.
	     request_count;
	 toClient.haQRR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQRR->req_status[i]
		 = frmServer.hQR->status_response.request_response.
		 request_status[i];
	 }
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY SCRATCH                                            */
	 /*---------------------------------------------------------------*/
     case TYPE_SCRATCH:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQSCRR->query_scr_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQSCRR->query_scr_status != STATUS_SUCCESS) {
             toClient.haQSCRR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.scratch_response.
	     volume_count;
	 toClient.haQSCRR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQSCRR->scr_status[i]
		 = frmServer.hQR->status_response.scratch_response.
		 scratch_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY SERVER                                             */
	 /*---------------------------------------------------------------*/
     case TYPE_SERVER:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQSVR->query_srv_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQSVR->query_srv_status != STATUS_SUCCESS) {
             toClient.haQSVR->count = 0;
             break;
         }
	 toClient.haQSVR->count = 1;
	 for (i = 0; i < 1; i++) {
	     toClient.haQSVR->srv_status[i]
		 = frmServer.hQR->status_response.server_response.
		 server_status;
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

         /*---------------------------------------------------------------*/
         /*      QUERY SUBPOOL NAME                                       */
	 /*---------------------------------------------------------------*/
     case TYPE_SUBPOOL_NAME:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQSNR->query_subpool_name_status
	     = frmServer.hQR->message_status.status;
	 if (toClient.haQSNR->query_subpool_name_status != STATUS_SUCCESS) {
	     toClient.haQSNR->count = 0;
	     break;
	 }
	 cnt = frmServer.hQR->status_response.subpl_name_response.spn_status_count;
	 toClient.haQSNR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQSNR->subpool_name_status[i]
	        = frmServer.hQR->status_response.subpl_name_response.
		  subpl_name_status[i];
	 }
         acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      QUERY VOLUME                                             */
	 /*---------------------------------------------------------------*/
     case TYPE_VOLUME:
	 /*acs_trace_point(SELF##_LINE_);*/
	 toClient.haQVR->query_vol_status
	     = frmServer.hQR->message_status.status;
         if (toClient.haQVR->query_vol_status != STATUS_SUCCESS) {
             toClient.haQVR->count = 0;
             break;
         }
	 cnt = frmServer.hQR->status_response.volume_response.
	     volume_count;
	 toClient.haQVR->count = cnt;
	 for (i = 0; i < cnt; i++) {
	     toClient.haQVR->vol_status[i]
		 = frmServer.hQR->status_response.volume_response.
		 volume_status[i];
	 }
	 acsRtn = STATUS_SUCCESS;
	 break;

	 /*---------------------------------------------------------------*/
	 /*      UNKNOWN QUERY TYPES                                      */
	 /*---------------------------------------------------------------*/
     default:
	 msg_num = ACSMSG_BAD_QUERY_RESP_TYPE;
	 acs_error_msg ((&msg_num,
	     frmServer.hQR->type));
	 acsRtn = STATUS_PROCESS_FAILURE;
    }
    acsRtn = STATUS_SUCCESS;
    acs_trace_exit (acsRtn);
    return acsRtn;
}

