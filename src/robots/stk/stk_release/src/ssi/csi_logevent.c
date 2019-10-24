#ifndef lint
static char *_csrc = "@(#) %filespec: csi_logevent.c,2 %  (%full_filespec: csi_logevent.c,2:csrc:1 %)";
#endif
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 * Name:
 *
 *      csi_logevent()
 *
 * Description:
 *
 *      Event logging function.  Differentiates between 3 types of logging:
 *      (1) logs errors to event log only  (2) logs status messages to event
 *      log only.
 *
 *      If STATUS_SUCCESS == status (passed in): 
 *      
 *              Calls sprintf to format a status message for writing to the
 *              event log.
 *      
 *      Else (all other status's):
 *      
 *              Calls sprintf to format informational message for writing to the
 *              writing to the event log.
 *
 *      Calls cl_log_event() to log the message to the event log.
 *
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
 *      Appends source file and line number to message for DEBUG compiliation.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       11-Jan-1989.    Created.
 *      H. S. Fear          13-Nov-1990.    enlarged msg buffers to handle
 *                                          test requirements
 *      J. A. Wishner       20-Oct-1991.    Delete st_src; unused.
 *      E. A. Alongi        06-Aug-1992     Increase message buffer to 1024.
 *      Mike Williams       01-Jun-2010     Included cl_log.h to remedy warning.
 *      
 *     
 */


/*
 *      Header Files:
 */
#include <stdio.h>
#include <string.h>
#include "cl_pub.h"
#include "cl_log.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */
#define FAILED_LABEL "failed:"

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_logevent()";


/*
 *      Procedure Type Declarations:
 */

void 
csi_logevent (
    STATUS status,                /* status code */
    char *msg,                   /* message */
    char *caller,                /* name of function calling csi_logevent() */
    char *failed_func,           /* name of funcion experiencing error */
    char *source_file,           /* name of C source file for caller */
    int source_line           /* line number in caller calling from */
)
{


    char        cvt[1024];       /* miscellaneous message conversion buffer */

#ifdef DEBUG
    char        dbg[512];       /* debug message conversion buffer */

    /* get source file and line number for message */
    sprintf(dbg, "\nsource %s; line %d", source_file, source_line); 

    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 6,                             /* parameter count */
                 (unsigned long) status,        /* argument list */
                 (unsigned long) msg,           /* argument list */
                 (unsigned long) caller,        /* argument list */
                 (unsigned long) failed_func,   /* argument list */
                 (unsigned long) source_file,   /* argument list */
                 (unsigned long) source_line);  /* argument list */
#endif /* DEBUG */

    if (STATUS_SUCCESS != status) {

        /* caller, status, failed_func, message */
        sprintf(cvt, "%s: %s: status:%s; %s %s\n%s;", 
            CSI_LOG_NAME,
            caller, 
            cl_status(status), 
            (NULL == failed_func || '\0' == *failed_func) ? "" : FAILED_LABEL,
            (NULL == failed_func || '\0' == *failed_func) ? "" : failed_func,
            msg); 
    }
    else {
        
        /* informational message, no status */
        sprintf(cvt, "%s: %s: %s", CSI_LOG_NAME, caller, msg);
    }

#ifdef DEBUG
    /* attach source file and line number to end of message */
    strcat(cvt, dbg);
#endif /* DEBUG */

    /* log the event */
    cl_log_event(cvt);


#ifndef SSI /* csi code */

    /* send rpc status to ACSSA */
    switch (status) {

        case STATUS_RPC_FAILURE:
        case STATUS_NI_TIMEDOUT:
            cl_inform(status, TYPE_SERVER, NULL, 0);
            break;
        default:
            break;

    } /* end of switch on status */

#endif /* SSI */

}

