#ifndef lint
static char SccsId[] = "@(#)td_enter_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_enter_req
 *
 * Description:
 *      Decode the message content of an enter request packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_ptr, td_mopts, td_xopts.
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
td_enter_req(VERSION version)
{
    if (td_mopts & EXTENDED && td_xopts & VIRTUAL) {
	/* venter request */
	td_venter_req(version);
	return;
    }
	
    if (version == VERSION0) {
	CSI_V0_ENTER_REQUEST *enter_ptr;
	
	enter_ptr = (CSI_V0_ENTER_REQUEST *) td_msg_ptr;
	td_print_v0_capid(&enter_ptr->cap_id);
    }
    if (version == VERSION1) {
	CSI_V1_ENTER_REQUEST *enter_ptr;
	
	enter_ptr = (CSI_V1_ENTER_REQUEST *) td_msg_ptr;
	td_print_v0_capid(&enter_ptr->cap_id);
    }
    else {
	/* version 2, 3, 4 are the same */
	CSI_ENTER_REQUEST *enter_ptr;
	
	enter_ptr = (CSI_ENTER_REQUEST *) td_msg_ptr;
	td_print_capid(&enter_ptr->cap_id);
    }
}

/*
 * Decode a venter request.
 */
void
td_venter_req(VERSION version)
{
    fputs("  Fixed Portion (venter):\n", stdout);
    if (version == VERSION1) {
	CSI_V1_VENTER_REQUEST *venter_ptr;
	
	venter_ptr = (CSI_V1_VENTER_REQUEST *) td_msg_ptr;
	td_print_v0_capid(&venter_ptr->cap_id);
	td_print_count(&venter_ptr->count);
	fputs(td_var_portion, stdout);
	td_print_volid_list(venter_ptr->vol_id, venter_ptr->count);
    }
    else {
	/* version 2, 3, 4 are the same */
	CSI_VENTER_REQUEST *venter_ptr;
	
	venter_ptr = (CSI_VENTER_REQUEST *) td_msg_ptr;
	td_print_capid(&venter_ptr->cap_id);
	td_print_count(&venter_ptr->count);
	fputs(td_var_portion, stdout);
	td_print_volid_list(venter_ptr->vol_id, venter_ptr->count);
    }

}


