#ifndef lint
static char SccsId[] = "@(#)td_chk_reg_p.c	2.2 10/25/01 ";
#endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_check_registration_resp
 *
 * Description:
 *      Decode the message content of a check_registration response packet.
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
td_check_registration_resp(VERSION version)
{
	/* version 4 */
	CSI_CHECK_REGISTRATION_RESPONSE *check_registration_ptr;
	
	check_registration_ptr = (CSI_CHECK_REGISTRATION_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&check_registration_ptr->message_status);
	td_show_register_status(&check_registration_ptr->event_register_status);
}
