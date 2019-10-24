#ifndef lint
static char SccsId[] = "@(#)td_mount_pinfo_p.c	2.2 2/10/02 ";
#endif
/*
 * Copyright (2002, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_virtual_mount_resp
 *
 * Description:
 *      Decode the message content of a virtual mount response packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_buf, td_fix_portion, td_var_portion.
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
 *	Mitch Black	28-Dec-2004	Fixed SccsId.
 *
 */

#include "csi.h"
#include "td.h"

void
td_mount_pinfo_resp(VERSION version)
{
    CSI_MOUNT_PINFO_RESPONSE *mount_pinfo_ptr;
    
    mount_pinfo_ptr = (CSI_MOUNT_PINFO_RESPONSE *) td_msg_ptr;
    td_decode_resp_status(&mount_pinfo_ptr->message_status);
    td_print_poolid(&mount_pinfo_ptr->pool_id);
    td_print_volid(&mount_pinfo_ptr->vol_id);
    td_print_driveid(&mount_pinfo_ptr->drive_id);
}



