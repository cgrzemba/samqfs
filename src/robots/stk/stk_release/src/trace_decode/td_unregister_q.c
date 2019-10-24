#ifndef lint
static char SccsId[] = "@(#)td_unregister_q.c	2.2 10/25/01 ";
#endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_unregister_req
 *
 * Description:
 *      Decode the message content of an unregister request packet.
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
td_unregister_req(VERSION version)
{
    CSI_UNREGISTER_REQUEST *unregister_ptr;
    
    unregister_ptr = (CSI_UNREGISTER_REQUEST *) td_msg_ptr;
    td_print_registration_id(&unregister_ptr->registration_id);
    td_print_count(&unregister_ptr->count);
    if (unregister_ptr->count > 0) {
	fputs(td_var_portion, stdout);
	td_print_eventClass_list(&unregister_ptr->eventClass[0], unregister_ptr->count);
    }
}

