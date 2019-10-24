#ifndef lint
static char SccsId[] = "@(#)td_dismoun_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_dismount_resp
 *
 * Description:
 *      Decode the message content of a dismount response packet.
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
 *      M. H. Shum          10-Sep-1993     Original.
 *
 */

#include "td.h"

void
td_dismount_resp(VERSION version)
{
    if (version == VERSION0) {
	CSI_V0_DISMOUNT_RESPONSE *dismount_ptr;
	
	dismount_ptr = (CSI_V0_DISMOUNT_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&dismount_ptr->message_status);
	td_print_volid(&dismount_ptr->vol_id);
	td_print_driveid(&dismount_ptr->drive_id);
    }
    else {
	/* version 1, 2, 3, 4 are the same */
	CSI_DISMOUNT_RESPONSE *dismount_ptr;
	
	dismount_ptr = (CSI_DISMOUNT_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&dismount_ptr->message_status);
	td_print_volid(&dismount_ptr->vol_id);
	td_print_driveid(&dismount_ptr->drive_id);
    }
}



