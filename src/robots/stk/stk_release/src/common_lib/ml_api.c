#ifndef lint
static char *_csrc = "@(#) %filespec: ml_api.c,6 %  (%full_filespec: ml_api.c,6:csrc:1 %)";
#endif
/*
 * Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 * Description:
 *
 *   This file contains functions to log messages. 
 *   This function is the one for the clients.
 *
 * Revision History:
 *
 *   Alec Sharp       30-Oct-1993  Original.
 *   David Farmer     23-Nov-1993  extracted ml_log_event from ml_log
 *   David Farmer     30-Nov-1993  added #include <stdarg.h>
 *   Howie Grapek     15-Dec-1993  New file specificly for the CSC 
 *				   Developer's Toolkit.... renamed to ml_api.h 
 *				   from ml_pub.h... need to keep this one
 *				   with ml_api.c.
 *   Mitch Black      20-Jun-2003  (Linux port) Zero out ca_output in several places
 *   Mitch Black      02-Dec-2004  Changed ml_api.h reference to ml_pub.h.
 *                       Those files were duplicates, and this change allows
 *                       us to remove ml_api.h from the CDK.
 *   Mitch Black      30-Dec-2004  Add line numbers and other useful info to
 *                       error messages, a la ACSLS messaging.
 *   Mike Williams    01-Jun-1020  Included libgen.h to remove basename warning.
 */

/* ----------- Header Files -------------------------------------------- */

#include "flags.h"
#include "system.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <time.h>
#include "cl_pub.h"

/* These must be defined BEFORE the call to include ml_pub.h */
char   *ml_file_name;    /* File name */
char   *ml_file_id;      /* Sccs ID. Used to get the version number */
int 	ml_line_num;     /* Line number in the source file */

#include "ml_pub.h"


/* ----------- Defines, Typedefs and Structure Definitions ------------- */

#define FAILED_LABEL "failed:"

/* ----------- Global and Static Variable Declarations ----------------- */
 
static void (*p_Output_function)(const char *) = NULL;
  

/* ----------  Procedure Declarations ---------------------------------- */

/*--------------------------------------------------------------------------
 *
 * Name: ml_log_event
 *
 * Description:
 *
 *   This function logs a message using variable arguments.
 *
 * Return Values:	NONE
 *
 * Parameters:		cp_fmt		Message number to look up
 *			...		printf-style arguments
 *   
 * Implicit Inputs:	NONE
 * Implicit Outputs:	NONE
 *
 */

void ml_log_event (char * cp_fmt, ...)
{
    va_list        arg_ptr;
    char     	   ca_output[MAX_MESSAGE_SIZE];
    
    memset(ca_output, '\0', sizeof(ca_output));	/* Start with empty string */
    ml_start_message(ca_output);		/* Prefix filename & line number */

    /* Add message text and variable arguments */
    va_start (arg_ptr, cp_fmt);
    vsprintf (&ca_output[strlen(ca_output)], cp_fmt, arg_ptr);
    va_end (arg_ptr);
    
    ml_output (ca_output);
}


/*--------------------------------------------------------------------------
 *
 * Name: ml_log_unexpected
 *
 * Description:
 *
 *   This function logs an "unexpected result from function call"
 *   message using variable arguments.
 *
 * Return Values:	NONE
 *
 * Parameters:		cp_caller	Calling function name
 *                      cp_callee	Function that gave the error
 *                      status		Status returned from callee
 *                      cp_fmt		Message number to look up
 *			...		printf-style arguments
 *   
 * Implicit Inputs:	NONE
 * Implicit Outputs:	NONE
 *
 */

void ml_log_unexpected (char *cp_caller, char *cp_callee,
			STATUS status, char * cp_fmt, ...)
{
    va_list        arg_ptr;
    char     	   ca_output[MAX_MESSAGE_SIZE];
    char           *cp_status;
    
 
    /* get status code sting */
    cp_status = cl_status(status);
    memset(ca_output, '\0', sizeof(ca_output));	/* Start with empty string */
    ml_start_message(ca_output);		/* Prefix filename & line number */

    /* Add the caller, callee, and the status */
    sprintf(&ca_output[strlen(ca_output)], 
	    "%s: %s() unexpected status = %s",
	    cp_caller, cp_callee, cp_status);
    
    /* Most of the log_unexpected calls will have a message
       number of zero. If there is a specified number, get
       the message and append it to the string. */

    if (cp_fmt != NULL && *cp_fmt != '\0') {
	va_start (arg_ptr, cp_fmt);
	vsprintf (&ca_output[strlen(ca_output)], cp_fmt, arg_ptr);
	va_end (arg_ptr);
    }
    
    ml_output (ca_output);
}

