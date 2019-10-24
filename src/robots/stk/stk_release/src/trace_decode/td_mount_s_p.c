#ifndef lint
static char SccsId[] = "@(#)td_mount_s_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_mount_scratch_resp
 *
 * Description:
 *      Decode the message content of a mount_scratch response packet.
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

#include "csi.h"
#include "td.h"

void
td_mount_scratch_resp(VERSION version)
{
    /* the structure of version 1, 2, 3, 4 are the same */
    CSI_MOUNT_SCRATCH_RESPONSE *mount_scratch_ptr;
	
    mount_scratch_ptr = (CSI_MOUNT_SCRATCH_RESPONSE *) td_msg_ptr;
    td_decode_resp_status(&mount_scratch_ptr->message_status);
    td_print_poolid(&mount_scratch_ptr->pool_id);
    td_print_driveid(&mount_scratch_ptr->drive_id);
    td_print_volid(&mount_scratch_ptr->vol_id);
}



