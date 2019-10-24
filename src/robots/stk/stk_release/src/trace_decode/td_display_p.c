#ifndef lint
static char SccsId[] = "@(#)td_display_p.c	2.2 10/25/01 ";
#endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_display_resp
 *
 * Description:
 *      Decode the message content of a display response packet.
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
 *      S. L. Siao          13-Nov-2001     Original.
 *      S. L. Siao          26-Mar-2002     Added memset to clear buffer to
 *                                          avoid garbage remaining for next
 *                                          call to this function.
 *
 */

#include "td.h"
#include "db_defs.h"


void
td_display_resp(VERSION version)
{
	/* version 4 */
	CSI_DISPLAY_RESPONSE *display_ptr;
	
	display_ptr = (CSI_DISPLAY_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&display_ptr->message_status);

        td_print_type(&display_ptr->display_type);
        td_print_xml_data(&display_ptr->display_xml_data);
	memset(&display_ptr->display_xml_data.xml_data, 0, 
	       sizeof(DISPLAY_XML_DATA));
}
