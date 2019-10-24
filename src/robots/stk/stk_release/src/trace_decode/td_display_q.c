#ifndef lint
static char SccsId[] = "@(#)td_display_q.c	2.2 10/25/01 ";
#endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_display_req
 *
 * Description:
 *      Decode the message content of an display request packet.
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
 *      S. L. Siao          13-Nov-2001     Original.
 *
 */

#include "csi.h"
#include "td.h"

void
td_display_req(VERSION version)
{
    CSI_DISPLAY_REQUEST *display_ptr;
    
    display_ptr = (CSI_DISPLAY_REQUEST *) td_msg_ptr;
    td_print_type(&display_ptr->display_type);
    td_print_xml_data(&display_ptr->display_xml_data);
}

