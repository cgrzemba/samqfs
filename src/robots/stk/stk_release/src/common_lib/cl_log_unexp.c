/**********************************************************************
*
*	C Source:		cl_log_unexp.c
*	Subsystem:		1
*	Description:	
*	%created_by:	kjs %
*	%date_created:	Thu Dec  1 13:51:43 1994 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %filespec: cl_log_unexp.c,2.1.1 %  (%full_filespec: 1,csrc,cl_log_unexp.c,2.1.1 %)";
#endif
/*
 * Copyright (C) 1988,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      cl_log_unexpected
 *
 * Description:
 *
 *      Common routine to log unexpected STATUS errors to the event_logger.
 *      When a routine receives an unexpected status code from a called routine,
 *      it's usually indicative of a programming or system error.  In this case,
 *      the calling routine invokes cl_log_unexpected to log the following
 *      message to the event_logger (via cl_log_event).
 *
 *          caller: callee() unexpected status=%s
 *
 *      where:  caller = name of calling routine receiving unexpected error,
 *              callee = name of called routine returning unexpected error.
 *              status = unexpected status string
 *
 * Return Values:
 *
 *      NONE
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
 *      Verified in log_test.c.
 *
 * Revision History:
 *
 *      D. F. Reed              18-Nov-1988     Original.
 *      Alec Sharp              30-Nov-1992     Changed lbuf size
 *      Mike Williams           01-Jun-2010     Modified to include prototype
 *                                              for cl_log_event
 */

/*
 *      Header Files:
 */

#include "flags.h"
#include "system.h"
#include <stdio.h>

#include "cl_pub.h"
#include "cl_log.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

/*
 *      Procedure Type Declarations:
 */

void 
cl_log_unexpected (
    char *caller,                        /* name of calling routine          */
    char *callee,                        /* name of routine returning error  */
    STATUS status                         /* status code to include in message */
)
{
    register char *sp;
    char lbuf[MAX_LOG_MSG_SIZE];        /* cl_log_event message buffer */

#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_log_unexpected",   /* routine name */
                 3,                     /* parameter count */
                 (unsigned long)caller,
                 (unsigned long)callee,
                 (unsigned long)status);
#endif /* DEBUG */

    /* get status code sting */
    sp = cl_status(status);

    /* format event log message */
    if (*sp == 'U')

        /* unknown? */
        sprintf(lbuf, "%s: %s() unexpected status = %s(%d)\n",
            caller, callee, sp, (int)status);

    else
        sprintf(lbuf, "%s: %s() unexpected status = %s\n", caller, callee, sp);

    /* log it */
    cl_log_event(lbuf);

    return;
}

