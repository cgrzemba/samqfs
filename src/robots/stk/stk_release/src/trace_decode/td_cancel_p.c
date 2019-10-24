#ifndef lint
static char SccsId[] = "@(#)td_cancel_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_cancel_resp
 *
 * Description:
 *      Decode the message content of a cancel response packet.
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
td_cancel_resp(VERSION version)
{

    if (version == VERSION0) {
	CSI_V0_CANCEL_RESPONSE *cancel_ptr;
	
	cancel_ptr = (CSI_V0_CANCEL_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&cancel_ptr->message_status);
	td_print_msgid(&cancel_ptr->request);
    }
    else {
	/* version 1, 2, 3, 4 are the same */
	CSI_CANCEL_RESPONSE *cancel_ptr;
	
	cancel_ptr = (CSI_CANCEL_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&cancel_ptr->message_status);
	td_print_msgid(&cancel_ptr->request);
    }
}


