#ifndef lint
static char SccsId[] = "@(#)td_chk_reg_q.c	2.2 10/25/01 ";
#endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_check_registration_req
 *
 * Description:
 *      Decode the message content of an check_registration request packet.
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
td_check_registration_req(VERSION version)
{
    CSI_CHECK_REGISTRATION_REQUEST *check_registration_ptr;
    
    check_registration_ptr = (CSI_CHECK_REGISTRATION_REQUEST *) td_msg_ptr;
    td_print_registration_id(&check_registration_ptr->registration_id);
}

