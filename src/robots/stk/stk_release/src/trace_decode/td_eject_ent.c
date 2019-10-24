#ifndef lint
static char SccsId[] = "@(#)td_eject_ent.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_eject_enter
 *
 * Description:
 *      Decode the message content of a eject-enter response packet,
 *      enter response packet or eject respose packet.
 *      (eject-enter packet, enter response packet and eject response
 *      packet have the same structure)
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

#include <stdio.h>
#include "csi.h"
#include "td.h"

void
td_eject_enter(VERSION version)
{
    fputs(td_fix_portion, stdout);

    if (version == VERSION0) {
	CSI_V0_EJECT_ENTER *eject_enter_ptr;
	
	eject_enter_ptr = (CSI_V0_EJECT_ENTER *) td_msg_ptr;
	td_decode_resp_status(&eject_enter_ptr->message_status);
	td_print_v0_capid(&eject_enter_ptr->cap_id);
	td_print_count(&eject_enter_ptr->count);

	if (eject_enter_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_decode_vol_status_list(eject_enter_ptr->volume_status,
				      eject_enter_ptr->count);
	}
    }
    else if (version == VERSION1) {
	CSI_V1_EJECT_ENTER *eject_enter_ptr;
	
	eject_enter_ptr = (CSI_V1_EJECT_ENTER *) td_msg_ptr;
	td_decode_resp_status(&eject_enter_ptr->message_status);
	td_print_v0_capid(&eject_enter_ptr->cap_id);
	td_print_count(&eject_enter_ptr->count);

	if (eject_enter_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_decode_vol_status_list(eject_enter_ptr->volume_status,
				      eject_enter_ptr->count);
	}
    }
    else {
	/* version 2, 3 and 4 */
	CSI_EJECT_ENTER *eject_enter_ptr;
	
	eject_enter_ptr = (CSI_EJECT_ENTER *) td_msg_ptr;
	td_decode_resp_status(&eject_enter_ptr->message_status);
	td_print_capid(&eject_enter_ptr->cap_id);
	td_print_count(&eject_enter_ptr->count);

	if (eject_enter_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_decode_vol_status_list(eject_enter_ptr->volume_status,
				      eject_enter_ptr->count);
	}
    }
}