/*--------------------------------------------------------------------------
 * Name: ml_log_csi
 *
 * Description:
 *
 *      Event logging function.
 *
 *      If STATUS_SUCCESS == status (passed in): 
 *      
 *              Format informational message for writing to the event log.
 *
 *      Else (all other status's):
 *      
 *              Format a status message for writing to the event log.
 *      
 *      Calls ml_output to log the message to the event log.
 *
 *      This function is based on csi_logevent.
 */

void 
ml_log_csi (
    STATUS status,               /* status code */
    char *caller,                /* name of function calling this function */
    char *failed_func,           /* name of funcion experiencing error */
    char *cp_fmt,               /* Message string */
    ...                          /* printf-style arguments */
)
{
    va_list        arg_ptr;
    char     	   ca_output[MAX_MESSAGE_SIZE];
    int            i_len;
    
    /* For the time being, since there's no real text string
       associated with the status and name information, we'll just
       keep the format strings hard-coded */
    
    va_start (arg_ptr, cp_fmt);
    memset(ca_output, '\0', sizeof(ca_output));	/* Start with empty string */
    ml_start_message(ca_output);		/* Prefix filename & line number */
    
    if (STATUS_SUCCESS != status) {

	/* Error message with status set */
	sprintf (&ca_output[strlen(ca_output)], "%s: %s: status:%s; %s %s\n", 
	     "ONC RPC", caller, cl_status(status), 
	     (NULL == failed_func || '\0' == *failed_func) ? "" : FAILED_LABEL,
	     (NULL == failed_func || '\0' == *failed_func) ? "" : failed_func);
	vsprintf (&ca_output[strlen(ca_output)], cp_fmt, arg_ptr);
	/* Now add a semicolon to the end for compatibility with 
	   all the existing messages. */
	i_len = strlen(ca_output);
	if (ca_output[i_len - 1] == '\n')
	    i_len--;
	ca_output[i_len++] = ';';
	ca_output[i_len++] = '\n';
	ca_output[i_len] = '\0';
    }
    else {
        
        /* informational message, no status */
        sprintf(&ca_output[strlen(ca_output)], "%s: %s: ",
		"ONC RPC", caller);
	vsprintf (&ca_output[strlen(ca_output)], cp_fmt, arg_ptr);
	strcat(ca_output, "\n"); 	/* hig */
    }
    
    va_end (arg_ptr);


#ifdef DEBUG
    /* attach source file and line number to end of message */
    sprintf (&ca_output[strlen(ca_output)], "source %s; line %d\n",
	     ml_file_name, ml_line_num);
#endif /* DEBUG */
    
    ml_output (ca_output);



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


/*--------------------------------------------------------------------------
 *
 * Name: ml_output
 *
 * Description:
 *
 *   This function outputs the message string. It passes the string to
 *   the registered output function. If there is no registered output
 *   function, it prints the message to stderr.
 *
 * Return Values:	NONE
 *
 * Parameters:		cp_message              A \n terminated message
 *                                              string to log.
 *
 * Implicit Inputs:	p_Output_function	Ptr to function which
 *                                              outputs the string.
 *
 */

void ml_output (const char *cp_message)
{
    if (p_Output_function != NULL) {
	/* We have a logging function */
	p_Output_function (cp_message);
    }
    else {
	/* We don't have a logging function so just dump the message
	   to stderr. */
	
	static BOOLEAN  B_first_time = TRUE;
	static char    *cp_time_fmt;                /* Time format buffer  */
	char            ca_timestamp[MAX_LINE_LEN]; /* Timestamp buffer    */
	time_t          timestamp;                  /* Current time of day */
	
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
	fprintf(stderr, "\n%s\n%s", ca_timestamp, cp_message);
    }
}


/*--------------------------------------------------------------------------
 *
 * Name: ml_output_register
 *
 * Description:
 *
 *   This function allows the caller to register a function for outputting
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
 *   p_Output_function	Module scope global function pointer, pointing to
 *                      a function which will output the event log message.
 *
 */

void ml_output_register (void (*funcptr)(const char *))
{
    p_Output_function = funcptr;
}

/*--------------------------------------------------------------------------
 *
 * Name: ml_start_message
 *
 * Description:
 *   This function creates the first line of the message, printing
 *   the message number, and various pieces of file information.
 *   This was taken from ACSLS 7.1, then simplified to conform to CDK
 *   internals.
 *
 * Return Values:       NONE
 *
 * Parameters:          cp_output               Output buffer.
 *
 * Implicit Inputs:     ml_file_name            Name of source file from
 *                                              which message is being logged.
 *                      ml_line_num             Line number in source file.
 */

void ml_start_message (char *cp_output)
{ 
    /* Create message prefix: file name and line number */
    sprintf (cp_output, "[%s:%d] ", basename (ml_file_name), ml_line_num);
}

