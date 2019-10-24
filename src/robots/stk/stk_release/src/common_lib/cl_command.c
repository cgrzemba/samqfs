static char P4_Id[]="$Id: //depot/acsls6.0/lib/common/cl_command.c#2 $";

/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_command
 *
 * Description:
 *
 *      Common routine to convert a COMMAND enum value to its character
 *      string counterpart.
 *
 * Return Values:
 *
 *      (char *)pointer to character string.
 *      if value is invalid, pointer to string "UNKNOWN_COMMAND(%d)".
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      All common_lib test programs.
 *
 * Revision History:
 *
 *      D. A. Beidle            25-Jan-1989     Original.
 *      J. W. Montgomery        27-Apr-1990     Added Scratch stuff.
 *      H. I. Grapek            30-Apr-1990     Added clean drive commands
 *      D. L. Trachy            03-May-1990     Added lock server commands
 *      Alec Sharp              08-Sep-1992     Added set_owner command
 *      H. L. Freeman           18-Mar-1997     R5.2 - Switch LMU support.
 *      George Noble            16-Dec-2000     R6.0 - added Cartridge Recovery.
 *      Scott Siao              26-Oct-2001     Added Display, register, 
 *                                              unregister,
 *                                              check_registration commands
 *      Scott Siao              26-Feb-2002     Added mount_pinfo.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>                      /* ANSI-C compatible */

#include "cl_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

static struct command_map {
    COMMAND     command_value;
    char       *command_string;
} command_table[] = {
    COMMAND_ABORT,              "ABORT",
    COMMAND_AUDIT,              "AUDIT",
    COMMAND_CANCEL,             "CANCEL",
    COMMAND_DEFINE_POOL,        "DEFINE_POOL",
    COMMAND_DELETE_POOL,        "DELETE_POOL",
    COMMAND_DISMOUNT,           "DISMOUNT",
    COMMAND_EJECT,              "EJECT",
    COMMAND_ENTER,              "ENTER",
    COMMAND_IDLE,               "IDLE",
    COMMAND_MOUNT,              "MOUNT",
    COMMAND_MOUNT_SCRATCH,      "MOUNT",     /* Map mount_scratch to mount */
    COMMAND_QUERY,              "QUERY",
    COMMAND_RECOVERY,           "RECOVERY",
    COMMAND_SET_SCRATCH,        "SET_SCRATCH",
    COMMAND_SET_CLEAN,          "SET_CLEAN",
    COMMAND_SET_OWNER,          "SET_OWNER",
    COMMAND_START,              "START",
    COMMAND_VARY,               "VARY",
    COMMAND_TERMINATE,          "TERMINATE",
    COMMAND_LOCK,               "LOCK",
    COMMAND_UNLOCK,             "UNLOCK",
    COMMAND_CLEAR_LOCK,         "CLEAR_LOCK",
    COMMAND_QUERY_LOCK,         "QUERY_LOCK",
    COMMAND_SET_CAP,            "SET_CAP",
    COMMAND_LS_RES_AVAIL,       "LOCK_RESOURCES_AVAILABLE",
    COMMAND_LS_RES_REM,         "LOCK_RESOURCES_REMOVED",
    COMMAND_INIT,               "INIT",
    COMMAND_SELECT,             "SELECT",
    COMMAND_UNSOLICITED_EVENT,  "UNSOLICITED_EVENT",
    COMMAND_DB_REQUEST,         "DB_REQUEST",
    COMMAND_SWITCH,             "SWITCH",
    COMMAND_MOVE,               "MOVE", 
    COMMAND_RCVY,               "CARTRIDGE RCVY",
    COMMAND_DISPLAY,            "DISPLAY",
    COMMAND_REGISTER,           "REGISTER",
    COMMAND_UNREGISTER,         "UNREGISTER",
    COMMAND_CHECK_REGISTRATION, "CHECK_REGISTRATION",
    COMMAND_MOUNT_PINFO,        "MOUNT_PINFO"
};

static char unknown[32];                /* unknown command buffer */

/*
 *      Procedure Type Declarations:
 */


char *
cl_command (
    COMMAND command                    /* command code to convert */
)
{
    int     i;


#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_command",          /* routine name */
                 1,                     /* parameter count */
                 (unsigned long)command);
#endif /* DEBUG */

    /* look through table for command */
    for (i = 0; i < (sizeof(command_table)/sizeof(struct command_map)); i++) 
        if (command == command_table[i].command_value) break;
    /* did we find it? */
    if (i >= (sizeof(command_table)/sizeof(struct command_map))) {
        sprintf(unknown, "UNKNOWN_COMMAND(%d)", command);
        return(unknown);
    }

    return(command_table[i].command_string);
}
