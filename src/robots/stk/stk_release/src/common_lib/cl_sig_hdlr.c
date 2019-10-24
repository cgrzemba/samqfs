static char SccsId[] = "@(#)cl_sig_hdlr.c	5.8 12/2/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *  cl_sig_hdlr
 *
 * Description:
 *
 *      Common routine to catch signals indicating that a request_process is
 *  cancelled (SIGQUIT) or terminated (SIGTERM).  If cancelled, the
 *  global variable, process_state, is set to STATE_CANCELLED.  If
 *  terminated, this routine calls cl_ipc_destroy() and issues a exit(1).
 *
 * Return Values:
 *
 *  NONE
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      process_state
 *
 * Considerations:
 *
 *  NONE
 *
 * Module Test Plan:
 *
 *  NONE
 *
 * Revision History:
 *
 *  D. F. Reed      11-Oct-1988 Original.
 *  D. B. Farmer        30-Sep-1993     Replace signal() with sigaction().
 *					no longer need to re-arm handler
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "cl_pub.h"
#include "ml_pub.h"

/*
 *  Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

/*
 *      Procedure Type Declarations:
 */

void 
cl_sig_hdlr (
    int sig                /* signal number received */
)
{
#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_sig_hdlr",     /* routine name */
                 1,         /* parameter count */
                 (unsigned long)sig);
#endif /* DEBUG */
    
    MLOGDEBUG(0,(MMSG(514, "cl_sig_hdlr(): Detected signal %d\n"), sig));

    if (sig == SIGQUIT) {

        /* set process state and return */
        process_state = STATE_CANCELLED;
        return;
    }

    /* otherwise, if terminate, just exit */
    if (sig == SIGTERM) {
        exit((int)STATUS_TERMINATED);
    }

    /* shouldn't get here, but if we do, log it */
    MLOG((MMSG(515, "cl_sig_hdlr(): unexpected signal=%d\n"), sig));

    return;
}
