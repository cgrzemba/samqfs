/* SccsId @(#)acsel.h	1.2 1/11/94  */
#ifndef _ACSEL_
#define _ACSEL_		
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header will include the external data references for the 
 *      ACSEL process.
 *
 * Modified by:
 *
 *      H. L. Freeman IV        23-Nov-1988.    Original.
 *      M. Black                19-Aug-1992     Added MAX_ARCHIVE_FILES (the
 *             maximum number of event log files to archive), MIN_ARCHIVE_FILES,
 *             and LOG_ARCHIVE_TEMPLATE (template for creating archived event
 *             logs using sprintf).  Removed current_file_open global (defined
 *             locally to modules instead).
 *	Mitch Black		03-Dec-2004	Commented the text following
 *		the #endif (AIX doesn't like it uncommented).
 */

/*
 *      Header Files:
 */
#include <stdio.h>
#include "structs.h"

/*
 *	Defines, Typedefs and Structure Definitions:
 */
#define EL_SELECT_FOREVER -1L		/* forever timeout on select */
#define EL_SELECT_TIMEOUT (long)(RETRY_TIMEOUT * 2) 
					/* 2 second timeout on select */

#define EL_ERROR_MSG_SIZE 256L		/* limit of EL internal errmsg buffer */
#define FILE_PATHNAME_SIZE 256L		/* Max size of logfile path+name */
#define MIN_LOG_SIZE	32L		/* Min event log size (in Kbytes) */

#define TRACE_DISABLED    0		/* Write nothing to trace file */
#define TRACE_ENABLED    (!TRACE_DISABLED)	/* Write to trace file */

#define EVENT_MSG_WIDTH   78L		/* Max output width of EL msg */
#define INFORM_PERIOD	  60L		/* every 1 minute or 60 seconds inform
					the SA of the event log being full */ 

#define LOG_EVENT_FILE_NAME "acsss_event.log"	
#define LOG_TRACE_FILE_NAME "acsss_trace.log"
#define LOG_ARCHIVE_TEMPLATE "event%d.log"	/* To create event log
						archive file names */

#define MIN_ARCHIVE_FILES	0	/* Min # of event logs to archive */
#define MAX_ARCHIVE_FILES	10	/* Max # of event logs to archive */

/*
 *	Global and Static Variable Declarations:
 */
extern EVENT_LOG_MESSAGE  acsel_input_buffer;   /* el input buffer for msgs */

extern long     event_log_size;		/* max size of event log file */
extern int	event_log_full;		/* indicates event log file full */
extern long	event_log_time;		/* time SA was last informed */
extern long	event_file_time;	/* time SA was last informed */
extern int 	trace_logging_off;	/* flag to stop trace logging */

extern char	event_file[];		/* current event file */ 
extern char	trace_file[];		/* current trace file */
extern char     archive_template[];	/* template for archive files */

extern int      el_terminated;		/* SIGTERM recv'd indicator */
extern long     el_select_timeout;	/* select timeout variable */

extern int      file_num;		/* number of event files to keep */
extern long     el_clock; 		/* Used for system time referencing */

/*
 *      Procedure Type Declarations:
 */
STATUS  el_init();
STATUS  el_input();
STATUS  el_roll_file();
void    el_output();
void	el_format();
void	el_fwrite();
void    el_log_error();
void    el_sig_hdlr();

#endif /* _ACSEL_ */

