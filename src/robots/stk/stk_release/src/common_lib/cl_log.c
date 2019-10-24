/**********************************************************************
*
*	C Source:		cl_log.c
*	Subsystem:		1
*	Description:	
*	%created_by:	kjs %
*	%date_created:	Thu Dec  1 13:34:11 1994 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %filespec: cl_log.c,1 %  (%full_filespec: 1,csrc,cl_log.c,1 %)";
#endif
static char SccsId[] = "@(#)cl_log.c	1.1 1/10/94 ";
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Description:
 *
 *  This file contains functions to:
 *	Log an event log or trace log message (wrapper functions)
 *      Register functions for doing the actual logging.
 *
 * Revision History:
 *
 *  Alec Sharp       	14-Oct-1993  R5.0 Original.
 *  Howie Grapek	04-Jan-1994  Added "IFDEF NOT_CSC" to remove stuff.
 *
 */

/* ----------- Header Files -------------------------------------------- */

#include "flags.h"
#include "system.h"

#include <stdio.h> 
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cl_pub.h"


/* ----------- Defines, Typedefs and Structure Definitions ------------- */

/* ----------- Global and Static Variable Declarations ----------------- */


/* Module scope global pointers to logging function. These are set by
   calls to cl_log_event_register and cl_log_trace_register. */

static void (*p_Event_function)(const char *) = NULL;
static void (*p_Trace_function)(const char *) = NULL;


/* ----------  Procedure Declarations ---------------------------------- */

static void st_common (void (*p_function)(const char *), 
		       const char *cp_format, va_list arg_ptr);


    
  

/*--------------------------------------------------------------------------
 *
 * Name: cl_log_event
 *
 * Description:
 *
 *   Common routine to log event log messages using variable arguments.
 *
 * Return Values:	NONE
 *
 * Parameters:
 *
 *   cp_msg		Either a format string or a pre-formatted,
 *			null-terminated, text message buffer.
 *   ...		printf-style arguments.
 *
 * Implicit Inputs:
 *
 *   p_Event_function	Module scope global function pointer, pointing to
 *                      a function which will output the event log message.
 *
 * Implicit Outputs:	NONE
 *
 */

void cl_log_event (const char *cp_msg, ...)
{
    va_list     arg_ptr;
    
    va_start (arg_ptr, cp_msg);
    st_common (p_Event_function, cp_msg, arg_ptr);
    va_end (arg_ptr);
}




/*--------------------------------------------------------------------------
 *
 * Name: cl_log_event_register
 *
 * Description:
 *
 *   This function allows the caller to register a function for logging
 *   event log messages.
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
 *   p_Event_function	Module scope global function pointer, pointing to
 *                      a function which will output the event log message.
 *
 */

void cl_log_event_register (void (*funcptr)(const char *))
{
    p_Event_function = funcptr;
}




/*--------------------------------------------------------------------------
 *
 * Name: cl_log_trace
 *
 * Description:
 *
 *   Common routine to log trace log messages. 
 *
 * Return Values:	NONE
 *
 * Parameters:
 *
 *   cp_msg		Either a format string or a pre-formatted,
 *			null-terminated, text message buffer.
 *   ...		printf-style arguments.
 *
 * Implicit Inputs:
 *
 *   p_Trace_function   Pointer to a logging function that the application
 *                      has registered.
 *
 * Implicit Outputs:	NONE
 *
 */

void 
cl_log_trace (const char *cp_msg, ...)
{
    va_list     arg_ptr;
    
    va_start (arg_ptr, cp_msg);
    st_common (p_Trace_function, cp_msg, arg_ptr);
    va_end (arg_ptr);
}



/*--------------------------------------------------------------------------
 *
 * Name: cl_log_trace_register
 *
 * Description:
 *
 *   This function allows the caller to register a function for logging
 *   trace log messages.
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
 *   p_Trace_function	Module scope global function pointer, pointing to
 *                      a function which will output the trace log message.
 *
 */

void cl_log_trace_register (void (*funcptr)(const char *))
{
    p_Trace_function = funcptr;
}

   

/*--------------------------------------------------------------------------
 *
 * Name: st_common
 *
 * Description:
 *
 *   This function creates a message string, adding a final newline if there
 *   is not already one. It then passes the string to the passed in
 *   function. If the function pointer is NULL, it prints the message
 *   to stderr.
 *
 * Return Values:	NONE
 *
 * Parameters:
 *
 *   p_function		Pointer to the function to call with the message
 *                      string. If NULL, print the message to stderr.
 *
 *   cp_format 		Format string for the variable argument list or
 *                      the pre-formatted string. This string may or may
 *                      not have a terminating '\n' character.
 
 *   arg_ptr            Pointer to the start of the variable argument list.
 *
 */

