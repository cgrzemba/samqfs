static char SccsId[] = "@(#)cl_state.c	5.4 05/13/02 ";
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_state
 *
 * Description:
 *
 *      Common routine to convert a STATE enum value to its character
 *      string counterpart.
 *
 * Return Values:
 *
 *      (char *)pointer to character string.
 *      if value is invalid, pointer to string "UNKNOWN_STATE(%d)".
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
 *      NONE
 *
 * Revision History:
 *
 *      J. S. Alexander     06-Jun-1991     Original.
 *      S. L. Siao          13-May-2002     Re-synched with new values for state
 *                                          Added: STATE_CONNECT,
 *						   STATE_DISCONNECT,
 *	                                           STATE_DISBAND_1,
 *	                                           STATE_DISBAND_2,
 *		                                   STATE_DISBAND_3,
 *		                                   STATE_DISBAND_4,
 *			                           STATE_JOIN_1,
 *			                           STATE_JOIN_2,
 *				                   STATE_RESIGN_1,
 *				                   STATE_RESIGN_2,
 *						   STATE_RESIGN_3.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>                      /* ANSI-C compatible */

#include "cl_pub.h"
#include "ml_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

static  struct  state_map {
    STATE        state_value;
    char         *state_string;
} state_table[] = {
    STATE_CANCELLED,        "STATE_CANCELLED",
    STATE_DIAGNOSTIC,       "STATE_DIAGNOSTIC",
    STATE_IDLE,             "STATE_IDLE",
    STATE_IDLE_PENDING,     "STATE_IDLE_PENDING",
    STATE_OFFLINE,          "STATE_OFFLINE",
    STATE_OFFLINE_PENDING,  "STATE_OFFLINE_PENDING",
    STATE_ONLINE,           "STATE_ONLINE",
    STATE_RECOVERY,         "STATE_RECOVERY",
    STATE_RUN,              "STATE_RUN",
    STATE_CONNECT,          "STATE_CONNECT",
    STATE_DISCONNECT,       "STATE_DISCONNECT",
    STATE_DISBAND_1,        "STATE_DISBAND_1",
    STATE_DISBAND_2,        "STATE_DISBAND_2",
    STATE_DISBAND_3,        "STATE_DISBAND_3",
    STATE_DISBAND_4,        "STATE_DISBAND_4",
    STATE_JOIN_1,           "STATE_JOIN_1",
    STATE_JOIN_2,           "STATE_JOIN_2",
    STATE_RESIGN_1,         "STATE_RESIGN_1",
    STATE_RESIGN_2,         "STATE_RESIGN_2",
    STATE_RESIGN_3,         "STATE_RESIGN_3",
    STATE_LAST,             "code %d",
};

static  char   *self = "cl_state";
static  char   unknown[32];             /* unknown state buffer */

/*
 *      Procedure Type Declarations:
 */


char *
cl_state (
    STATE state                         /* state code to convert */
)
{
    int     i;


#ifdef DEBUG
    if TRACE(0) {
        cl_trace("cl_state", 1,         /* routine name and parameter count */
                 (unsigned long)state);
    }
#endif /* DEBUG */
 
    /* verify table contents */
    if ((int)STATE_LAST != (sizeof(state_table) / sizeof(struct state_map))) {
        MLOGDEBUG(0,(MMSG(159,"%s: Contents of table %s are incorrect"), self,  "state_table"));
    }


    /* Look for the state parameter in the table. */
    for (i = 0; state_table[i].state_value != STATE_LAST; i++) {
        if (state == state_table[i].state_value)
            return state_table[i].state_string;
    }


    /* untranslatable state */
    sprintf(unknown, state_table[i].state_string, state);
    return unknown;
}
