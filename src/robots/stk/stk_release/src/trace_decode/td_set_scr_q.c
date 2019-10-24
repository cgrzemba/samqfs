#ifndef lint
static char SccsId[] = "@(#)td_set_scr_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_set_scratch_req
 *
 * Description:
 *      Decode the message content of a set_scratch request packet.
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
td_set_scratch_req(VERSION version)
{
    /* the SET_SCRATCH_REQUEST of version 1, 2, 3, 4 are the same */
    CSI_SET_SCRATCH_REQUEST *set_scratch;
    
    set_scratch = (CSI_SET_SCRATCH_REQUEST *) td_msg_ptr;

    fputs(td_fix_portion, stdout);
    td_print_poolid(&set_scratch->pool_id);
    td_print_count(&set_scratch->count);

    if (set_scratch->count > 0) {
	fputs(td_var_portion, stdout);
	td_print_volrange_list(set_scratch->vol_range, set_scratch->count);
    }
}


