#ifndef lint
static char SccsId[] = "@(#)td_ack.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_decode_ack
 *
 * Description:
 *      Decode and output an acknowledge packet.
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

#include <stdio.h>
#include "td.h"

void
td_decode_ack(VERSION version)
{
    printf("\nACKNOWLEDGE RESPONSE:\n");
    if (version == VERSION0) {
	CSI_V0_ACKNOWLEDGE_RESPONSE  *ackptr;
	
	ackptr = (CSI_V0_ACKNOWLEDGE_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&ackptr->message_status);
	td_print_msgid(&ackptr->message_id);
    }
    else {
	/* version 1,2,3 and 4 */
	CSI_ACKNOWLEDGE_RESPONSE  *ackptr;
	
	ackptr = (CSI_ACKNOWLEDGE_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&ackptr->message_status);
	td_print_msgid(&ackptr->message_id);
    }
}


