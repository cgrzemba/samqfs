#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  1/csrc/acs_set_access.c/2.1.3 %";
#endif
/*
 * Copyright (1994, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *      acs_set_access()
 *
 * Description:
 *
 *      This procedure is called by an Application Program to set the global
 *      access ID value, which will be sent to the server as part of the
 *      request packet header for every request.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_INVALID_VALUE.
 *
 * Parameters:
 *     aid_user       - Name to be used to fill in the user_id of the access_id
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *     global_aid     - This structure changed to contain new user_id member
 *
 * Considerations:
 *
 * Module Test Plan:
 * Revision History:
 *
 *      Mitch Black	01-Aug-1994   Original.
 *      Ken Stickney    11-Aug-1994   Changed code to treat NULL user_id
 *                                    input parm as reset of access control 
 *                                    user id.
 *      Ken Stickney    11-Dec-1994   Explicit cast required for Solaris
 *	
 */

/*
 *      Header Files:
 */

#include <stddef.h>                      /* system include files */
#include <stdio.h>                      /* system include files */
#include <string.h>                      /* system include files */

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */
#undef SELF
#define SELF  "acs_set_access"
#undef ACSMOD
#define ACSMOD 55

/*
 *      Global and Static Variable Declarations:
 */


/*
 *      Procedure Type Declarations:
 */

STATUS acs_set_access
(
    char *user_id      /* Ptr to string no longer than EXTERNAL_USERID_SIZE */
)
{
    COPYRIGHT;
    STATUS acsRtn;

    acs_trace_entry ();

    acsRtn = acs_verify_ssi_running ();

    if (acsRtn == STATUS_SUCCESS) {
        if (user_id != NULL) {
            if (strlen(user_id) <= (size_t)EXTERNAL_USERID_SIZE) {
                    strcpy(global_aid.user_id.user_label, user_id);
                    acsRtn = STATUS_SUCCESS;
            } else {        /* Passed in label is too long */
                    acsRtn = STATUS_INVALID_VALUE;
            }
        }
        else {
            global_aid.user_id.user_label[0] = '\0';
        }
    }
    acs_trace_exit (acsRtn); 
    return acsRtn;
}
