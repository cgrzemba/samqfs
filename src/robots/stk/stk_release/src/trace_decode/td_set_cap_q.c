#ifndef lint
static char SccsId[] = "@(#)td_set_cap_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_set_cap_req
 *
 * Description:
 *      Decode the message content of a set_cap request packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      NONE
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
td_set_cap_req(VERSION version)
{
    fputs(td_fix_portion, stdout);
    if (version == VERSION1) {
	/* version 1 does not have cap_mode */
	CSI_V1_SET_CAP_REQUEST *set_cap_ptr;
    
	set_cap_ptr = (CSI_V1_SET_CAP_REQUEST *) td_msg_ptr;
	td_print_cap_priority(&set_cap_ptr->cap_priority);
	td_print_count(&set_cap_ptr->count);
	if (set_cap_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_print_v0_capid_list(set_cap_ptr->cap_id, set_cap_ptr->count);
	}
    }
    else {
    /* version  2, 3 and 4 are the same */
	CSI_SET_CAP_REQUEST *set_cap_ptr;
    
	set_cap_ptr = (CSI_SET_CAP_REQUEST *) td_msg_ptr;
	td_print_cap_priority(&set_cap_ptr->cap_priority);
	td_print_cap_mode(&set_cap_ptr->cap_mode);
	td_print_count(&set_cap_ptr->count);
	if (set_cap_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_print_capid_list(set_cap_ptr->cap_id, set_cap_ptr->count);
	}
    }
}






