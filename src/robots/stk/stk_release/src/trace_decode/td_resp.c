#ifndef lint
static char SccsId[] = "@(#)td_resp.c	2.2 10/25/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_decode_resp
 *
 * Description:
 *      td_decode_resp call the appropriate function to decode a response
 *      command packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      NONE
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
 *      S. L. Siao          25-Oct-2001     Added register, unregister, check_registration
 *                                          commands.
 *      S. L. Siao          13-Nov-2001     Added display command.
 *      S. L. Siao          28-Jan-2002     Re-synched with command enum.
 *
 */

#include <stdio.h>

#include "td.h"

void 
td_decode_resp(VERSION version, COMMAND command)
{
    /* an array of pointers to functions for every command, this
       array corresponds to the enum of COMMAND */

    static FUNC resp_funcs[COMMAND_LAST] = {
	td_invalid_command,	/* 0 */
	td_audit_resp,
    	td_cancel_resp,
	td_dismount_resp,
	td_eject_enter,		/* eject_response is the same as eject_enter */

	td_eject_enter,		/* enter_response is the same as eject_enter */
	td_idle_command,
	td_mount_resp,
	td_query_resp,
	td_invalid_command,	/* ACSLM internal use only */

	td_start_command,	/* 10 */
	td_vary_resp,
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */

	td_set_scratch_resp,	/* 15 */
	td_define_pool_resp,
	td_delete_pool_resp,
	td_set_clean_resp,
	td_mount_scratch_resp,

	td_lock_resp,		/* 20 */
	td_lock_resp,		/* the structure of lock, unlock */
	td_lock_resp,		/* and clear lock response are the same */
	td_query_lock_resp,
	td_set_cap_resp,

	td_invalid_command,	/* 25 */ /* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */

	td_invalid_command,	/* 30 */ /* ACSLM internal use only */
	td_mount_pinfo_resp,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */

	td_display_resp,	/* 35 */ /* Display Command */
	td_register_resp,	/* Register Command*/
	td_unregister_resp,	/* Unregister Command */
	td_check_registration_resp,	/* Check Registration Command */
	td_invalid_command	/* ACSLM internal use only */
    };

    if (version >= VERSION_LAST) {
	printf("Invalid version: %d, unable to decode!\n", version);
	return;
    }

    printf("\nVersion %d %s Response\n", version, cl_command(command));
    
    if (command < COMMAND_LAST)
	resp_funcs[command](version);
    else
	td_invalid_command(version);
}







