#ifndef lint
static char SccsId[] = "@(#)td_mount_pinfo_q.c	2.2 2/10/02 ";
#endif
/*
 * Copyright (2002, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_mount_pinfo_req
 *
 * Description:
 *      Decode the message content of a mount request packet.
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
 *      S. L. Siao          10-Feb-2002     Original.
 *
 */

#include "csi.h"
#include "td.h"

void
td_mount_pinfo_req(VERSION version)
{
    CSI_MOUNT_PINFO_REQUEST *mount_pinfo_ptr;
    
    mount_pinfo_ptr = (CSI_MOUNT_PINFO_REQUEST *) td_msg_ptr;
    td_print_volid(&mount_pinfo_ptr->vol_id);
    td_print_poolid(&mount_pinfo_ptr->pool_id);
    td_print_mgmt_clas(&mount_pinfo_ptr->mgmt_clas);
    td_print_media_type(&mount_pinfo_ptr->media_type);
    td_print_job_name(&mount_pinfo_ptr->job_name);
    td_print_dataset_name(&mount_pinfo_ptr->dataset_name);
    td_print_step_name(&mount_pinfo_ptr->step_name);
    fputs(td_var_portion, stdout);
    td_print_driveid(&mount_pinfo_ptr->drive_id);
}




