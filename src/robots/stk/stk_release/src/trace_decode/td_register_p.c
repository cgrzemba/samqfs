#ifndef lint
static char SccsId[] = "@(#)td_unregister_p.c	2.2 10/25/01 ";
#endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_register_resp
 *
 * Description:
 *      Decode the message content of a register response packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_ptr.
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *
 * Revision History:
 *
 *      S. L. Siao          25-Oct-2001     Original.
 *      Wipro(Hemendra)     18-Jun-2004     Support for mount/ dismount events (for CIM)
 *					    Modified td_register_resp
 *					    Added td_show_drive_activity_status
 *					    Also added support for EVENT_REPLY_CLIENT_CHECK
 *
 */

#include "td.h"
#include "db_defs.h"


void
td_register_resp(VERSION version)
{
	/* version 4 */
	CSI_REGISTER_RESPONSE *register_ptr;
	
	register_ptr = (CSI_REGISTER_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&register_ptr->message_status);
	td_print_event_sequence(&register_ptr->event_sequence);

	if ((register_ptr->event_reply_type == EVENT_REPLY_REGISTER) ||
	   (register_ptr->event_reply_type == EVENT_REPLY_UNREGISTER) ||
	   (register_ptr->event_reply_type == EVENT_REPLY_SUPERCEDED) ||
	   (register_ptr->event_reply_type == EVENT_REPLY_SHUTDOWN)) {
	    
	    td_print_reply_type(&register_ptr->event_reply_type);

            td_show_register_status(&register_ptr->event.event_register_status);
	}

	else if (register_ptr->event_reply_type == EVENT_REPLY_VOLUME) {
	    
	    td_print_reply_type(&register_ptr->event_reply_type);

	    td_print_volume_event_type(&register_ptr->event.
				       event_volume_status.event_type);
	    td_print_volid(&register_ptr->event.event_volume_status.vol_id);
        }

	else if (register_ptr->event_reply_type == EVENT_REPLY_RESOURCE) {

	    td_print_reply_type(&register_ptr->event_reply_type);

	    td_show_resource_status(&register_ptr->event.event_resource_status);
	}

	else if (register_ptr->event_reply_type == EVENT_REPLY_CLIENT_CHECK) {

	    td_print_reply_type(&register_ptr->event_reply_type);
	}

	else if (register_ptr->event_reply_type == EVENT_REPLY_DRIVE_ACTIVITY) {

	    td_print_reply_type(&register_ptr->event_reply_type);

	    td_show_drive_activity_status(&register_ptr->event.event_drive_status);
	}
}

void
td_show_register_status(EVENT_REGISTER_STATUS *event_register_status)
{
    int i;
    unsigned short count;
    EVENT_CLASS_REGISTER_RETURN register_return;

    count = event_register_status->count;
    td_print_register_status(event_register_status, count);

}

void
td_show_resource_status(EVENT_RESOURCE_STATUS *event_resource_status)
{

    td_print_resource_type(&event_resource_status->resource_type);
    td_print_resource_identifier(event_resource_status->resource_type, 
				 &event_resource_status->resource_identifier);

    td_print_resource_event(&event_resource_status->resource_event);

    td_print_resource_data_type(&event_resource_status->resource_data_type);

    switch(event_resource_status->resource_data_type) {
	case SENSE_TYPE_HLI:
	    td_print_type_hli(&event_resource_status->resource_data.sense_hli);
	    break;
	case SENSE_TYPE_SCSI:
	    td_print_type_scsi(&event_resource_status->resource_data.sense_scsi);
	    break;
	case SENSE_TYPE_FSC:
	    td_print_type_fsc(&event_resource_status->resource_data.sense_fsc);
	    break;
	case RESOURCE_CHANGE_SERIAL_NUM:
	    td_print_type_serial_num(&event_resource_status->resource_data.serial_num);
	    break;
	case RESOURCE_CHANGE_LSM_TYPE:
	    td_print_type_lsm(&event_resource_status->resource_data.lsm_type);
	    break;
	case RESOURCE_CHANGE_DRIVE_TYPE:
	    td_print_drive_type(&event_resource_status->resource_data.drive_type);
	    break;
	default:
	    td_print("resource_data_type", &event_resource_status->resource_data_type,
		     sizeof(event_resource_status->resource_data_type), "Invalid value");
	    /* shouldn't get here! */
    }
}

void
td_show_drive_activity_status(EVENT_DRIVE_STATUS *event_drive_status)
{
    td_print_drive_activity_type(&event_drive_status->event_type);

    td_print_resource_data_type(&event_drive_status->resource_data_type);

    if (event_drive_status->resource_data_type == DRIVE_ACTIVITY_DATA_TYPE)
	td_print_drive_activity_data(&event_drive_status->resource_data.drive_activity_data);
    else
        td_print("drive_activity_data_type", &event_drive_status->resource_data_type,
                 sizeof(event_drive_status->resource_data_type), "Invalid value");
        /* shouldn't get here! */
}
