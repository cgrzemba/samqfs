#ifndef lint
static char SccsId[] = "@(#)td_set_cln_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_set_clean_req
 *
 * Description:
 *      Decode the message content of a set_clean request packet.
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
 *      M. H. Shum          10-Sep-1993     Original.
 *
 */

#include "td.h"

void
td_set_clean_req(VERSION version)
{
    /* the SET_CLEAN_REQUEST of version 1, 2, 3, 4 are the same */
    CSI_SET_CLEAN_REQUEST *set_clean;
    
    set_clean = (CSI_SET_CLEAN_REQUEST *) td_msg_ptr;

    fputs(td_fix_portion, stdout);
    td_print_max_use(&set_clean->max_use);
    td_print_count(&set_clean->count);

    if (set_clean->count > 0) {
	fputs(td_var_portion, stdout);
	td_print_volrange_list(set_clean->vol_range, set_clean->count);
    }
}




