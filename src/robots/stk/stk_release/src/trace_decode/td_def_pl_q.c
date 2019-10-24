#ifndef lint
static char SccsId[] = "@(#)td_def_pl_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_define_pool_req
 *
 * Description:
 *      Decode the message content of a define_pool request packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_ptr, td_fix_portion, td_var_portion.
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
td_define_pool_req(VERSION version)
{
    /* version 1, 2, 3, 4 are the same */
    CSI_DEFINE_POOL_REQUEST *define_pool_ptr;

    fputs(td_fix_portion, stdout);
    define_pool_ptr = (CSI_DEFINE_POOL_REQUEST *) td_msg_ptr;
    td_print_low_water_mark(&define_pool_ptr->low_water_mark);
    td_print_high_water_mark(&define_pool_ptr->high_water_mark);
    td_print_pool_attr(&define_pool_ptr->pool_attributes);
    td_print_count(&define_pool_ptr->count);

    if (define_pool_ptr->count > 0) {
	fputs(td_var_portion, stdout);
	td_print_poolid_list(define_pool_ptr->pool_id, define_pool_ptr->count);
    }
}





