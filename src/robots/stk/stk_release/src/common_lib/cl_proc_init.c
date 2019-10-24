/**********************************************************************
*
*	C Source:		cl_proc_init.c
*	Subsystem:		2
*	Description:	
*	%created_by:	kjs %
*	%date_created:	Thu Dec  1 14:07:53 1994 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %filespec: cl_proc_init.c,2.1.1 %  (%full_filespec: 2,csrc,cl_proc_init.c,2.1.1 %)";
static char SccsId[] = "@(#) %filespec: cl_proc_init.c,2.1.1 %  (%full_filespec: 2,csrc,cl_proc_init.c,2.1.1 %)";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_proc_init
 *
 * Description:
 *
 *      This module performs the following initializations steps common to
 *      main processes (below the acsss_daemon):
 *
 *        o registers for system signals using the common library signal
 *          handler.  any errors are returned to the caller.
 *
 *        o validates and initializes my_module_type based on input parameter.
 *          STATUS_INVALID_TYPE is returned if type is invalid.
 *
 *        o evaluates command line arguments to determine parent PID, input
 *          socket name and request originator type.  STATUS_INVALID_VALUE is
 *          returned if less than 4 arguments are specified, or a non-numeric
 *          PID is specified, or an invalid requestor is specified.
 *
 *        o sets up a communication path to other processes.  If a path can
 *          not be established, an event is logged and STATUS_PROCESS_FAILURE
 *          is returned.
 *
 *        o sends a SIGHUP to the parent process to inform it that this
 *          process is ready for input.
 *
 *        o return STATUS_SUCCCESS.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_INVALID_TYPE     Invalid module type specified.
 *      STATUS_INVALID_VALUE    Invalid argument count or value.
 *      STATUS_IPC_FAILURE      Failed to create process IPC.
 *      STATUS_PROCESS_FAILURE  Failed to signal parent process.
 *
 * Implicit Inputs:
 *
 *      Command line arguments:
 *
 *          argv[0] - name of caller
 *          argv[1] - parent PID
 *          argv[2] - input socket name
 *          argv[3] - request originator type
 *          argv[4] - restart count (optional)
 *          argv[5] - ACSMT process count (optional - used by ACSLM only))
 *
 * Implicit Outputs:
 *
 *      my_module_type                  - executing module type
 *      my_sock_name[SOCKET_NAME_SIZE]  - module input socket name
 *      requestor_type                  - request originator's module type
 *      trace_value                     - ACSLS trace flag
 *
 * Considerations:
 *
 *      Since this routine creates the communication path to other processes,
 *      including the ACSEL, logging of errors should NOT be done before the
 *      IPC mechanism is created.  This consideration also applies to any
 *      routine invoked by this one.
 *
 *      The recommendation is for a main program to exit with, and a function
 *      to return, the non-successful status returned by this routine.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      D. F. Reed              24-Oct-1988     Original.
 *      J. W. Montgomery        16-Mar-1990.    Added scratch rp stuff.
 *      H. I. Grapek            02-May-1990     Added set_clean code.
 *      J. W. Montgomery        24-May-1990.    Added TYPE_MV
 *      D. L. Trachy            29-Jun-1990.    Added IPC_CLEAN
 *      D. A. Beidle            29-Jun-1990     Added TYPE_SET_CAP.
 *      H. I. Grapek            05-Sep-1990     shrunk ok originator list.
 *      D. A. Beidle            15-Oct-1990     R2 EVT BUG #64, #108, #127 -
 *            Close timing gap where SIGQUIT signals not caught.
 *
 *      D. A. Beidle            28-Jul-1990     Set cl_trace_enter and
 *            cl_trace_volume globals; remove acs_count and port_count;
 *            significant code cleanup.
 *      Alec Sharp              08-Sep-1992     Added set_owner
 *      Alec Sharp              05-Nov-1992     Wait for debugger to be
 *            attached, and set trace_value, using SIGUSR2.
 *      Alec Sharp              19-Nov-1992     Pulled out the code to read
 *            the debugger/trace file and put it in cl_debug_trace.c
 *      Alec Sharp	        26-Jul-1993     Removed code associated with
 *            cl_trace_enter and cl_trace_volume. This tracing is handled
 *            using dynamic variables now.
 *      Alec Sharp		24-Aug-1993     Register the logging functions.
 *      D. B. Farmer        30-Sep-1993     Replace signal() with sigAction().
 *	Emanuel A. Alongi	18-Oct-1993.	Added TYPE_SSI.
 *      Ken Stickney            02-Feb-1994     Put NOT_CSC ifdef around 
 *                                              cl_debug_trace() calls, so
 *                                              func not shipped with ACSLS
 *                                              CDK.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <signal.h>                     /* ANSI-C compatible */
#include <stdio.h>                      /* ANSI-C compatible */
#include <string.h>                      /* ANSI-C compatible */
#include <stdlib.h>                     /* ANSI-C compatible */
#include <errno.h>
#include <unistd.h>

#include "cl_pub.h"
#include "cl_ipc_pub.h"
#include "ml_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

static  char   *self = "cl_proc_init";

/*
 *      Procedure Type Declarations:
 */

static  void    st_sigusr2(int signal_number);


