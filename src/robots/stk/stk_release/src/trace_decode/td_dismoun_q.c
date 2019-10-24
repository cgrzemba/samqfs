#ifndef lint
static char SccsId[] = "@(#)td_dismoun_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_dismount_req
 *
 * Description:
 *      Decode the message content of a dismount request packet.
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
td_dismount_req(VERSION version)
{

    if (version == VERSION0) {
	CSI_V0_DISMOUNT_REQUEST *dismount_ptr;
	
	dismount_ptr = (CSI_V0_DISMOUNT_REQUEST *) td_msg_ptr;
	td_print_volid(&dismount_ptr->vol_id);
	td_print_driveid(&dismount_ptr->drive_id);
    }
    else {
	/* the structure of version 1, 2, 3, 4 are the same */
	CSI_DISMOUNT_REQUEST *dismount_ptr;
	
	dismount_ptr = (CSI_DISMOUNT_REQUEST *) td_msg_ptr;
	td_print_volid(&dismount_ptr->vol_id);
	td_print_driveid(&dismount_ptr->drive_id);
    }
}


