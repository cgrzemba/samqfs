#ifndef lint
static char SccsId[] = "@(#)td_eject_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_eject_req
 *
 * Description:
 *      Decode the message content of a eject request packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_ptr, td_fix_portion, td_var_portion, td_xopts.
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

#include <stdio.h>
#include "csi.h"
#include "td.h"

void
td_eject_req(VERSION version)
{
    fputs(td_fix_portion, stdout);

    if (version == VERSION0) {
	CSI_V0_EJECT_REQUEST *eject_ptr;
	
	eject_ptr = (CSI_V0_EJECT_REQUEST *) td_msg_ptr;
	td_print_v0_capid(&eject_ptr->cap_id);
	td_print_count(&eject_ptr->count);

        fputs(td_var_portion, stdout);
	td_print_volid_list(eject_ptr->vol_id, eject_ptr->count);
    }
    else if (version == VERSION1) {
	if (td_xopts & RANGE) {
	    /* if RANGE is set, vol_range is used */
	    CSI_V1_EXT_EJECT_REQUEST *eject_ptr;
	
	    eject_ptr = (CSI_V1_EXT_EJECT_REQUEST *) td_msg_ptr;
	    td_print_v0_capid(&eject_ptr->cap_id);
	    td_print_count(&eject_ptr->count);

	    fputs(td_var_portion, stdout);
	    td_print_volrange_list(eject_ptr->vol_range, eject_ptr->count);
	}
	else {
	    /* vol_id is used */
	    CSI_V1_EJECT_REQUEST *eject_ptr;
	
	    eject_ptr = (CSI_V1_EJECT_REQUEST *) td_msg_ptr;
	    td_print_v0_capid(&eject_ptr->cap_id);
	    td_print_count(&eject_ptr->count);

	    fputs(td_var_portion, stdout);
	    td_print_volid_list(eject_ptr->vol_id, eject_ptr->count);
	}
    }
    else {
	/* the structure of version 2, 3, 4 are the same */
	if (td_xopts & RANGE) {
	    /* if RANGE is set, vol_range is used */
	    CSI_EXT_EJECT_REQUEST *eject_ptr;
	
	    eject_ptr = (CSI_EXT_EJECT_REQUEST *) td_msg_ptr;
	    td_print_capid(&eject_ptr->cap_id);
	    td_print_count(&eject_ptr->count);

	    fputs(td_var_portion, stdout);
	    td_print_volrange_list(eject_ptr->vol_range, eject_ptr->count);
	}
	else {
	    /* vol_id is used */
	    CSI_EJECT_REQUEST *eject_ptr;
	
	    eject_ptr = (CSI_EJECT_REQUEST *) td_msg_ptr;
	    td_print_capid(&eject_ptr->cap_id);
	    td_print_count(&eject_ptr->count);

	    fputs(td_var_portion, stdout);
	    td_print_volid_list(eject_ptr->vol_id, eject_ptr->count);
	}
    }
}



