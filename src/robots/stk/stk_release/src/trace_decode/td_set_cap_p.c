#ifndef lint
static char SccsId[] = "@(#)td_set_cap_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_set_cap_resp
 *
 * Description:
 *      Decode the message content of a set_cap response packet.
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
td_set_cap_resp(VERSION version)
{
    unsigned short i;

    fputs(td_fix_portion, stdout);
    if (version == VERSION1) {
	/* version 1 does not have cap_mode */
	CSI_V1_SET_CAP_RESPONSE *set_cap_ptr;
	
	set_cap_ptr = (CSI_V1_SET_CAP_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&set_cap_ptr->message_status);
	td_print_cap_priority(&set_cap_ptr->cap_priority);
	td_print_count(&set_cap_ptr->count);

	if (set_cap_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    for (i = 0; i < set_cap_ptr->count; i++) {
		td_print_v0_capid(&set_cap_ptr->set_cap_status[i].cap_id);
		td_decode_resp_status(&set_cap_ptr->set_cap_status[i].status);
	    }
	}
    }
    else {
	/* version 2, 3, 4 are the same */
	CSI_SET_CAP_RESPONSE *set_cap_ptr;
	
	set_cap_ptr = (CSI_SET_CAP_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&set_cap_ptr->message_status);
	td_print_cap_priority(&set_cap_ptr->cap_priority);
	td_print_cap_mode(&set_cap_ptr->cap_mode);
	td_print_count(&set_cap_ptr->count);

	if(set_cap_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    for (i = 0; i < set_cap_ptr->count; i++) {
		td_print_capid(&set_cap_ptr->set_cap_status[i].cap_id);
		td_decode_resp_status(&set_cap_ptr->set_cap_status[i].status);
	    }
	}
    }
}




