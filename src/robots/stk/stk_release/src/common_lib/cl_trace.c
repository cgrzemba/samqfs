static char SccsId[] = "@(#)cl_trace.c	5.4 10/14/93 ";
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Description:
 *
 *   This file contain software to log trace information in the form of
 *   a variable list of unsigned longs.
 *
 * Revision History:
 *
 *   D. F. Reed              05-Oct-1988     Original.
 *   D. A. Beidle            06-Sep-1989     Modified to use <stdarg.h>
 *                      which is more portable to a SPARC environment.
 *   D. A. Beidle            22-Nov-1989     Modified to include request_id
 *                      in ipc_header so messages from different processes can
 *                      be distinguished in the trace log.
 *   Alec Sharp              24-Sep-1993     R5.0 Split to allow use of
 *                      a registered logging function.
 *   Alec Sharp              14-Oct-1993     Code review changes.
 *
 */

/* ----------- Header Files -------------------------------------------- */

#include "flags.h"
#include "system.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

#include "structs.h"


/* ----------- Defines, Typedefs and Structure Definitions ------------- */

/* ----------- Global and Static Variable Declarations ----------------- */

static void (*p_Function)(const char *) = NULL;


/* ----------  Procedure Declarations ---------------------------------- */


    

/*--------------------------------------------------------------------------
 *
 * Name: cl_trace
 *
 * Description:
 *
 *   Common routine used to format trace information in the form of
 *   a variable list of unsigned longs, and send it to the trace log.
 *   Formats output lines as follows:
 *
 *      routine_name: 9999999999 9999999999 9999999999 9999999999
 *                    9999999999    etc.
 *   
 * Return Values:	NONE
 *
 * Parameters:		rtn_name	Routine name of caller
 *
 *                      parm_cnt        Number of parameters in list
 *
 *                      ...             List of unsigned longs
 *
 * Implicit Inputs:	p_Function	Pointer to registered logging function
 *
 * Implicit Outputs:	NONE
 *
 */

void 
cl_trace (
    char       *rtn_name, 
    int         parm_cnt,
    ...                  
)
{
    va_list     ap;                     /* argument pointer */
    char       *cp;
    int         i;
    char 	ca_message[MAX_MESSAGE_SIZE];	/* Buffer to build message in */
   

    va_start(ap, parm_cnt);

    sprintf(ca_message, "%-32.32s:", rtn_name);

    /* format calling arg list */
    cp = &ca_message[strlen(ca_message)];
    for (i = 0; i < parm_cnt; i++) {
        if (i && ((i % 4) == 0)) {

            /* break and add newline */
            *cp++ = '\n';
	    memset (cp, ' ', 33);
	    cp += 33;
        }
        sprintf(cp, " %08lx", va_arg(ap, unsigned long));
        cp += strlen(cp);
    }
    va_end(ap);

    /* add trailing newline */
    *cp++ = '\n';
    *cp++ = '\0';

    /* We've built the string. Now either print it or ship it off
       to the logging function */
    
    if (p_Function != NULL)
	p_Function (ca_message);
    else
	fprintf (stderr, ca_message);
}


/*--------------------------------------------------------------------------
 *
 * Name: cl_trace_register
 *
 * Description:
 *
 *   This function allows the caller to register a function for logging
 *   trace messages.
 *
 * Return Values:	NONE
 *
 * Parameters:
 *
 *   funcptr		Pointer to a void function which has a single
 *			const char * parameter. I.e.
 *	                    void (*funcptr)(const char *)
 *
 * Implicit Inputs:	NONE
 *
 * Implicit Outputs:
 *
 *   p_Function		Module scope global function pointer.
 *
 */

void cl_trace_register (void (*funcptr)(const char *))
{
    p_Function = funcptr;
}




/* -------------- Testing function - compile with -DTESTMAIN ------------ */
#ifdef TESTMAIN

int main(int argc, char *argv[])
{
    char *rtn_name = "test";

    fprintf (stderr, "\nTesting cl_trace with no registered function\n");

    cl_trace (rtn_name, 0);
    cl_trace (rtn_name, 1, 1);
    cl_trace (rtn_name, 2, 1, 2);
    cl_trace (rtn_name, 3, 1, 2, 3);
    cl_trace (rtn_name, 4, 1, 2, 3, 4);
    cl_trace (rtn_name, 5, 1, 2, 3, 4, 5);
    cl_trace (rtn_name, 6, 1, 2, 3, 4, 5, 6);
    cl_trace (rtn_name, 7, 1, 2, 3, 4, 5, 6, 7);
    cl_trace (rtn_name, 8, 1, 2, 3, 4, 5, 6, 7, 8);
    cl_trace (rtn_name, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    cl_trace (rtn_name, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
       
    fprintf (stderr, "---------------------------------------------\n\n");
    
    fprintf (stderr, "\nTesting cl_trace with registered function.\n"
	     "Should see nothing since IPC mechanism not active.\n");
    
    cl_el_log_register();

    cl_trace (rtn_name, 0);
    cl_trace (rtn_name, 1, 1);
    cl_trace (rtn_name, 2, 1, 2);
    cl_trace (rtn_name, 3, 1, 2, 3);
    cl_trace (rtn_name, 4, 1, 2, 3, 4);
    cl_trace (rtn_name, 5, 1, 2, 3, 4, 5);
    cl_trace (rtn_name, 6, 1, 2, 3, 4, 5, 6);
    cl_trace (rtn_name, 7, 1, 2, 3, 4, 5, 6, 7);
    cl_trace (rtn_name, 8, 1, 2, 3, 4, 5, 6, 7, 8);
    cl_trace (rtn_name, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    cl_trace (rtn_name, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    
    fprintf (stderr, "---------------------------------------------\n\n");

    return (0);
}

#endif
