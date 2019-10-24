#ifndef lint
static char SccsId[] = "@(#)td_query_q.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_query_req
 *
 * Description:
 *      Decode the message content of a query request packet.
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
 *      S. L. Siao          06-Oct-2000     Added media_type for query mount
 *                                          scratch.
 *      S. L. Siao          27-Feb-2002     Added drive_group, subpool_info.
 *      S. L. Siao          01-Feb-2002     Added mount_scratch_pinfo.
 *
 */

#include "csi.h"
#include "td.h"

void
td_query_req(VERSION version)
{
    fputs(td_fix_portion, stdout);
    if (version == VERSION0) {
	CSI_V0_QUERY_REQUEST *query_ptr;
	
	query_ptr = (CSI_V0_QUERY_REQUEST *) td_msg_ptr;
	td_print_type(&query_ptr->type);
	td_print_count(&query_ptr->count);

	if (query_ptr->count != 0) {
	    fputs(td_var_portion, stdout);
	    td_print_id_list((IDENTIFIER *) &query_ptr->identifier,
			     query_ptr->type, query_ptr->count, version);
	}
    }
    else if (version <= VERSION3) {
	/* version 1, 2, 3 are the same except the cap_id
	   td_print_id_list will take care of it */
	
	CSI_V2_QUERY_REQUEST *query_ptr;
	
	query_ptr = (CSI_V2_QUERY_REQUEST *) td_msg_ptr;
	td_print_type(&query_ptr->type);
	td_print_count(&query_ptr->count);

	if (query_ptr->count != 0) {
	    fputs(td_var_portion, stdout);
	    td_print_id_list((IDENTIFIER *) &query_ptr->identifier,
			     query_ptr->type, query_ptr->count, version);
	}
    }
    else {
	/* version 4 */
	CSI_QUERY_REQUEST *query_ptr;

	query_ptr = (CSI_QUERY_REQUEST *) td_msg_ptr;

	td_print_type(&query_ptr->type);

	switch(query_ptr->type) {
	  case TYPE_ACS:
	    td_print_count(&query_ptr->select_criteria.acs_criteria.acs_count);
	    fputs(td_var_portion, stdout);
	    td_print_acs_list(query_ptr->select_criteria.acs_criteria.acs_id,
			      query_ptr->select_criteria.acs_criteria.acs_count);
	    break;
	  case TYPE_LSM:
	    td_print_count(&query_ptr->select_criteria.lsm_criteria.lsm_count);
	    fputs(td_var_portion, stdout);
	    td_print_lsmid_list(query_ptr->select_criteria.lsm_criteria.lsm_id,
				query_ptr->select_criteria.lsm_criteria.lsm_count);
	    break;
	  case TYPE_CAP:
	    td_print_count(&query_ptr->select_criteria.cap_criteria.cap_count);
	    fputs(td_var_portion, stdout);
	    td_print_capid_list(query_ptr->select_criteria.cap_criteria.cap_id,
				query_ptr->select_criteria.cap_criteria.cap_count);
	    break;
	  case TYPE_DRIVE:
	    td_print_count(&query_ptr->select_criteria.drive_criteria.drive_count);
	    fputs(td_var_portion, stdout);
	    td_print_driveid_list(query_ptr->select_criteria.drive_criteria.drive_id,
				  query_ptr->select_criteria.drive_criteria.drive_count);
	    break;
	  case TYPE_VOLUME:
	  case TYPE_CLEAN:
	  case TYPE_MOUNT:
	    td_print_count(&query_ptr->select_criteria.vol_criteria.volume_count);
	    fputs(td_var_portion, stdout);
	    td_print_volid_list(query_ptr->select_criteria.vol_criteria.volume_id,
				query_ptr->select_criteria.vol_criteria.volume_count);
	    break;
	  case TYPE_PORT:
	    td_print_count(&query_ptr->select_criteria.port_criteria.port_count);
	    fputs(td_var_portion, stdout);
	    td_print_portid_list(query_ptr->select_criteria.port_criteria.port_id,
				 query_ptr->select_criteria.port_criteria.port_count);
	    break;
	  case TYPE_REQUEST:
	    td_print_count(&query_ptr->select_criteria.request_criteria.request_count);
	    fputs(td_var_portion, stdout);
	    td_print_msgid_list(query_ptr->select_criteria.request_criteria.request_id,
				query_ptr->select_criteria.request_criteria.request_count);
	    break;
	  case TYPE_POOL:
	  case TYPE_SCRATCH:
	    td_print_count(&query_ptr->select_criteria.pool_criteria.pool_count);
	    fputs(td_var_portion, stdout);
	    td_print_poolid_list(query_ptr->select_criteria.pool_criteria.pool_id,
				 query_ptr->select_criteria.pool_criteria.pool_count);
	    break;
	  case TYPE_MOUNT_SCRATCH:
	    td_print_media_type(&query_ptr->select_criteria.
					     mount_scratch_criteria.media_type);
	    td_print_count(&query_ptr->select_criteria.mount_scratch_criteria.
				       pool_count);
	    fputs(td_var_portion, stdout);
	    td_print_poolid_list(
		query_ptr->select_criteria.mount_scratch_criteria.pool_id,
		query_ptr->select_criteria.mount_scratch_criteria.pool_count);
	    break;
	  case TYPE_DRIVE_GROUP:
            td_print_group_type(&query_ptr->select_criteria.drive_group_criteria.
				group_type);
            td_print_count(&query_ptr->select_criteria.drive_group_criteria.
			   drg_count);
	    td_print_group_info(query_ptr->select_criteria.drive_group_criteria.
			      drg_count, query_ptr->select_criteria.
			      drive_group_criteria.group_id);
	    break;
	  case TYPE_SUBPOOL_NAME:
	    td_print_count(&query_ptr->select_criteria.subpl_name_criteria.
			   spn_count);
	    td_print_subpool_info(query_ptr->select_criteria.
				  subpl_name_criteria.spn_count,
				  query_ptr->select_criteria.
			          subpl_name_criteria.subpl_name);
	    break;
	  case TYPE_MOUNT_SCRATCH_PINFO:
	    td_print_media_type(&query_ptr->select_criteria.
				     mount_scratch_pinfo_criteria.media_type);
	    td_print_count(&query_ptr->select_criteria.
				     mount_scratch_pinfo_criteria.pool_count);
	    fputs(td_var_portion, stdout);
	    td_print_poolid_list(
		query_ptr->select_criteria.mount_scratch_pinfo_criteria.pool_id,
		query_ptr->select_criteria.mount_scratch_pinfo_criteria.
			   pool_count);
	    td_print_mgmt_clas(&query_ptr->select_criteria.
				        mount_scratch_pinfo_criteria.mgmt_clas);
	    break;
	  default:
	    break;
	}
    }
}











