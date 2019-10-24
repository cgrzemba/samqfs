#ifndef lint
static char SccsId[] = "@(#)td_set_cln_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_set_clean_resp
 *
 * Description:
 *      Decode the message content of a set_clean response packet.
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
td_set_clean_resp(VERSION version)
{
    unsigned short i;
    
    CSI_SET_CLEAN_RESPONSE *set_clean;

    /* version 1, 2, 3, 4 are the same */
    set_clean = (CSI_SET_CLEAN_RESPONSE *) td_msg_ptr;

    fputs(td_fix_portion, stdout);
    td_decode_resp_status(&set_clean->message_status);
    td_print_max_use(&set_clean->max_use);
    td_print_count(&set_clean->count);

    if (set_clean->count > 0) {
	fputs(td_var_portion, stdout);
	for (i = 0; i < set_clean->count; i++) {
	    td_print_volid(&(set_clean->volume_status[i].vol_id));
	    td_decode_resp_status(&(set_clean->volume_status[i].status));
	}
    }
}