STATUS 
cl_proc_init (
    TYPE mod_type,                       /* calling module type */
    int argc,                           /* command line arg count from main() */
    char **argv                           /* command line arg ptr from main() */
)
{
    int     ppid;                       /* parent PID */
    STATUS  ret;
    char   *s;
    struct sigaction	action;		/* needed for calls to sigaction() */

    trace_value = 0;

    /* Register the logging functions for sending messages to the
       event logger.. This should be done before any possiblity of
       calling cl_log* or cl_trace. NOTE: One day we may want to pull
       this code out from here and put in at the start of each process's
       main function to give maximum flexibility when splitting common_lib */

    cl_el_log_register();
    
    /* register for signals */
    cl_sig_trap(cl_sig_hdlr, 2, SIGQUIT, SIGTERM);

    /* check module type */
    switch (mod_type) {
      case TYPE_AUDIT:
      case TYPE_CM:
      case TYPE_CP:
      case TYPE_CSI:
      case TYPE_DEFINE_POOL:
      case TYPE_DELETE_POOL:
      case TYPE_EJECT:
      case TYPE_EL:
      case TYPE_ENTER:
      case TYPE_IPC_CLEAN: 
      case TYPE_LH:
      case TYPE_LM:
      case TYPE_LOCK_SERVER:
      case TYPE_MT:
      case TYPE_QUERY:
      case TYPE_RECOVERY:
      case TYPE_SA:
      case TYPE_SET_CAP:
      case TYPE_SET_CLEAN:
      case TYPE_SET_OWNER:
      case TYPE_SET_SCRATCH:
      case TYPE_SSI:
      case TYPE_SV:
      case TYPE_VARY:

        /* set global module type */
        my_module_type = mod_type;
        break;

      /* anything else is an error */
      default:
        return(STATUS_INVALID_TYPE);
    }
    
    /* time to look at the calling args */
    /* argc first, must be 4 or 5 */
    if (argc < 4)
        return(STATUS_INVALID_VALUE);

    /* parent PID */
    if ((ppid = atoi(*++argv)) <= 0)
        return(STATUS_INVALID_VALUE);

    /* input socket name */
    s = *++argv;

    /* request originator type */
    requestor_type = (TYPE)atoi(*++argv);
    switch(requestor_type) {
      case TYPE_CM:                     /* server initiation only */
      case TYPE_CSI:
      case TYPE_CP:
      case TYPE_EL:                     /* server initiation only */
      case TYPE_IPC_CLEAN:              /* server initiation only */
      case TYPE_LH:                     /* server initiation only */
      case TYPE_LM:
      case TYPE_LOCK_SERVER:            /* server initiation only */
      case TYPE_MT:                     /* server initiation only */
      case TYPE_RECOVERY:
      case TYPE_SA:
      case TYPE_SV:                     /* server initiation only */
        break;

      /* anything else is an error */
      default:
        return(STATUS_INVALID_VALUE);
    }

    /* get restart count */
    if (argc > 4)
        restart_count = atoi(*++argv);

    /* set up IPC */
    if ((ret = cl_ipc_create(s)) != STATUS_SUCCESS)
        return(ret);

    /* IPC mechansim established, logging via ACSEL can commence */

    /* signal the parent process ... unless CP */
    if (my_module_type != TYPE_CP) {
        if (kill(ppid, SIGHUP) < 0) {

            /* whoops, log an error */
            MLOG((MMSG(124,"%s: Signaling process %d with %s failed on \"%s\""), self,  ppid,
                                    "SIGHUP", strerror(errno)));
            return(STATUS_PROCESS_FAILURE);
        }
    }



#ifdef DEBUG
    
   /* If instructed to do so, pause and wait for the debugger to be
       attached.  We start up again when receive a signal (any signal
       will do, including simply attaching the debugger, but SIGUSR2
       is set up to handle this. We have to do this after sending the
       SIGHUP to the parent otherwise the parent will time out.  */
 

    /* setup for call to sigaction() */
    action.sa_handler = (SIGFUNCP) st_sigusr2;
    (void) sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    /* SA_RESTART is a sigaction() flag (sa_flags) defined on most platforms.   
     * This flag causes certain "slow" systems calls (usually those that can
     * block) to restart after being interrupted by a signal.  The SA_RESTART
     * flag is not defined under SunOS because the interrupted system call is
     * automatically restarted by default.  So, in order to make it possible
     * to write one version of the sigaction() code, SA_RESTART is defined in
     * defs.h for platforms running SunOS.
     */
    action.sa_flags |= SA_RESTART;

    (void)sigaction(SIGUSR2, &action, NULL);
    
#ifdef NOT_CSC
    if (cl_debug_trace ())
	(void) pause();
#endif /* not a client system component- CSC */

    /* To get to the actual process that you want to start debugging,
       the easiest way is to type 'up', set a breakpoint at the place
       you want to start, then type 'continue'.
         For RPs,
           up -> cl_rp_init
	   up -> component process main()
	 For PPs,
	   up -> component process main()
    */   
#endif /* DEBUG */
    
    
    return(STATUS_SUCCESS);
}


/* -----------------------------------------------------------------------
 *
 * Name:
 *
 *   st_sigusr2
 *
 * Description:
 *
 *   This function is the signal handler for SIGUSR2.  It reads the
 *   file associated with my_module_type and resets the trace value
 *   as appropriate.
 *
 * Return Values:      NONE (void)
 *
 * Parameters:         NONE
 *   
 * Implicit Inputs:    NONE
 *
 * Implicit Outputs:   NONE
 *
 * Considerations:     The function ignores then resets the signal to
 *                     emulate SRV4 signal handling.
 *
 */

#ifdef DEBUG

/*lint -esym(715,signal_number)  Tell lint to ignore the unused parameters*/
static void 
st_sigusr2 (
    int signal_number
)
{
    /* Now call cl_debug_trace to get the new trace value. We don't
       care about the return because we aren't going to pause again */

#ifdef NOT_CSC
    (void) cl_debug_trace();
#endif /* not a client system component- CSC */
}
/*lint -restore   Restore any lint flags that we suppressed. */

#endif /* DEBUG */
