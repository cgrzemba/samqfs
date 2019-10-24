#ifndef lint
static char SccsId[] = "@(#)td_audit_p.c	2.0 1/20/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_audit_resp.c
 *
 * Description:
 *      Decode and output the command specific portion of an audit 
 *	response packet.
 *      (from version 0 to version 4)
 * 
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_ptr, td_fix_portion, td_var_portion, td_mopts.
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
 *      M. H. Shum          20-Jan-1994     Added td_decode_resp_status to
 *                                          version 4 packet.
 */

/*
 * header files
 */
#include <stdio.h>
#include "csi.h"
#include "td.h"

/*
 * static function prototype
 */

static void st_print_status_list(TYPE, char *, unsigned short);


void
td_audit_resp(VERSION version)
{
    /* message option INTERMEDIATE indicates a eject enter response */
    if (td_mopts & INTERMEDIATE) {
	td_eject_enter(version);
	return;
    }

    /* final audit response */
    fputs(td_fix_portion, stdout);

    if (version == VERSION0) {
        /* version 0 has different message header */
        CSI_V0_AUDIT_RESPONSE *audit_ptr = (CSI_V0_AUDIT_RESPONSE *) td_msg_ptr;

	td_decode_resp_status(&audit_ptr->message_status);
        td_print_v0_capid(&audit_ptr->cap_id);
        td_print_type(&audit_ptr->type);
        td_print_count(&audit_ptr->count);

        /* variable portion code */
	if (audit_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    st_print_status_list(audit_ptr->type,
				 (char *) &audit_ptr->identifier_status,
				 audit_ptr->count);
	}
    } 
    else if (version == VERSION1) {
        /* version 1 has different capid */
        CSI_V1_AUDIT_RESPONSE *audit_ptr = (CSI_V1_AUDIT_RESPONSE *) td_msg_ptr;

	td_decode_resp_status(&audit_ptr->message_status);
        td_print_v0_capid(&audit_ptr->cap_id); /* v1_capid is the same as v0 */
        td_print_type(&audit_ptr->type);
        td_print_count(&audit_ptr->count);

        /* variable portion code */
	if (audit_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    st_print_status_list(audit_ptr->type,
				 (char *) &audit_ptr->identifier_status,
				 audit_ptr->count);
	}
    } 
    else {	
        /* version 2, 3 and 4 are the same */
        CSI_AUDIT_RESPONSE *audit_ptr = (CSI_AUDIT_RESPONSE *) td_msg_ptr;

	td_decode_resp_status(&audit_ptr->message_status);
        td_print_capid(&audit_ptr->cap_id);
        td_print_type(&audit_ptr->type);
        td_print_count(&audit_ptr->count);

        /* variable portion code */
	if (audit_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    st_print_status_list(audit_ptr->type,
				 (char *) &audit_ptr->identifier_status,
				 audit_ptr->count);
	}
    }
}

/*
 * Decode an array of status.
 */
static void
st_print_status_list(TYPE type, char *status, unsigned short count)
{
    AU_ACS_STATUS *acs_st;
    AU_LSM_STATUS *lsm_st;
    AU_PNL_STATUS *pnl_st;
    AU_SUB_STATUS *sub_st;
    unsigned short i;
    
    switch(type) {
      case TYPE_ACS:
	acs_st = (AU_ACS_STATUS *)status;
	for (i = 0; i < count; i++, acs_st++) {
	    td_print_acs(&acs_st->acs_id);
	    td_decode_resp_status(&acs_st->status);
	}
	break;
    
      case TYPE_LSM:
	lsm_st = (AU_LSM_STATUS *)status;
	for (i = 0; i < count; i++, lsm_st++) {
	    td_print_lsmid(&lsm_st->lsm_id);
	    td_decode_resp_status(&lsm_st->status);
	}
	break;
		     
      case TYPE_PANEL:
	pnl_st = (AU_PNL_STATUS *)status;
	for (i = 0; i < count; i++, pnl_st++) {
	    td_print_panelid(&pnl_st->panel_id);
	    td_decode_resp_status(&pnl_st->status);
	}
	break;

      case TYPE_SUBPANEL:
	sub_st = (AU_SUB_STATUS *)status;
	for (i = 0; i < count; i++, sub_st++) {
	    td_print_subpanelid(&sub_st->subpanel_id);
	    td_decode_resp_status(&sub_st->status);
	}
	break;
    }
}






