#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_get_pktver/2.01A %";
#endif
/*
 *
 *                             Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_get_packet_version()
 *
 * Description:
 *      This procedure is called by acs toolkit interface functions.
 *      It returns a packet version number. This number represents the
 *      highest packet version that the current ACSLM server will 
 *      support.
 *
 * Return Values:
 *      A VERSION enumeration value.
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
 *    Ken Stickney         10-Jun-1993    Original
 *    Ken Stickney         06-Nov-1993    Added param section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright. Changes for new error
 *                                        messaging. Changed code to check
 *                                        for lowest version no. supported
 *                                        as well.
 *    Ken Stickney         25-May-1994    Fixed check for bounds of incoming
 *                                        packet version number. BR#26.
 */

/* Header Files: */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "acsapi.h"
#include "acssys_pvt.h"

/* Defines, Typedefs and Structure Definitions: */

#undef SELF
#define SELF	"acs_get_packet_version"
#undef ACSMOD
#define ACSMOD 107

/* Global and Static Variable Declarations: */

/* Procedure Type Declarations: */

VERSION acs_get_packet_version ()
{

    char *envp;
    ACSMESSAGES msg_num;

    static BOOLEAN once = FALSE;

    VERSION highest_packet_version = (VERSION) ((int) (VERSION_LAST) - 1);
    VERSION lowest_packet_version = (VERSION) ((int) VERSION_LAST - 
					       NUM_RECENT_VERSIONS);

    /* get user supplied version number */
    if ((envp = getenv (ACSAPI_PACKET_VERSION)) != NULL) {

	if ((atoi (envp)) > (int) highest_packet_version ||
	    (atoi (envp)) < (int) lowest_packet_version) {
	    if (!once) {
		int def_val = (int)(highest_packet_version);
		int bad_val = (int)(atoi(envp));
		msg_num = ACSMSG_INVALID_VERSION;
		acs_error_msg ((&msg_num, bad_val, def_val));
		once = TRUE;
	    }
	}
	else {
	    /* convert version string to a number */
	    highest_packet_version = (VERSION) atoi (envp);
	}
    }
    return highest_packet_version;
}
