#ifndef lint
static char SccsId[] = "@(#)td_req.c	2.2 10/10/01 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_decode_req
 *
 * Description:
 *      Calls the appropriate function to decode a request packet.
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
 *      S. L. Siao          25-Oct-2001     Added register, unregister, and check_registration
 *                                          commands.
 *      S. L. Siao          13-Nov-2001     Added display command.
 *      S. L. Siao          28-Jan-2002     Synchronized with command enum.
 *
 */

#include <stdio.h>
#include "td.h"


void 
td_decode_req(VERSION version, COMMAND command)
{
    /* an array of pointers to function for every command, this
       array corresponds to the enum of COMMAND */

    static FUNC req_funcs[COMMAND_LAST] = {
	td_invalid_command,	/* 0 */
	td_audit_req,
    	td_cancel_req,
	td_dismount_req,
	td_eject_req,

	td_enter_req,		/* 5 */
	td_idle_command,
	td_mount_req,
	td_query_req,
	td_invalid_command,	/* ACSLM internal use only */

	td_start_command,	/* 10 */
	td_vary_req,
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */

	td_set_scratch_req,	/* 15 */
	td_define_pool_req,
	td_delete_pool_req,
	td_set_clean_req,
	td_mount_scratch_req,

	td_lock_req,		/* 20 */
	td_lock_req,            /* the structure of lock, unlock, */
	td_lock_req,		/* clear lock and query lock request */
	td_lock_req,            /* are the same */
	td_set_cap_req,

	td_invalid_command,	/* 25 */ /* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */
	td_invalid_command,	/* ACSLM internal use only */

	td_invalid_command,     /* 30 */
	td_mount_pinfo_req,
	td_invalid_command,
	td_invalid_command,
	td_invalid_command,

	td_display_req,         /* 35 */ /* display command */
	td_register_req,       
	td_unregister_req,
	td_check_registration_req,
	td_invalid_command

    };

    if (version >= VERSION_LAST) {
	printf("Invalid version: %d, unable to decode!\n", version);
	return;
    }

    printf("\nVersion %d %s Request\n", version, cl_command(command));

    if (command < COMMAND_LAST)
	req_funcs[command](version);
    else
	td_invalid_command(version);
}

void
td_invalid_command(VERSION version)
{
    printf("\nInvalid Command, unable to decode!\n");
}


