#ifndef lint
static char SccsId[] = "@(#)td_def_pl_p.c	1.1 12/3/93 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_define_pool_resp
 *
 * Description:
 *      Decode the message content of a define_pool response packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_buf, td_fix_portion, td_var_portion.
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
td_define_pool_resp(VERSION version)
{
    unsigned short i;
    
    /* version 1, 2, 3, 4 are the same */
    CSI_DEFINE_POOL_RESPONSE *define_pool_ptr;
    
    define_pool_ptr = (CSI_DEFINE_POOL_RESPONSE *) td_msg_buf;

    fputs(td_fix_portion, stdout);
    td_decode_resp_status(&define_pool_ptr->message_status);
    td_print_low_water_mark(&define_pool_ptr->low_water_mark);
    td_print_high_water_mark(&define_pool_ptr->high_water_mark);
    td_print_pool_attr(&define_pool_ptr->pool_attributes);
    td_print_count(&define_pool_ptr->count);

    if (define_pool_ptr->count > 0) {
	fputs(td_var_portion, stdout);
	for (i = 0; i < define_pool_ptr->count; i++) {
	    td_print_poolid(&(define_pool_ptr->pool_status[i].pool_id));
	    td_decode_resp_status(&(define_pool_ptr->pool_status[i].status));
	}
    }
}



