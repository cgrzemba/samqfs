#ifndef lint
static char SccsId[] = "@(#)td_lock_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_lock_req
 *
 * Description:
 *      Decode the message content of a lock request packet.
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
td_lock_req(VERSION version)
{
    /* version 1, 2, 3 and 4 are the same */
    CSI_LOCK_REQUEST *lock_ptr;

    fputs(td_fix_portion, stdout);
    lock_ptr = (CSI_LOCK_REQUEST *) td_msg_ptr;
    td_print_type(&lock_ptr->type);
    td_print_count(&lock_ptr->count);
    
    fputs(td_var_portion, stdout);
    if (lock_ptr->type == TYPE_VOLUME)
	td_print_volid_list(lock_ptr->identifier.vol_id, lock_ptr->count);
    else if (lock_ptr->type == TYPE_DRIVE)
	td_print_driveid_list(lock_ptr->identifier.drive_id, lock_ptr->count);
    else
	fputs("Invalid type, unable to decode!\n", stdout);
}











