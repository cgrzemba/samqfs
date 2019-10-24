static char SccsId[] = "@(#)cl_sig_trap.c	5.7 11/15/93 ";
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_sig_trap
 *
 * Description:
 *
 *      This module provides a consistent method for all ACSLS programs to
 *      trap signals.  It ignores all system signals except those that can't
 *      be trapped (SIGCONT, SIGKILL, SIGSTOP), those that should not be
 *      trapped (SIGBUS, SIGILL, SIGSEGV, SIGSYS), and those that need to be
 *      trapped (user specified).  If the caller doesn't supply a signal 
 *      handler, the common library signal handler is used.
 *
 *      A variable number of signals can be specified for processing by the
 *      signal handler.  The user-specified list is stripped of all signals
 *      that can't or shouldn't be trapped.
 *
 *      If the caller desires to trap other signals, or alter system call
 *      interrupt handling, it must be done AFTER invoking this routine.
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
 *      Since this routine can be invoked before the IPC mechanism is created,
 *      logging of errors is not done.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      D. A. Beidle        28-Jul-1990     Original.
 *      D. B. Farmer        30-Sep-1993     Replace signal() with sigAction().
 *	D. A. Myers	    15-Nov-1993     Added SIGTSTP to list of signals
 *					    whose default behavior shouldn't
 *					    be over-ridden
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <signal.h>                     /* ANSI-C compatible */
#include <stdarg.h>
#include <string.h>

#include "cl_pub.h"

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
cl_sig_trap(
    SIGFUNCP    sig_hdlr,                   /* pointer to signal handler */
    int         sig_count,                  /* number of signals to trap */
    ...                                     /* variable argument list */
)
{
    va_list     ap;                     /* argument pointer */
    int         i;
    SIGFUNCP    shp = (SIGFUNCP)cl_sig_hdlr;
    int         sig;                    /* signal value */
    SIGFUNCP    sig_list[NSIG];
    struct sigaction	action;		/* needed for calls to sigaction() */

    /* setup for call to sigaction() */
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

    /* did we get a handler? */
    if (sig_hdlr)
        shp = sig_hdlr;

    /* make flexelint happy */
    memset ((char *)sig_list, '\0', sizeof (sig_list));

    /* set up signals to be ignored */
    for (i=0; i<NSIG; i++)
	sig_list[i] = SIG_IGN;

    /* filter signal list */
    if (sig_count > 0) {
        va_start(ap, sig_count);
	for (i = 0; i < sig_count; i++) {
	    sig = va_arg(ap, int);
	    if (sig > 0 && sig < NSIG)
		sig_list[sig] = shp;
	}
        va_end(ap);
    }

    /* register all signals */
    for (sig = 1; sig < NSIG; sig++) {
        switch (sig) {

          /* signals that can't be ignored. Ignore what we put in the array */
          case SIGCONT:
          case SIGKILL:
          case SIGSTOP:
            break;

          /* signals that should't be ignored. Use the default action */
	  case SIGBUS:
	  case SIGCHLD:
	  case SIGILL:
	  case SIGSEGV:
	  case SIGSYS:
	  case SIGTSTP:
	    action.sa_handler = (SIGFUNCP) SIG_DFL;
	    (void)sigaction(sig, &action, NULL);
            break;

	  /* signals that are trapped */
          default:
	    action.sa_handler = (SIGFUNCP) sig_list[sig];
	    (void)sigaction(sig, &action, NULL);
            break;
        }
    }
}
