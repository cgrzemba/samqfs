static char SccsId[] = "@(#)cl_get_sockn.c	5.2 5/3/93 ";
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_get_sockname
 *
 * Description:
 *
 *      Common routine for getting alternative port numbers for the well-known
 *      socket names.  This is done under debug only to permit multiple ACSLS
 *      test environments to run concurrently.
 *
 * Return Values:
 *
 *      char *      = pointer to alternative socket name if DEBUG and
 *                    alternative exists; otherwise it points to original
 *                    socket name.
 *
 * Implicit Inputs:
 *
 *      environment variables   specify alternate ports for standard sockets.
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
 *      D. A. Beidle            28-Sep-1991     Original.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>
#include <stdlib.h>                     /* ANSI-C compatible */

#include "string.h"
#include "cl_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

/*
 *      Procedure Type Declarations:
 */

char *
cl_get_sockname (
    char *sock_name                     /* pointer to input socket name     */
)
{

#ifdef DEBUG                            /* check environment only if DEBUG  */
    char *s;

    if TRACE(0)
        cl_trace("cl_get_sockname", 1,  /* routine name and parameter count */
                 (unsigned long)sock_name);

    /* check environment for well-known name changes */
    if (strcmp(sock_name, ACSEL) == 0) {
        if ((s = getenv("EL_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSLH) == 0) {
        if ((s = getenv("LH_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSLM) == 0) {
        if ((s = getenv("LM_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSSA) == 0) {
        if ((s = getenv("SA_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSSS) == 0) {
        if ((s = getenv("SS_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSPD) == 0) {
        if ((s = getenv("PD_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSLOCK) == 0) {
        if ((s = getenv("LO_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSSV) == 0) {
        if ((s = getenv("SV_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSCM) == 0) {
        if ((s = getenv("CM_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACES) == 0) {
        if ((s = getenv("DG_PORT")) != NULL)
            return s;
    }
    else if (strcmp(sock_name, ACSMT) == 0) {
        if ((s = getenv("MT_PORT")) != NULL)
            return s;
    }

#endif /* DEBUG */

    return sock_name;
}
