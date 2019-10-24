#ifndef lint
static char SccsId[] = "@(#)td_del_pl_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_delete_pool_req
 *
 * Description:
 *      Decode the message content of a delete_pool request packet.
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
td_delete_pool_req(VERSION version)
{
    /* version 1, 2, 3, 4 are the same */
    CSI_DELETE_POOL_REQUEST *delete_pool_ptr;

    fputs(td_fix_portion, stdout);
    delete_pool_ptr = (CSI_DELETE_POOL_REQUEST *) td_msg_ptr;
    td_print_count(&delete_pool_ptr->count);

    if (delete_pool_ptr->count > 0) {
	fputs(td_var_portion, stdout);
	td_print_poolid_list(delete_pool_ptr->pool_id, delete_pool_ptr->count);
    }
}





