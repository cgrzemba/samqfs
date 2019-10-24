#ifndef lint
static char SccsId[] = "@(#)td_vary_p.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_vary_resp
 *
 * Description:
 *      Decode the message content of a vary response packet.
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

#include "csi.h"
#include "td.h"

static void st_print_status_list(VERSION, char *, TYPE, unsigned short);

void
td_vary_resp(VERSION version)
{
    fputs(td_fix_portion, stdout);
    if (version == VERSION0) {
	CSI_V0_VARY_RESPONSE *vary_ptr;
	
	vary_ptr = (CSI_V0_VARY_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&vary_ptr->message_status);
	td_print_state(&vary_ptr->state);
	td_print_type(&vary_ptr->type);
	td_print_count(&vary_ptr->count);
	if (vary_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    st_print_status_list(version, (char *) &vary_ptr->device_status,
				 vary_ptr->type, vary_ptr->count);
	}
    }
    else {
	/* version 1, 2, 3, 4 */
	CSI_VARY_RESPONSE *vary_ptr;
	
	vary_ptr = (CSI_VARY_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&vary_ptr->message_status);
	td_print_state(&vary_ptr->state);
	td_print_type(&vary_ptr->type);
	td_print_count(&vary_ptr->count);
	if (vary_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    st_print_status_list(version, (char *) &vary_ptr->device_status,
				 vary_ptr->type, vary_ptr->count);
	}
    }
}

/*
 * Decode and print an array of status.
 */
static void
st_print_status_list(VERSION version, char *status, TYPE type, unsigned short count)
{
    VA_ACS_STATUS *acs_st;
    VA_LSM_STATUS *lsm_st;
    VA_CAP_STATUS *cap_st;
    VA_DRV_STATUS *drv_st;
    VA_PRT_STATUS *prt_st;
    unsigned short i;
    
    switch(type) {
      case TYPE_ACS:
	acs_st = (VA_ACS_STATUS *)status;
	for (i = 0; i < count; i++, acs_st++) {
	    td_print_acs(&acs_st->acs_id);
	    td_decode_resp_status(&acs_st->status);
	}
	break;
    
      case TYPE_LSM:
	lsm_st = (VA_LSM_STATUS *)status;
	for (i = 0; i < count; i++, lsm_st++) {
	    td_print_lsmid(&lsm_st->lsm_id);
	    td_decode_resp_status(&lsm_st->status);
	}
	break;
		     
      case TYPE_DRIVE:
	drv_st = (VA_DRV_STATUS *)status;
	for (i = 0; i < count; i++, drv_st++) {
	    td_print_driveid(&drv_st->drive_id);
	    td_decode_resp_status(&drv_st->status);
	}
	break;

      case TYPE_PORT:
	prt_st = (VA_PRT_STATUS *)status;
	for (i = 0; i < count; i++, prt_st++) {
	    td_print_portid(&prt_st->port_id);
	    td_decode_resp_status(&prt_st->status);
	}
	break;
	
      case TYPE_CAP:
	/* version 0 and 1 vary response should not have type cap */
	if (version > VERSION1) {
	    cap_st = (VA_CAP_STATUS *)status;
	    for (i = 0; i < count; i++, cap_st++) {
		td_print_capid(&cap_st->cap_id);
		td_decode_resp_status(&cap_st->status);
	    }
	    break;
	}
	/* else, fall through to default */
	
      default:
	fputs("Invalid type, unable to decode!\n", stdout);
    }
}








