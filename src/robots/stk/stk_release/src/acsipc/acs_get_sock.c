#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsipc/csrc/acs_get_sock/2.01A %";
#endif
/*
 *
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *  acs_get_sockname()
 *
 * Description:
 *      The ACSAPI_SSI_SOCKET environment variable is read
        and a pointer to the name of the socket is returned.
 *
 * Return Values:
 *     NULL or socket name string.
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
 *    Ken Stickney         15-Jun-1993    Original.
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright. Changes for new error
 *                                        messaging.
 *    Ken Stickney         31-May-1994    Changes to support ANSI standard
 *                                        arg (stdarg.h) usage by acs_error_
 *                                        msg.
 *    Joseph Nofi          15-Jul-2011    Change default port number from 
 *                                        "50004" to defs.h ACSSA #define.
 *
 */

/* Header Files: */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "defs.h"

/* Defines, Typedefs and Structure Definitions: */
#undef SELF
#define SELF    "acs_get_sockname"
#undef ACSMOD
#define ACSMOD 106

/* Global and Static Variable Declarations: */

/* Procedure Type Declarations: */

char *
  acs_get_sockname (void)
{
    char *envp;
    char *sp;
    ACSMESSAGES msg_num;
    static char ssi_socket[SOCKET_NAME_SIZE] = "\0";
    static BOOLEAN got_name = FALSE;

    /* get SSI socket name */
    if (!got_name) {
    envp = getenv ("ACSAPI_SSI_SOCKET");
    for (sp = envp; NULL != sp && '\0' != *sp; sp++) {
        if (!isdigit (*sp)) {
        msg_num = ACSMSG_BAD_SOCKET;
        acs_error_msg ((&msg_num, envp));
        return (NULL);
        }
    }
    if (envp) {
        strcpy (ssi_socket, envp);
    }
    else {
        strcpy (ssi_socket, ACSSA);
    }
    got_name = TRUE;
    }

    return ssi_socket;
}

