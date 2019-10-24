#ifndef lint
static char SccsId[] = "@(#)td_audit_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_audit_req.c
 *
 * Description:
 *      Decode and output the command specific portion of an audit 
 *	request packet. 
 *      It can handle version 0 to version 4.      
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

#include <stdio.h>
#include "csi.h"
#include "td.h"

void
td_audit_req(VERSION version)
{
    fputs(td_fix_portion, stdout);

    if (version == VERSION0) {
        /* version 0 has different message header */
        CSI_V0_AUDIT_REQUEST *audit_ptr = (CSI_V0_AUDIT_REQUEST *) td_msg_ptr;

	td_print_v0_capid(&audit_ptr->cap_id);
        td_print_type(&audit_ptr->type);
        td_print_count(&audit_ptr->count);

	if (audit_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_print_id_list((IDENTIFIER *) &audit_ptr->identifier,
			     audit_ptr->type, audit_ptr->count, version);
	}
    }
    else if (version == VERSION1) {	
        /* version 1 has different cap id */
        CSI_V1_AUDIT_REQUEST *audit_ptr = (CSI_V1_AUDIT_REQUEST *) td_msg_ptr;

	td_print_v0_capid(&audit_ptr->cap_id);
        td_print_type(&audit_ptr->type);
        td_print_count(&audit_ptr->count);

	if (audit_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_print_id_list((IDENTIFIER *) &audit_ptr->identifier,
			     audit_ptr->type, audit_ptr->count, version);
	}
    }
    else {
        /* version 2, 3 and 4 are the same */
        CSI_AUDIT_REQUEST *audit_ptr = (CSI_AUDIT_REQUEST *) td_msg_ptr;
	
        td_print_capid(&audit_ptr->cap_id);
	td_print_type(&audit_ptr->type);
        td_print_count(&audit_ptr->count);
	
	if (audit_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    td_print_id_list((IDENTIFIER *) &audit_ptr->identifier,
			 audit_ptr->type, audit_ptr->count, version);
	}
    }
}






