#ifndef lint
static char SccsId[] = "@(#)td_unregister_p.c	2.2 10/25/01 ";
#endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_unregister_resp
 *
 * Description:
 *      Decode the message content of a unregister response packet.
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
 *      S. L. Siao          25-Oct-2001     Original.
 *
 */

#include "td.h"

void
td_unregister_resp(VERSION version)
{
	/* version 4 */
	CSI_UNREGISTER_RESPONSE *unregister_ptr;
	
	unregister_ptr = (CSI_UNREGISTER_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&unregister_ptr->message_status);
	td_show_register_status(&unregister_ptr->event_register_status);
}



