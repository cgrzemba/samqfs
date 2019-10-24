#ifndef lint
static char SccsId[] = "@(#)td_register_q.c	2.2 10/25/01 ";
#endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_register_req
 *
 * Description:
 *      Decode the message content of an register request packet.
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
 *      S. L. Siao          25-Oct-2001     Original.
 *
 */

#include "csi.h"
#include "td.h"

void
td_register_req(VERSION version)
{
    CSI_REGISTER_REQUEST *register_ptr;
    
    register_ptr = (CSI_REGISTER_REQUEST *) td_msg_ptr;
    td_print_registration_id(&register_ptr->registration_id);
    td_print_count(&register_ptr->count);
    if (register_ptr->count > 0) {
	fputs(td_var_portion, stdout);
	td_print_eventClass_list(&register_ptr->eventClass[0], register_ptr->count);
    }
}

