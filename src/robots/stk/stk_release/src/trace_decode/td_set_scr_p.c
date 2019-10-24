#ifndef lint
static char SccsId[] = "@(#)td_set_scr_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_set_scratch_resp
 *
 * Description:
 *      Decode the message content of a set_scratch response packet.
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

#include "td.h"

void
td_set_scratch_resp(VERSION version)
{
    unsigned short i;

    /* version 1, 2, 3, 4 are the same */
    CSI_SET_SCRATCH_RESPONSE *set_scratch;
    
    set_scratch = (CSI_SET_SCRATCH_RESPONSE *) td_msg_ptr;

    fputs(td_fix_portion, stdout);
    td_decode_resp_status(&set_scratch->message_status);
    td_print_poolid(&set_scratch->pool_id);
    td_print_count(&set_scratch->count);

    if (set_scratch->count > 0) {
	fputs(td_var_portion, stdout);
	for (i = 0; i < set_scratch->count; i++) {
	    td_print_volid(&(set_scratch->scratch_status[i].vol_id));
	    td_decode_resp_status(&(set_scratch->scratch_status[i].status));
	}
    }
}


