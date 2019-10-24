#ifndef lint
static char SccsId[] = "@(#)td_vary_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_vary_req
 *
 * Description:
 *      Decode the message content of a vary request packet.
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
td_vary_req(VERSION version)
{
    fputs(td_fix_portion, stdout);
    if (version == VERSION0) {
	CSI_V0_VARY_REQUEST *vary_ptr;
	
	vary_ptr = (CSI_V0_VARY_REQUEST *) td_msg_ptr;
	td_print_state(&vary_ptr->state);
	td_print_type(&vary_ptr->type);
	td_print_count(&vary_ptr->count);
	if (vary_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_print_id_list((IDENTIFIER *) &vary_ptr->identifier,
			     vary_ptr->type, vary_ptr->count, version);
	}
    }
    else {
	/* the structure of version 1, 2, 3, 4 are the same
	   except the identifier */
	CSI_VARY_REQUEST *vary_ptr;
	
	vary_ptr = (CSI_VARY_REQUEST *) td_msg_ptr;
	td_print_state(&vary_ptr->state);
	td_print_type(&vary_ptr->type);
	td_print_count(&vary_ptr->count);

	if (vary_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_print_id_list((IDENTIFIER *) &vary_ptr->identifier,
			     vary_ptr->type, vary_ptr->count, version);
	}
    }
}










