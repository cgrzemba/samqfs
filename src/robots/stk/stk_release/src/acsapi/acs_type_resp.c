#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:	acsapi/csrc/acs_type_resp/2.1 %";
#endif
/*
 *
 *                            (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_type_response()
 *
 * Description:
 *      This procedure is called by acs toolkit interface functions.
 *      It returns a the name of the ACS RESPONSE TYPE given a
 *      ACS_RESPONSE_TYPE enum value.
 *
 * Return Values:
 *      A string describing the ACS_RESPONSE_TYPE.
 *
 * Parameters:
 *
 *  rtype - ACS_RESPONSE_TYPE enum corresponding to an entry in the 
 *          type name table found below.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Ken Stickney         1-Oct-1993    Original
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright. Changes for new error
 *                                        messaging.
 *   Ken Stickney          29-Apr-1994    Changed name for semantic sense from
 *                                        acs_type.
 *   Ken Stickney          31-Aug-1994    Added conversion of RT_FINAL for 
 *                                        BR#37.
 */

/* Header Files: */
#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"

/* Defines, Typedefs and Structure Definitions: */

#undef SELF
#define SELF	"acs_type_response"
#undef ACSMOD
#define ACSMOD 108

/* Global and Static Variable Declarations: */

/* Procedure Type Declarations: */

  static struct typename {
      ACS_RESPONSE_TYPE rtype;
      char *tname_string;
  }
tname_table[] = {

    RT_FIRST,
    "RT_FIRST",
    RT_ACKNOWLEDGE,
    "RT_ACKNOWLEDGE",
    RT_INTERMEDIATE,
    "RT_INTERMEDIATE",
    RT_FINAL,
    "RT_FINAL",
    RT_NONE,
    "RT_NONE",
    RT_LAST,
    "RT_LAST",
};

static char unknown[80];

char *acs_type_response (ACS_RESPONSE_TYPE rtype)
{

    unsigned short i;
    ACSMESSAGES msg_num;
    acs_trace_entry ();

#ifdef DEBUG
    /* verify table contents (allow for STATUS_SUCCESS in table size) */
    if ((int) STATUS_LAST != ((sizeof (tname_table) /
		sizeof (struct typename)) - 1)) {
        msg_num = ACSMSG_BAD_RESPONSE_TYPE;
	acs_error_msg ((&msg_num, rtype));
    }
#endif /* DEBUG */

    /* Look for the status parameter in the table. */
    for (i = 1; tname_table[i].rtype != RT_LAST; i++) {
	if (rtype == tname_table[i].rtype) {
	    acs_trace_exit (0);
	    return tname_table[i].tname_string;
	}
    }

    /* untranslatable status */
    acs_trace_exit (0);
    sprintf (unknown, tname_table[i].tname_string);
    return unknown;
}
