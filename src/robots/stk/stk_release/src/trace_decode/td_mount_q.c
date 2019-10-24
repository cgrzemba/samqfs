#ifndef lint
static char SccsId[] = "@(#)td_mount_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_mount_req
 *
 * Description:
 *      Decode the message content of a mount request packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_ptr, td_fix_portion, td_var_portion.
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
td_mount_req(VERSION version)
{
    fputs(td_fix_portion, stdout);
    if (version == VERSION0) {
	CSI_V0_MOUNT_REQUEST *mount_ptr;
	
	mount_ptr = (CSI_V0_MOUNT_REQUEST *) td_msg_ptr;
	td_print_volid(&mount_ptr->vol_id);
	td_print_count(&mount_ptr->count);
	fputs(td_var_portion, stdout);
	td_print_driveid_list(mount_ptr->drive_id, mount_ptr->count);
    }
    else {
	/* version 1, 2, 3 and 4 are the same */
	CSI_MOUNT_REQUEST *mount_ptr;
	
	mount_ptr = (CSI_MOUNT_REQUEST *) td_msg_ptr;
	td_print_volid(&mount_ptr->vol_id);
	td_print_count(&mount_ptr->count);
	fputs(td_var_portion, stdout);
	td_print_driveid_list(mount_ptr->drive_id, mount_ptr->count);
    }
}




