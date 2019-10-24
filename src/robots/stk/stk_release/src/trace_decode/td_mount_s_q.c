#ifndef lint
static char SccsId[] = "@(#)td_mount_s_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_mount_scratch_req
 *
 * Description:
 *      Decode the message content of a mount_scratch request packet.
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
td_mount_scratch_req(VERSION version)
{
    fputs(td_fix_portion, stdout);
    if (version >= VERSION1 && version <= VERSION3) {
	/* version 1, 2 and 3 are the same */
	CSI_V2_MOUNT_SCRATCH_REQUEST *mount_scratch_ptr;
	
	mount_scratch_ptr = (CSI_V2_MOUNT_SCRATCH_REQUEST *) td_msg_ptr;
	td_print_poolid(&mount_scratch_ptr->pool_id);
	td_print_count(&mount_scratch_ptr->count);
	fputs(td_var_portion, stdout);
	td_print_driveid_list(mount_scratch_ptr->drive_id,
			      mount_scratch_ptr->count);
    }
    else {
	/* version 4 */
	CSI_MOUNT_SCRATCH_REQUEST *mount_scratch_ptr;
	
	mount_scratch_ptr = (CSI_MOUNT_SCRATCH_REQUEST *) td_msg_ptr;
	td_print_poolid(&mount_scratch_ptr->pool_id);
	td_print_media_type(&mount_scratch_ptr->media_type);
	td_print_count(&mount_scratch_ptr->count);
	fputs(td_var_portion, stdout);
	td_print_driveid_list(mount_scratch_ptr->drive_id,
			      mount_scratch_ptr->count);
    }
}


