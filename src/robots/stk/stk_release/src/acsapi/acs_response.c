# ifndef lint
static char SccsId[] = "@(#) %full_name:  1/csrc/acs_response.c/5 %";
# endif
/*
 *
 *                            (c) Copyright (1993-2004)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_response()
 *
 * Description:
 *      This procedure is called by an Application Program to retrieve
 *      responses from the ACSSS software.
 *
 *      acs_response() waits timeout seconds for a response from the SSI
 *      A timeout value of -1 causes acs_response() to block
 *        indefinitely.
 *      A timeout value of 0  effects a poll of response availability.
 *
 * Parameters:
 *
 *  timeout          - Time to wait for response. 
 *  hSeqNum          - Pointer to space for sequence number to be returned.
 *  hRequestId       - Pointer to space for request id to be returned.
 *  hType,             - Pointer to space for response type to be returned.
 *  bufferForClient  - Pointer to buffer for response packet to be returned.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE,
 *     STATUS_PROCESS_FAILURE, or STATUS_PENDING.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *     This module is machine dependent. The function signature is
 *     constant across platforms, but the code contained herein will
 *     vary.
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Jim Montgomery       26-Jul-1989    Original.
 *    Jim Montgomery       23-Jul-1990    Version 2.
 *    David A. Myers       21-Nov-1991    Version 3.
 *    Scott Siao           19-Oct-1992    Changed bzeros to memsets and
 *                                        bcopys to memcpys.
 *    Emanuel Alongi       07-Aug-1992    Assign global packet version
 *                                        number.
 *    Ken Stickney         04-Jun-1993    ANSI version from RMLS/400
 *                                        (Tom Rethard).
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened 
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler, 
 *                                        added defines ACSMOD and SELF for 
 *                                        trace and error messages, fixed  
 *                                        copyright.
 *    Ken Stickney         13-Jan-1994    Added the setting of the cap_used 
 *                                        array in the ACS_EJECT_RESPONSE 
 *                                        for COMMAND_EJECT.
 *    Ken Stickney         17-Jan-1994    Added the setting of the 
 *                                        dlock_vol_id & dlock_drive_id 
 *                                        in the ACS_LOCK_VOL_RESPONSE 
 *                                        and ACS_LOCK_DRV_RESPONSE
 *                                        for COMMAND_LOCK.
 *    Ken Stickney         06-May-1994    Replaced acs_ipc_write with new
 *                                        function acs_send_request for 
 *                                        down level server support.
 *    Ken Stickney         31-May_1994    Changes in support of ANSI 
 *                                        standard args (stdarg.h) usage in
 *                                        acs_error_msg().
 *    Ken Stickney         08-Aug_1994    Fixed status checking on return
 *                                        from acs_get_response().
 *    Ken Stickney         08-Aug_1994    Fix for BR#37 in "bugs" data base.
 *    Ken Stickney         27-Jun-1995    Fixed status checking of final
 *					  responses.
 *    Van Lepthien         22-Aug-2001    Repeat select if acs_get_response
 *                                        returns STATUS_PENDING.
 *                                       Update Toolkit Version String. 
 *    Scott Siao           10-11-2001     Added support for acs_register,
 *                                        acs_unregister, acs_check_registration
 *    Scott Siao           11-12-2001     Added support for acs_display.
 *    Scott Siao           02-14-2002     Added support for acs_mount_pinfo.
 *    Scott Siao           04-19-2002     Moved acsRtn in define pool outside of loop.
 *                                        Added acsRtn for Idle.
 *    Mitch Black          28-Mar-2004    Corrected error message for failed select().
 *    Wipro(Hemendra)	   24-Jun-2004	  Added code to handle STATUS_NI_FAILURE (from
 *                           bug found in Toulouse Spring 2004; code change suggested
 *                           by Mitch).
 */

/* Header Files: */

# include <stddef.h>
# include <stdio.h>
# include <string.h>

# include "acssys.h"
# include "acsapi.h"
# include "acssys_pvt.h"
# include "acsapi_pvt.h"
# include "acsrsp_pvt.h"

/* Defines, Typedefs and Structure Definitions: */

# undef  SELF
# define SELF  "acs_response"
# undef ACSMOD
# define ACSMOD 32

/* Procedure Type Declarations: */

