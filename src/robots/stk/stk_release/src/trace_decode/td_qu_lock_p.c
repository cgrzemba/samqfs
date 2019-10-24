#ifndef lint
static char SccsId[] = "@(#)td_qu_lock_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_query_lock_resp
 *
 * Description:
 *      Decode the message content of a query_lock response packet.
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

static void st_print_lock_dur(unsigned long *);
static void st_print_lock_pend(unsigned int *);

void
td_query_lock_resp(VERSION version)
{
    /* version 1, 2, 3, 4 are the same */
    CSI_QUERY_LOCK_RESPONSE *qu_lock_ptr;
    unsigned short i;
    
    qu_lock_ptr = (CSI_QUERY_LOCK_RESPONSE *) td_msg_ptr;

    fputs(td_fix_portion, stdout);
    td_decode_resp_status(&qu_lock_ptr->message_status);

    td_print_type(&qu_lock_ptr->type);
    td_print_count(&qu_lock_ptr->count);

    if (qu_lock_ptr->count > 0) {
	fputs(td_var_portion, stdout);
	if (qu_lock_ptr->type == TYPE_VOLUME) {
	    QL_VOL_STATUS *vol_status = qu_lock_ptr->identifier_status.volume_status;
	    
	    for (i = 0; i < qu_lock_ptr->count; i++, vol_status++) {
		td_print_volid(&vol_status->vol_id);
		td_print_lockid(&vol_status->lock_id);
		st_print_lock_dur(&vol_status->lock_duration);
		st_print_lock_pend(&vol_status->locks_pending);
		td_print_userid(&vol_status->user_id);
		td_print_status(&vol_status->status);
	    }
	}
	else if (qu_lock_ptr->type == TYPE_DRIVE) {
	    QL_DRV_STATUS *drive_status = qu_lock_ptr->identifier_status.drive_status;
	    
	    for (i = 0; i < qu_lock_ptr->count; i++, drive_status++) {
		td_print_driveid(&drive_status->drive_id);
		td_print_lockid(&drive_status->lock_id);
		st_print_lock_dur(&drive_status->lock_duration);
		st_print_lock_pend(&drive_status->locks_pending);
		td_print_userid(&drive_status->user_id);
		td_print_status(&drive_status->status);
	    }
	}
	else
	    fputs("Invalid type, unable to decode!\n", stdout);
    }
}

/*
 * Decode lock duration.
 */
static void
st_print_lock_dur(unsigned long *dur)
{
    td_print("lock_duration", dur, sizeof(unsigned long), td_ultoa(*dur));
}

/*
 * Decode lock pending.
 */
static void
st_print_lock_pend(unsigned int *pend)
{
    td_print("lock_pending", pend, sizeof(unsigned int), td_utoa(*pend));
}



