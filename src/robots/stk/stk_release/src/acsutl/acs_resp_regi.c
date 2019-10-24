#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsutl/csrc/acs_resp_regi/2.2 %";
#endif
/*
 *
 *                            (c) Copyright (2001)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_response_registeri()
 *
 * Description:
 *   This routine takes a intermediate register response packet and 
 *   takes it apart, returning via bufferForClient an 
 *   ACS_REGISTER_"XXX"_RESPONSE structure filled in with the data 
 *   from the response packet.
 *
 * Return Values:
 *    STATUS_SUCCESS.
 *
 * Parameters:
 *    bufferForClient - pointer to space for the
 *                      ACS_REGISTER_"XXX"_RESPONSE data.
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
 *    Scott Siao       16-Oct-2001    Original.
 *    Wipro (Subhash)  04-Jun-2004    Modified to support 
 *                                    EVENT_REPLY_DRIVE_ACTIVITY.
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
#define SELF  "acs_reg_int_response"
#undef ACSMOD
#define ACSMOD 206

/*===================================================================*/
/*                                                                   */
/*                acs_register_int_response                             */
/*                                                                   */
/*===================================================================*/

STATUS acs_register_int_response
  ( char *bufferForClient,
    ALIGNED_BYTES rspBfr) {
    COPYRIGHT;
    unsigned int i, cnt;
    STATUS acsRtn;
    ACSFMTREC toClient;
    SVRFMTREC frmServer;

    acs_trace_entry ();

    toClient.haRR = (ACS_REGISTER_RESPONSE *) bufferForClient;

    frmServer.hRR = (REGISTER_RESPONSE *) rspBfr;

    memset ((char *)toClient.haRR, '\00', sizeof (ACS_REGISTER_RESPONSE));

    toClient.haRR->register_status
	= frmServer.hRR->message_status.status;
    toClient.haRR->event_reply_type
        = frmServer.hRR->event_reply_type;
    toClient.haRR->event_sequence
        = frmServer.hRR->event_sequence;
    if ((toClient.haRR->event_reply_type == EVENT_REPLY_REGISTER) ||
        (toClient.haRR->event_reply_type == EVENT_REPLY_UNREGISTER) ||
        (toClient.haRR->event_reply_type == EVENT_REPLY_SUPERCEDED) ||
        (toClient.haRR->event_reply_type == EVENT_REPLY_CLIENT_CHECK) ||
        (toClient.haRR->event_reply_type == EVENT_REPLY_SHUTDOWN)) {
            toClient.haRR->event.event_register_status.registration_id 
               = frmServer.hRR->event.event_register_status.registration_id;
            toClient.haRR->event.event_register_status.count 
               = frmServer.hRR->event.event_register_status.count;
	    cnt = toClient.haRR->event.event_register_status.count;
            for (i=0; i< cnt; i++) {
		toClient.haRR->event.event_register_status.
		  register_status[i].event_class
		    =frmServer.hRR->event.event_register_status.
		      register_status[i].event_class;
		toClient.haRR->event.event_register_status.
		  register_status[i].register_return
		    =frmServer.hRR->event.event_register_status.
		      register_status[i].register_return;
            }
	}
    else if(toClient.haRR->event_reply_type == EVENT_REPLY_VOLUME) {
	toClient.haRR->event.event_volume_status.event_type
	    =frmServer.hRR->event.event_volume_status.event_type;
	toClient.haRR->event.event_volume_status.vol_id
	    =frmServer.hRR->event.event_volume_status.vol_id;
    }
    else if(toClient.haRR->event_reply_type == EVENT_REPLY_RESOURCE) {
	toClient.haRR->event.event_resource_status.resource_type
	    =frmServer.hRR->event.event_resource_status.resource_type;
	toClient.haRR->event.event_resource_status.resource_identifier
	    =frmServer.hRR->event.event_resource_status.resource_identifier;
	toClient.haRR->event.event_resource_status.resource_event
	    =frmServer.hRR->event.event_resource_status.resource_event;
	toClient.haRR->event.event_resource_status.resource_data_type
	    =frmServer.hRR->event.event_resource_status.resource_data_type;

	if(toClient.haRR->event.event_resource_status.resource_data_type 
	  == SENSE_TYPE_HLI) {
	    toClient.haRR->event.event_resource_status.resource_data.sense_hli.category =
	      frmServer.hRR->event.event_resource_status.resource_data.sense_hli.category;
	    toClient.haRR->event.event_resource_status.resource_data.sense_hli.code = 
	      frmServer.hRR->event.event_resource_status.resource_data.sense_hli.code;
	    }
	else if(toClient.haRR->event.event_resource_status.resource_data_type 
	  == SENSE_TYPE_SCSI) {
	    toClient.haRR->event.event_resource_status.resource_data.sense_scsi.sense_key =
	      frmServer.hRR->event.event_resource_status.resource_data.sense_scsi.sense_key;
	    toClient.haRR->event.event_resource_status.resource_data.sense_scsi.asc =
	      frmServer.hRR->event.event_resource_status.resource_data.sense_scsi.asc;
	    toClient.haRR->event.event_resource_status.resource_data.sense_scsi.ascq = 
	      frmServer.hRR->event.event_resource_status.resource_data.sense_scsi.ascq;
	    }
	else if(toClient.haRR->event.event_resource_status.resource_data_type 
	  == SENSE_TYPE_FSC) {
	    toClient.haRR->event.event_resource_status.resource_data.sense_fsc 
	      = frmServer.hRR->event.event_resource_status.resource_data.sense_fsc;
	    }
	else if(toClient.haRR->event.event_resource_status.resource_data_type 
	  == RESOURCE_CHANGE_SERIAL_NUM) {
	    toClient.haRR->event.event_resource_status.resource_data.serial_num 
	      = frmServer.hRR->event.event_resource_status.resource_data.serial_num;
	    }
	else if(toClient.haRR->event.event_resource_status.resource_data_type 
	  == RESOURCE_CHANGE_LSM_TYPE) {
	    toClient.haRR->event.event_resource_status.resource_data.lsm_type 
	      = frmServer.hRR->event.event_resource_status.resource_data.lsm_type;
	    }
	else if(toClient.haRR->event.event_resource_status.resource_data_type 
	  == RESOURCE_CHANGE_DRIVE_TYPE) {
	    toClient.haRR->event.event_resource_status.resource_data.drive_type 
	      = frmServer.hRR->event.event_resource_status.resource_data.drive_type;
	    }
    } else if(toClient.haRR->event_reply_type == EVENT_REPLY_DRIVE_ACTIVITY) {
        toClient.haRR->event.event_drive_status.event_type 
            =frmServer.hRR->event.event_drive_status.event_type;
        toClient.haRR->event.event_drive_status.resource_data.
                                   drive_activity_data.start_time
            =frmServer.hRR->event.event_drive_status.resource_data.
                                   drive_activity_data.start_time;
        toClient.haRR->event.event_drive_status.resource_data.
                                   drive_activity_data.completion_time
            =frmServer.hRR->event.event_drive_status.resource_data.
                                   drive_activity_data.completion_time;
        toClient.haRR->event.event_drive_status.resource_data.
                                   drive_activity_data.vol_id
            =frmServer.hRR->event.event_drive_status.resource_data.
                                   drive_activity_data.vol_id;
        toClient.haRR->event.event_drive_status.resource_data.
                                   drive_activity_data.volume_type
            =frmServer.hRR->event.event_drive_status.resource_data.
                                   drive_activity_data.volume_type;

        toClient.haRR->event.event_drive_status.resource_data.
                             drive_activity_data.drive_id.panel_id.lsm_id.acs 
            =frmServer.hRR->event.event_drive_status.resource_data.
                             drive_activity_data.drive_id.panel_id.lsm_id.acs;
        toClient.haRR->event.event_drive_status.resource_data.
                             drive_activity_data.drive_id.panel_id.lsm_id.lsm
            =frmServer.hRR->event.event_drive_status.resource_data.
                             drive_activity_data.drive_id.panel_id.lsm_id.lsm;
        toClient.haRR->event.event_drive_status.resource_data.
                             drive_activity_data.drive_id.panel_id.panel
            =frmServer.hRR->event.event_drive_status.resource_data.
                             drive_activity_data.drive_id.panel_id.panel;
        toClient.haRR->event.event_drive_status.resource_data.
                             drive_activity_data.drive_id.drive
            =frmServer.hRR->event.event_drive_status.resource_data.
                             drive_activity_data.drive_id.drive;

        toClient.haRR->event.event_drive_status.resource_data.
                             drive_activity_data.pool_id.pool
            =frmServer.hRR->event.event_drive_status.resource_data.
                             drive_activity_data.pool_id.pool;

        toClient.haRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.panel_id.lsm_id.acs
            =frmServer.hRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.panel_id.lsm_id.acs;
        toClient.haRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.panel_id.lsm_id.lsm
            =frmServer.hRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.panel_id.lsm_id.lsm;
        toClient.haRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.panel_id.panel
            =frmServer.hRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.panel_id.panel;
        toClient.haRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.row
            =frmServer.hRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.row;
        toClient.haRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.col
            =frmServer.hRR->event.event_drive_status.resource_data.
                          drive_activity_data.home_location.col;
    }

    acsRtn = STATUS_SUCCESS;

    acs_trace_exit (acsRtn);

    return acsRtn;

}

