static char SccsId[] = "@(#)cl_el_log.c	5.3 10/14/93 (c) 1993 StorageTek";
/*
 * Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 *
 * Description:
 *
 *  This file contains functions that send a log message to the Event
 *  Logger process.
 *
 * Revision History:
 *
 *  Alec Sharp       	22-Sep-1993  Original. Based on cl_log_event,
 *          cl_log_trace, cl_trace.
 *  Alec Sharp          14-Oct-1993  Code review changes.
 *  Mike Williams       27-May-2010  Included prototype for dv_get_string by
 *                                   defining NOT_CSC.
 *
 */

/* ----------- Header Files -------------------------------------------- */

#include "flags.h"
#include "system.h"

#include <stdio.h> 
#include <string.h>
#include <time.h>
#include <stddef.h>

#include "cl_pub.h"
#include "cl_ipc_pub.h"
#include "dv_pub.h"


/* ----------- Defines, Typedefs and Structure Definitions ------------- */

/* ----------- Global and Static Variable Declarations ----------------- */

/* ----------  Procedure Declarations ---------------------------------- */

static void st_send_log_msg (const char *cp_msg, LOG_OPTION log_option);

    

/*--------------------------------------------------------------------------
 *
 * Name: cl_el_log_event
 *
 * Description:
 *
 *      This function takes a message and logs it to the event log.
 *      Note that it handles recursive calls to itself.
 *   
 * Return Values:	NONE
 *
 * Parameters:		cp_msg	    Pointer to string to log.
 *
 * Implicit Inputs:	NONE
 *
 * Implicit Outputs:	NONE
 *
 */

void 
cl_el_log_event (const char *cp_msg)
{
    static BOOLEAN      B_already_in_function = FALSE;
    
    /* Avoid recursive calls  */
    if (B_already_in_function)
	return;
    B_already_in_function = TRUE;
    
    st_send_log_msg (cp_msg, LOG_OPTION_EVENT);
    
    B_already_in_function = FALSE;
}


/*--------------------------------------------------------------------------
 *
 * Name: cl_el_log_trace
 *
 * Description:
 *
 *      This function takes a message and logs it to the trace log.
 *      Note that it handles recursive calls to itself.
 *   
 * Return Values:	NONE
 *
 * Parameters:		cp_msg	    Pointer to string to log.
 *
 * Implicit Inputs:	NONE
 *
 * Implicit Outputs:	NONE
 *
 */

void 
cl_el_log_trace (const char *cp_msg)
{
    static BOOLEAN      B_already_in_function = FALSE;
    
    /* Avoid recursive calls  */
    if (B_already_in_function)
	return;
    B_already_in_function = TRUE;
    
    st_send_log_msg (cp_msg, LOG_OPTION_TRACE);
    
    B_already_in_function = FALSE;
}



/*--------------------------------------------------------------------------
 *
 * Name: st_send_log_msg
 *
 * Description:
 *
 *   This function is a common function for cl_el_log_event and cl_el_log_trace.
 *   It attempts to send the message to the event logger. If the send fails,
 *   it logs the message to stderr.
 *
 * Return Values:	NONE
 *
 * Parameters:		cp_msg		Pointer to the message to send
 *			log_option 	Option which specifies whether to
 *                                      log in the event log or the trace log.
 *
 * Implicit Input:      request_id      ID of the request.
 *
 */

static void
st_send_log_msg (const char *cp_msg, LOG_OPTION log_option)
{
    int         	msglen;             /* message length         */
    char        	ca_timestamp[MAX_LINE_LEN]; /* timestamp buffer    */
    char        	ca_timefmt[MAX_LINE_LEN];   /* time format buffer  */
    time_t      	timestamp;          /* current time of day    */
    EVENT_LOG_MESSAGE	log_msg;            /* Message structure      */
    STATUS      	status;
    
    /* Clear out message buffer area then fill in. */
    memset ((char *)&log_msg, '\0', sizeof(EVENT_LOG_MESSAGE));
    log_msg.ipc_header.ipc_identifier = request_id;
    log_msg.log_options = log_option;
    strcpy(log_msg.event_message, cp_msg);
   
    
    /* Calculate message length. Include the final '\0' in the message. */
    /*lint -e545 Ignore message generated from  offsetof */
    msglen = offsetof (EVENT_LOG_MESSAGE, event_message) +
	strlen (log_msg.event_message) + 1;
    /*lint +e545 */

    
    /* send message to event logger */
    status = cl_ipc_send(ACSEL, (char *)&log_msg, msglen, MAX_RETRY / 2);
    if (status != STATUS_SUCCESS) {
	/* route message to stderr if IPC failure */
	if (dv_get_string (DV_TAG_TIME_FORMAT, ca_timefmt) != STATUS_SUCCESS)
	    strcpy (ca_timefmt, DEFAULT_TIME_FORMAT);
	(void) time(&timestamp);
	strftime(ca_timestamp, sizeof(ca_timestamp),
		 ca_timefmt, localtime(&timestamp)); 
	fprintf(stderr, "\n%s %s[%d]:\n%s", ca_timestamp,
		cl_type(my_module_type), request_id,
		log_msg.event_message);
    }
}


/* ---------------------------------------------------------------
 *
 * Name:  cl_el_trace
 *
 * Description:
 *
 *      Common routine used to send trace messages to the event logger
 *      process. 
 *      Note that it handles recursive calls to itself.
 *
 * Return Values:	NONE
 *
 * Parameters:		cp_msg	    Pointer to string to log.
 *
 * Implicit Inputs:	request_id  ID of the request.
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 *      This routine calls cl_ipc_send to send messages and therefore
 *      must avoid recursive calls.
 *
 */

void 
cl_el_trace (const char *cp_msg)
{
    static BOOLEAN      B_already_in_function = FALSE;

    int 		len;
    EVENT_LOG_MESSAGE	log_msg;
    
    /* Avoid recursive calls  */
    if (B_already_in_function)
	return;
    B_already_in_function = TRUE;
    
    /* Clear out message buffer area then fill in. */
    memset ((char *)&log_msg, '\0', sizeof(EVENT_LOG_MESSAGE));
    log_msg.ipc_header.ipc_identifier = request_id;
    log_msg.log_options = LOG_OPTION_TRACE;
    strcpy (log_msg.event_message, cp_msg);
    
    /* Calculate message length. Include the final \0 in the message. */
    /*lint -e545 Ignore message generated from  offsetof */
    len = offsetof (EVENT_LOG_MESSAGE, event_message) +
	strlen (log_msg.event_message) + 1;
    /*lint +e545 */

    /* Send message and ignore errors */
    (void)cl_ipc_send(ACSEL, &log_msg, len, MAX_RETRY / 2);
    
    B_already_in_function = FALSE;
}