static void st_common (void (*p_function)(const char *), 
		       const char *cp_format, va_list arg_ptr)
{
    char        ca_timestamp[MAX_LINE_LEN];     /* timestamp buffer           */
    char 	ca_message[MAX_MESSAGE_SIZE];	/* Buffer to build message in */
    time_t      timestamp;                	/* current time of day        */
    int         i_len;                          /* Length of message          */
    
    /* Create a string from the variable arguments. */
    vsprintf (ca_message, cp_format, arg_ptr);
    
	
    /* Make sure there's a trailing newline. We do this by replacing
       the '\0' with a '\n' then appending another '\0'  */
    i_len = strlen (ca_message);
    if (ca_message[i_len - 1] != '\n') {
	ca_message[i_len]   = '\n';
	ca_message[++i_len] = '\0';
    }
    
    
    if (p_function != NULL) {
	/* We have a logging function */
	p_function (ca_message);
    }
    else {
	/* We don't have a logging function so just dump the message
	   to stderr. */

	static BOOLEAN B_first_time = TRUE;
	static char *cp_time_fmt;

	/* Start by seeing if there is an externally defined time format.
	   Since we are not using a registered function, we assume that
	   we are not in a full product environment, and that we should
	   use an environment variable rather than a dynamic variable. */

	if (B_first_time) {
	    B_first_time = FALSE;
	    cp_time_fmt = getenv ("TIME_FORMAT");
	}

	/* Get the time and create the timestamp string from the
	   appropriate format */
	(void) time(&timestamp);
	if (cp_time_fmt == NULL)
	    strftime(ca_timestamp, sizeof(ca_timestamp),
		     DEFAULT_TIME_FORMAT, localtime(&timestamp));
	else 
	    strftime(ca_timestamp, sizeof(ca_timestamp),
		     cp_time_fmt, localtime(&timestamp));

	/* Now log the message to stderr */
	fprintf(stderr, "\n%s\n%s", ca_timestamp, ca_message);
    }
}





/* -------------- Testing function - compile with -DTESTMAIN ------------ */
#ifdef TESTMAIN

int main(int argc, char *argv[])
{
    char msg[100];
    
    fprintf (stderr, "\nTesting cl_log_event with no registered function\n");
    
    cl_log_event ("Newline terminated literal\n");
    cl_log_event ("Non-newline terminated literal");
    
    cl_log_event ("Newline terminated <%d> varargs\n", 1);
    cl_log_event ("Non-newline terminated <%d> <%s> varargs", 2, "hello");
    
    sprintf (msg, "%s\n", "Newline terminated <%d> varargs");
    cl_log_event (msg, 1);
    sprintf (msg, "%s", "Non-newline terminated <%d> <%s> varargs");
    cl_log_event (msg, 2, "hello");
    
    fprintf (stderr, "---------------------------------------------\n\n");
    
    fprintf (stderr, "\nTesting cl_log_trace with no registered function\n");
    
    cl_log_trace ("Newline terminated literal\n");
    cl_log_trace ("Non-newline terminated literal");
    
    cl_log_trace ("Newline terminated <%d> varargs\n", 1);
    cl_log_trace ("Non-newline terminated <%d> <%s> varargs", 2, "hello");
    
    sprintf (msg, "%s\n", "Newline terminated <%d> varargs");
    cl_log_trace (msg, 1);
    sprintf (msg, "%s", "Non-newline terminated <%d> <%s> varargs");
    cl_log_trace (msg, 2, "hello");

    fprintf (stderr, "---------------------------------------------\n\n");

    fprintf (stderr, "\nTesting cl_log_event with registered function\n");

    cl_el_log_register();
    
    cl_log_event ("Newline terminated literal\n");
    cl_log_event ("Non-newline terminated literal");
    
    cl_log_event ("Newline terminated <%d> varargs\n", 1);
    cl_log_event ("Non-newline terminated <%d> <%s> varargs", 2, "hello");

    sprintf (msg, "%s\n", "Newline terminated <%d> varargs");
    cl_log_event (msg, 1);
    sprintf (msg, "%s", "Non-newline terminated <%d> <%s> varargs");
    cl_log_event (msg, 2, "hello");
    
    fprintf (stderr, "---------------------------------------------\n\n");
 
    fprintf (stderr, "\nTesting cl_log_trace with registered function\n");

    cl_log_trace ("Newline terminated literal\n");
    cl_log_trace ("Non-newline terminated literal");
    
    cl_log_trace ("Newline terminated <%d> varargs\n", 1);
    cl_log_trace ("Non-newline terminated <%d> <%s> varargs", 2, "hello");

    sprintf (msg, "%s\n", "Newline terminated <%d> varargs");
    cl_log_trace (msg, 1);
    sprintf (msg, "%s", "Non-newline terminated <%d> <%s> varargs");
    cl_log_trace (msg, 2, "hello");

    fprintf (stderr, "---------------------------------------------\n\n");

    return (0);
}

#endif

