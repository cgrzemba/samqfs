/* SccsId[] = "@(#)cl_log.h	1.2 11/11/93 "; */
#ifndef  _CL_LOG_
#define  _CL_LOG_  1
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Functional Description:
 *
 *    
 *    Defines all common library event logging function signatures. 
 *
 *
 * Modified by:
 *
 *	D.E. Skinner		4-Oct-1988	Original.
 *	D. F. Reed		13-Jan-1989	Added MAX macro (fixed).
 *  Jim Montgomery  02-Aug-1989 Use ALIGNED_BYTES instead of BYTE.
 */


/*
 * Header Files:
 */


/*
 * Defines, Typedefs and Structure Definitions:
 */

/*
 *
 * Function Prototypes
 *
 */

void cl_log_event(char *msg); /* msg - pre-formated, null terminated, */
			      /* text message buffer */


void cl_log_unexpected(char *caller,    /* name of calling routine          */
                       char *callee,    /* name of routine returning error  */
		       STATUS status);  /* status code to include in message */

#endif