/* Global and Static Variables */

/* THIS IS THE VERSION STRING THAT DEFINES THE VERSION OF THE ACSAPI TOOLKIT */
/* THIS MUST BE UPDATED WITH EACH RELEASE OF THE TOOLKIT */

char toolkit_version_string[] = "ACSAPI TOOLKIT VERSION 2.3";
STATUS acs_response
(
    int timeout,        	/* time to wait for response */
    SEQ_NO * hSeqNum,        	/* space for sequence number */
    REQ_ID * hRequestId,        /* space for request id      */
    ACS_RESPONSE_TYPE * hType,        /* space for response type   */
    ALIGNED_BYTES bufferForClient /* buffer for response packet */
)
{

    COPYRIGHT;
    ALIGNED_BYTES rspBfr[MAX_MESSAGE_SIZE / sizeof (ALIGNED_BYTES)];
    ACSFMTREC toClient;
    SVRFMTREC frmServer;

    unsigned int i;        	/* unsigned for compatibility */
    ACSMESSAGES msg_num;
    size_t size;

    STATUS acsRtn;
	
    acs_trace_entry ();
	
    acsRtn = acs_verify_ssi_running ();
	
    if (acsRtn != STATUS_SUCCESS) {
	msg_num = ACSMSG_SSI_NOT_RUNNING;
	acs_error_msg((&msg_num, NULL));
	acs_trace_exit(acsRtn);
	*hType = RT_NONE;
	return acsRtn;
    }
    
    /* Loop while STATUS_PENDING */
    
    do
    {
        /* Wait up to timeout seconds for response from the SSI */
        acsRtn = acs_select_input (timeout) /*1, &sd_in, timeout)*/ ;
        if(acsRtn == STATUS_PENDING) {
            *hType = RT_NONE;
            return acsRtn;
        }
        if(acsRtn == STATUS_IPC_FAILURE) {
            msg_num = ACSMSG_BAD_INPUT_SELECT;
            acs_error_msg ((&msg_num, NULL));
            acs_trace_exit (acsRtn);
            *hType = RT_NONE;
            return acsRtn;
        }
	
	
        /*-----------------------------------------------------------------*/
        /*                                                                 */
        /*               Read one response from the SSI                    */
        /*                                                                 */
        /*-----------------------------------------------------------------*/
	
        acsRtn = acs_get_response (rspBfr, &size);

#ifdef DEBUG
        printf("\nacs_response(): acs_get_response() returns %s.\n",acs_status(acsRtn));
#endif

        if (acsRtn == STATUS_IPC_FAILURE) 
        {
            msg_num = ACSMSG_IPC_READ_FAILED;
            acs_error_msg ((&msg_num, NULL));
            acs_trace_exit (acsRtn);
            *hType = RT_NONE;
            return acsRtn;
        }
    } while (acsRtn == STATUS_PENDING);
	
    if ((acsRtn == STATUS_IPC_FAILURE) ||
       (acsRtn == STATUS_NI_FAILURE) ||
       (acsRtn == STATUS_PENDING) ||
       (acsRtn == STATUS_PROCESS_FAILURE))
    { /* Please note that check for other statuses is superfluous and added for completeness. */
       /* Future enhancement: Call acs_error_msg here to report the specific error */
       acs_trace_exit (acsRtn);
       *hType = RT_NONE;
       return acsRtn;
    }
	
    frmServer.hRH = (REQUEST_HEADER *) rspBfr;

    /* Extended bit must be set */

    if (!(frmServer.hRH->message_header.message_options & EXTENDED)) {
	msg_num = ACSMSG_NOT_EXTENDED;
        acs_error_msg ((&msg_num, NULL));
        acsRtn = STATUS_PROCESS_FAILURE;
        acs_trace_exit (acsRtn);
        return acsRtn;
    }

    /*-----------------------------------------------------------------*/
    /*                                                                 */
    /*              Fill in output parameter information               */
    /*                                                                 */
    /*-----------------------------------------------------------------*/

    *hSeqNum = frmServer.hRH->message_header.packet_id;

    *hRequestId = 0;

    if (frmServer.hRH->message_header.message_options & INTERMEDIATE) {
        /* this is an INTERMEDIATE message */
        *hType = RT_INTERMEDIATE;
    }

    else if (frmServer.hRH->message_header.message_options
        & ACKNOWLEDGE) {
        /* this is an ACKNOWLEDGE message */
        *hType = RT_ACKNOWLEDGE;
        frmServer.hACKR = (ACKNOWLEDGE_RESPONSE *) rspBfr;
        *hRequestId = (REQ_ID) frmServer.hACKR->message_id;
        memcpy (bufferForClient, (const char *)rspBfr, sizeof (rspBfr));
        acsRtn = STATUS_SUCCESS;
        acs_trace_exit (acsRtn);
        return acsRtn;
    }

    else {
        /*
    ** although there is no way to verify that this is
    ** a FINAL message, we make that assumption.  ANY packet
    ** which is not marked ACKNOWLEDGE or INTERMEDIATE will
    ** be treated as if it is a FINAL
    ** This is, of course, a potential for delayed errors.
    */
        *hType = RT_FINAL;
        if ((acsRtn == STATUS_IPC_FAILURE) || 
            (acsRtn == STATUS_PENDING) ||
            (acsRtn == STATUS_PROCESS_FAILURE)){
            return acsRtn;
        }   
    }

    /*-----------------------------------------------------------------*/
    /*                                                                 */
    /*       Set the toClient pointers so we can process the packet    */
    /*                                                                 */
    /*-----------------------------------------------------------------*/

    toClient.haAAR = (ACS_AUDIT_ACS_RESPONSE *) bufferForClient;

    /*-----------------------------------------------------------------*/
    /*                                                                 */
    /*      Reformat the packet for the client application             */
    /*                                                                 */
    /*-----------------------------------------------------------------*/

    switch (frmServer.hRH->message_header.command) {
	/*---------------------------------------------------------------*/
	/*    COMMAND_AUDIT                                              */
	/*---------------------------------------------------------------*/
      case COMMAND_AUDIT:
	if (*hType == RT_INTERMEDIATE) {
	    acsRtn = acs_audit_int_response(bufferForClient, rspBfr);
	    if (acsRtn != STATUS_SUCCESS) {
		msg_num = ACSMSG_BAD_INT_RESPONSE;
		acs_error_msg((&msg_num, NULL));
		acsRtn = STATUS_PROCESS_FAILURE;
	    }
	}
	else {
						/* RT_FINAL */
	    acsRtn = acs_audit_fin_response(bufferForClient, rspBfr);
	    if (acsRtn != STATUS_SUCCESS) {
		msg_num = ACSMSG_BAD_FINAL_RESPONSE;
		acs_error_msg((&msg_num, NULL));
		acsRtn = STATUS_PROCESS_FAILURE;
	    }
	}
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_CLEAR_LOCK                                         */
	/*---------------------------------------------------------------*/
      case COMMAND_CLEAR_LOCK:
	if (frmServer.hCLR->type == TYPE_DRIVE) {
	    toClient.haCLDR->clear_lock_drv_status
	    = frmServer.hCLR->message_status.status;
	    toClient.haCLDR->count
	    = frmServer.hCLR->count;
	    for (i = 0; i < frmServer.hCLR->count; i++) {
		toClient.haCLDR->drv_status[i].status
		= frmServer.hCLR->
		  identifier_status.drive_status[i].status.status;
		toClient.haCLDR->drv_status[i].drive_id
		= frmServer.hCLR->
		  identifier_status.drive_status[i].drive_id;
	    }
	}
	else {
	    /* TYPE_VOLUME */
	    toClient.haCLVR->clear_lock_vol_status
	    = frmServer.hCLR->message_status.status;
	    toClient.haCLVR->count
	    = frmServer.hCLR->count;
	    for (i = 0; i < frmServer.hCLR->count; i++) {
		toClient.haCLVR->vol_status[i].vol_id
		= frmServer.hCLR->
		  identifier_status.volume_status[i].vol_id;
		toClient.haCLVR->vol_status[i].status
		= frmServer.hCLR->identifier_status.
		  volume_status[i].status.status;
	    }
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_CANCEL                                             */
	/*---------------------------------------------------------------*/
      case COMMAND_CANCEL:
	toClient.haCR->cancel_status
	= frmServer.hCR->message_status.status;
	toClient.haCR->req_id = frmServer.hCR->request;
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_CHECK_REGISTRATION                                 */
	/*---------------------------------------------------------------*/
      case COMMAND_CHECK_REGISTRATION:
	toClient.haCRR->check_registration_status
	= frmServer.hCRR->message_status.status;
	toClient.haCRR->event_register_status.registration_id
	= frmServer.hCRR->event_register_status.registration_id;
	toClient.haCRR->event_register_status.count
	= frmServer.hCRR->event_register_status.count;
	for (i = 0; i < toClient.haCRR->event_register_status.count; i++) {
	    toClient.haCRR->event_register_status.
	    register_status[i].event_class
	    = frmServer.hCRR->event_register_status.
	      register_status[i].event_class;
	    toClient.haCRR->event_register_status.
	    register_status[i].register_return
	    = frmServer.hCRR->event_register_status.
	      register_status[i].register_return;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_DEFINE_POOL                                       */
	/*---------------------------------------------------------------*/
      case COMMAND_DEFINE_POOL:
	toClient.haDFPR->define_pool_status
	= frmServer.hDFPR->message_status.status;
	toClient.haDFPR->lwm = frmServer.hDFPR->low_water_mark;
	toClient.haDFPR->hwm = frmServer.hDFPR->high_water_mark;
	toClient.haDFPR->attributes
	= frmServer.hDFPR->pool_attributes;
	toClient.haDFPR->count = frmServer.hDFPR->count;
	for (i = 0; i < frmServer.hDFPR->count; i++) {
	    toClient.haDFPR->pool[i]
	    = frmServer.hDFPR->pool_status[i].pool_id.pool;
	    toClient.haDFPR->pool_status[i]
	    = frmServer.hDFPR->pool_status[i].status.status;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_DELETE_POOL                                        */
	/*---------------------------------------------------------------*/
      case COMMAND_DELETE_POOL:
	toClient.haDLPR->delete_pool_status
	= frmServer.hDLPR->message_status.status;
	toClient.haDLPR->count = frmServer.hDLPR->count;
	for (i = 0; i < frmServer.hDLPR->count; i++) {
	    toClient.haDLPR->pool[i]
	    = frmServer.hDLPR->pool_status[i].pool_id.pool;
	    toClient.haDLPR->pool_status[i]
	    = frmServer.hDLPR->pool_status[i].status.status;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_DISMOUNT                                           */
	/*---------------------------------------------------------------*/
      case COMMAND_DISMOUNT:
	toClient.haDR->dismount_status
	= frmServer.hDR->message_status.status;
	toClient.haDR->vol_id = frmServer.hDR->vol_id;
	toClient.haDR->drive_id = frmServer.hDR->drive_id;
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_DISPLAY                                            */
	/*---------------------------------------------------------------*/
      case COMMAND_DISPLAY:
	toClient.haDSP->display_status
	= frmServer.hDSP->message_status.status;
	toClient.haDSP->display_type = frmServer.hDSP->display_type;
	toClient.haDSP->display_xml_data = frmServer.hDSP->display_xml_data;
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_EJECT                                              */
	/*---------------------------------------------------------------*/
      case COMMAND_EJECT:
	memset((char *) toClient.haEJR, '\00', sizeof(
						      ACS_EJECT_RESPONSE));
	toClient.haEJR->eject_status
	= frmServer.hEE->message_status.status;
	toClient.haEJR->cap_id = frmServer.hEE->cap_id;
	toClient.haEJR->count = frmServer.hEE->count;
	for (i = 0; i < frmServer.hEE->count; i++) {
	    toClient.haEJR->vol_id[i]
	    = frmServer.hEE->volume_status[i].vol_id;
	    toClient.haEJR->vol_status[i]
	    = frmServer.hEE->volume_status[i].status.status;
	    toClient.haEJR->cap_used[i]
	    = frmServer.hEE->volume_status[i].status.identifier.cap_id;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_ENTER                                              */
	/*---------------------------------------------------------------*/
      case COMMAND_ENTER:
	memset((char *) toClient.haEJR, '\00', sizeof(
						      ACS_ENTER_RESPONSE));
	toClient.haENR->enter_status
	= frmServer.hEE->message_status.status;
	toClient.haENR->cap_id = frmServer.hEE->cap_id;
	toClient.haENR->count = frmServer.hEE->count;
	for (i = 0; i < frmServer.hEE->count; i++) {
	    toClient.haENR->vol_id[i]
	    = frmServer.hEE->volume_status[i].vol_id;
	    toClient.haENR->vol_status[i]
	    = frmServer.hEE->volume_status[i].status.status;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_IDLE                                               */
	/*---------------------------------------------------------------*/

      case COMMAND_IDLE:
	toClient.haIR->idle_status
	= frmServer.hIR->message_status.status;
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_LOCK                                               */
	/*---------------------------------------------------------------*/

      case COMMAND_LOCK:
	/*-------------------------------------------------------------*/
	/*  COMMAND_LOCK  DRIVE                                        */
	/*-------------------------------------------------------------*/

	if (frmServer.hLR->type == TYPE_DRIVE) {
	    toClient.haLDR->lock_drv_status
	    = frmServer.hLR->message_status.status;
	    toClient.haLDR->lock_id
	    = frmServer.hLR->request_header.message_header.lock_id;
	    toClient.haLDR->count
	    = frmServer.hLR->count;
	    for (i = 0; i < frmServer.hLR->count; i++) {
		toClient.haLDR->drv_status[i].drive_id
		= frmServer.hLR->
		  identifier_status.drive_status[i].drive_id;
		toClient.haLDR->drv_status[i].dlocked_drive_id
		= frmServer.hLR->
		  identifier_status.drive_status[i].status.
		  identifier.drive_id;
		toClient.haLDR->drv_status[i].status
		= frmServer.hLR->
		  identifier_status.drive_status[i].status.status;
	    }
	}
	/*-------------------------------------------------------------*/
	/*  COMMAND_LOCK  VOLUME                                       */
	/*-------------------------------------------------------------*/

	else {
	    /* TYPE_VOLUME */
	    toClient.haLVR->lock_vol_status
	    = frmServer.hLR->message_status.status;
	    toClient.haLVR->lock_id
	    = frmServer.hLR->request_header.message_header.lock_id;
	    toClient.haLVR->count
	    = frmServer.hLR->count;
	    for (i = 0; i < frmServer.hLR->count; i++) {
		toClient.haLVR->vol_status[i].vol_id
		= frmServer.hLR->
		  identifier_status.volume_status[i].vol_id;
		toClient.haLVR->vol_status[i].dlocked_vol_id
		= frmServer.hLR->
		  identifier_status.volume_status[i].status.
		  identifier.vol_id;
		toClient.haLVR->vol_status[i].status
		= frmServer.hLR->
		  identifier_status.volume_status[i].status.status;
	    }
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_MOUNT                                              */
	/*---------------------------------------------------------------*/

      case COMMAND_MOUNT:
	toClient.haMR->mount_status
	= frmServer.hMR->message_status.status;
	toClient.haMR->vol_id = frmServer.hMR->vol_id;
	toClient.haMR->drive_id = frmServer.hMR->drive_id;
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_MOUNT_SCRATCH                                      */
	/*---------------------------------------------------------------*/

      case COMMAND_MOUNT_SCRATCH:
	toClient.haMSR->mount_scratch_status
	= frmServer.hMSR->message_status.status;
	toClient.haMSR->vol_id = frmServer.hMSR->vol_id;
	toClient.haMSR->pool = frmServer.hMSR->pool_id.pool;
	toClient.haMSR->drive_id = frmServer.hMSR->drive_id;
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_QUERY                                              */
	/*---------------------------------------------------------------*/
      case COMMAND_QUERY:
	acsRtn = acs_query_response(bufferForClient, rspBfr);
	if (acsRtn != STATUS_SUCCESS) {
	    msg_num = ACSMSG_BAD_QUERY_RESPONSE;
	    acs_error_msg((&msg_num, NULL));
	    acsRtn = STATUS_PROCESS_FAILURE;
	    acs_trace_exit(acsRtn);
	    return acsRtn;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_QUERY_LOCK                                         */
	/*---------------------------------------------------------------*/
      case COMMAND_QUERY_LOCK:
	/*-------------------------------------------------------------*/
	/*    COMMAND_QUERY_LOCK    DRIVE                              */
	/*-------------------------------------------------------------*/
	if (frmServer.hQLR->type == TYPE_DRIVE) {
	    toClient.haQLDR->query_lock_drv_status
	    = frmServer.hQLR->message_status.status;
	    toClient.haQLDR->count = frmServer.hQLR->count;
	    for (i = 0; i < frmServer.hQLR->count; i++) {
		toClient.haQLDR->drv_status[i]
		= frmServer.hQLR->identifier_status.drive_status[i];
	    }
	}
	/*-------------------------------------------------------------*/
	/*    COMMAND_QUERY_LOCK    VOLUME                             */
	/*-------------------------------------------------------------*/
	else {
	    /* TYPE_VOLUME */
	    toClient.haQLVR->query_lock_vol_status
	    = frmServer.hQLR->message_status.status;
	    toClient.haQLVR->count = frmServer.hQLR->count;
	    for (i = 0; i < frmServer.hQLR->count; i++) {
		toClient.haQLVR->vol_status[i]
		= frmServer.hQLR->identifier_status.volume_status[i];
	    }
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_REGISTER                                           */
	/*---------------------------------------------------------------*/
      case COMMAND_REGISTER:
	if (*hType == RT_INTERMEDIATE) {
	    acsRtn = acs_register_int_response(bufferForClient, rspBfr);
	    if (acsRtn != STATUS_SUCCESS) {
		msg_num = ACSMSG_BAD_INT_RESPONSE;
		acs_error_msg((&msg_num, NULL));
		acsRtn = STATUS_PROCESS_FAILURE;
	    }
	}
	else {
						/*RT_FINAL */
	    toClient.haRR->register_status
	    = frmServer.hRR->message_status.status;
	    toClient.haRR->event_reply_type = frmServer.hRR->event_reply_type;
	    toClient.haRR->event_sequence = frmServer.hRR->event_sequence;
	    toClient.haRR->event.event_register_status.registration_id
	    = frmServer.hRR->event.event_register_status.registration_id;
	    toClient.haRR->event.event_register_status.count
	    = frmServer.hRR->event.event_register_status.count;
	    for (i = 0; i < toClient.haRR->event.event_register_status.
		     count; i++) {
		toClient.haRR->event.event_register_status.
		register_status[i].event_class
		= frmServer.hRR->event.event_register_status.
		  register_status[i].event_class;
		toClient.haRR->event.event_register_status.
		register_status[i].register_return
		= frmServer.hRR->event.event_register_status.
		  register_status[i].register_return;
	    }
	    acsRtn = STATUS_SUCCESS;
	}
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_SET_CAP                                            */
	/*---------------------------------------------------------------*/
      case COMMAND_SET_CAP:
	toClient.haSECR->set_cap_status
	= frmServer.hSECR->message_status.status;
	toClient.haSECR->cap_priority = frmServer.hSECR->cap_priority;
	toClient.haSECR->cap_mode = frmServer.hSECR->cap_mode;
	toClient.haSECR->count = frmServer.hSECR->count;
	for (i = 0; i < frmServer.hSECR->count; i++) {
	    toClient.haSECR->cap_id[i]
	    = frmServer.hSECR->set_cap_status[i].cap_id;
	    toClient.haSECR->cap_status[i]
	    = frmServer.hSECR->set_cap_status[i].status.status;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_SET_CLEAN                                          */
	/*---------------------------------------------------------------*/
      case COMMAND_SET_CLEAN:
	toClient.haSECLNR->set_clean_status
	= frmServer.hSECLNR->message_status.status;
	toClient.haSECLNR->max_use = frmServer.hSECLNR->max_use;
	toClient.haSECLNR->count = frmServer.hSECLNR->count;
	for (i = 0; i < frmServer.hSECLNR->count; i++) {
	    toClient.haSECLNR->vol_id[i]
	    = frmServer.hSECLNR->volume_status[i].vol_id;
	    toClient.haSECLNR->vol_status[i]
	    = frmServer.hSECLNR->volume_status[i].status.status;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_SET_SCRATCH                                        */
	/*---------------------------------------------------------------*/
      case COMMAND_SET_SCRATCH:
	toClient.haSESCRR->set_scratch_status
	= frmServer.hSESCRR->message_status.status;
	toClient.haSESCRR->pool = frmServer.hSESCRR->pool_id.pool;
	toClient.haSESCRR->count = frmServer.hSESCRR->count;
	for (i = 0; i < frmServer.hSESCRR->count; i++) {
	    toClient.haSESCRR->vol_id[i]
	    = frmServer.hSESCRR->scratch_status[i].vol_id;
	    toClient.haSESCRR->vol_status[i]
	    = frmServer.hSESCRR->scratch_status[i].status.status;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_START                                              */
	/*---------------------------------------------------------------*/
      case COMMAND_START:
	toClient.haSR->start_status
	= frmServer.hSR->message_status.status;
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_UNLOCK                                             */
	/*---------------------------------------------------------------*/
      case COMMAND_UNLOCK:
	/*-------------------------------------------------------------*/
	/*    COMMAND_UNLOCK   DRIVE                                   */
	/*-------------------------------------------------------------*/
	if (frmServer.hULR->type == TYPE_DRIVE) {
	    toClient.haULDR->unlock_drv_status
	    = frmServer.hULR->message_status.status;
	    toClient.haULDR->count = frmServer.hULR->count;
	    for (i = 0; i < frmServer.hULR->count; i++) {
		toClient.haULDR->drv_status[i].drive_id
		= frmServer.hULR->
		  identifier_status.drive_status[i].drive_id;
		toClient.haULDR->drv_status[i].status
		= frmServer.hULR->
		  identifier_status.drive_status[i].status.status;
	    }
	}
	/*-------------------------------------------------------------*/
	/*    COMMAND_UNLOCK     VOLUME                                */
	/*-------------------------------------------------------------*/
	else {
	    /* TYPE_VOLUME */
	    toClient.haULVR->unlock_vol_status
	    = frmServer.hULR->message_status.status;
	    toClient.haULVR->count = frmServer.hULR->count;
	    for (i = 0; i < frmServer.hULR->count; i++) {
		toClient.haULVR->vol_status[i].vol_id
		= frmServer.hULR->
		  identifier_status.volume_status[i].vol_id;
		toClient.haULVR->vol_status[i].status
		= frmServer.hULR->
		  identifier_status.volume_status[i].status.status;
	    }
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_UNREGISTER                                         */
	/*---------------------------------------------------------------*/
      case COMMAND_UNREGISTER:
	toClient.haURR->unregister_status
	= frmServer.hURR->message_status.status;
	toClient.haURR->event_register_status.registration_id
	= frmServer.hURR->event_register_status.registration_id;
	toClient.haURR->event_register_status.count
	= frmServer.hURR->event_register_status.count;
	for (i = 0; i < toClient.haURR->event_register_status.count; i++) {
	    toClient.haURR->event_register_status.
	    register_status[i].event_class
	    = frmServer.hURR->event_register_status.
	      register_status[i].event_class;
	    toClient.haURR->event_register_status.
	    register_status[i].register_return
	    = frmServer.hURR->event_register_status.
	      register_status[i].register_return;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_VARY                                               */
	/*---------------------------------------------------------------*/
      case COMMAND_VARY:
	acsRtn = acs_vary_response(bufferForClient, rspBfr);
	if (acsRtn != STATUS_SUCCESS) {
	    msg_num = ACSMSG_VARY_RESP_FAILED;
	    acs_error_msg((&msg_num, NULL));
	    acsRtn = STATUS_PROCESS_FAILURE;
	    acs_trace_exit(acsRtn);
	    return acsRtn;
	}
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    COMMAND_MOUNT_PINFO                                      */
	/*---------------------------------------------------------------*/
      case COMMAND_MOUNT_PINFO:
	toClient.haVMR->mount_pinfo_status
	  = frmServer.hVMR->message_status.status;
	toClient.haVMR->pool_id = frmServer.hVMR->pool_id;
	toClient.haVMR->drive_id = frmServer.hVMR->drive_id;
	toClient.haVMR->vol_id = frmServer.hVMR->vol_id;
	acsRtn = STATUS_SUCCESS;
	break;
	/*---------------------------------------------------------------*/
	/*    UNRECOGNIZED RESPONSE PACKETS                              */
	/*---------------------------------------------------------------*/
      default:
	msg_num = ACSMSG_BAD_RESPONSE;
	acs_error_msg((&msg_num, NULL));
	acsRtn = STATUS_PROCESS_FAILURE;
      }
    acs_trace_exit(acsRtn);
    return acsRtn;
}
