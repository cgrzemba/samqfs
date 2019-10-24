#ifndef lint
static char SccsId[] = "@(#)td_mount_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_mount_resp
 *
 * Description:
 *      Decode the message content of a mount response packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_buf, td_fix_portion, td_var_portion.
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

#include "csi.h"
#include "td.h"

void
td_mount_resp(VERSION version)
{
    if (version == VERSION0) {
	CSI_V0_MOUNT_RESPONSE *mount_ptr;
	
	mount_ptr = (CSI_V0_MOUNT_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&mount_ptr->message_status);
	td_print_volid(&mount_ptr->vol_id);
	td_print_driveid(&mount_ptr->drive_id);
    }
    else {
	/* version 1, 2, 3, 4 are the same */
	CSI_MOUNT_RESPONSE *mount_ptr;
	
	mount_ptr = (CSI_MOUNT_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&mount_ptr->message_status);
	td_print_volid(&mount_ptr->vol_id);
	td_print_driveid(&mount_ptr->drive_id);
    }
}



