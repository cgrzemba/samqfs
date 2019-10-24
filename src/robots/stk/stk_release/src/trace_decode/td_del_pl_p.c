#ifndef lint
static char SccsId[] = "@(#)td_del_pl_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_delete_pool_resp
 *
 * Description:
 *      Decode the message content of a delete_pool response packet.
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
td_delete_pool_resp(VERSION version)
{
    unsigned short i;
    
    /* version 1, 2, 3, 4 are the same */
    CSI_DELETE_POOL_RESPONSE *delete_pool_ptr;
    
    delete_pool_ptr = (CSI_DELETE_POOL_RESPONSE *) td_msg_ptr;

    fputs(td_fix_portion, stdout);
    td_decode_resp_status(&delete_pool_ptr->message_status);
    td_print_count(&delete_pool_ptr->count);

    if (delete_pool_ptr->count > 0) {
	fputs(td_var_portion, stdout);
	for (i = 0; i < delete_pool_ptr->count; i++) {
	    td_print_poolid(&(delete_pool_ptr->pool_status[i].pool_id));
	    td_decode_resp_status(&(delete_pool_ptr->pool_status[i].status));
	}
    }
}


