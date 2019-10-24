static char SccsId[] = "@(#)csi_sighdlr.c	5.5 11/11/93 (c) 1992 StorageTek";
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 * Name:
 *
 *      csi_sighdlr()
 *
 * Description:
 *      signal handling routine for the CSI
 *      SIGTERM - clean up the CSI process and exit.
 *
 *      The following signals may also be seen for OSLAN:
 *      SIGCLD  - clean up the CSI process and exit.
 *      SIGHUP  - exit the CSI co-process.
 *
 *
 * Return Values:
 *      SUCCESS         description of value.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       05-Jan-1989.    Created.
 *      J. W. Montgomery    03-Oct-1990.    Modified for OSLAN.
 *      J. A. Wishner       20-Oct-1991.    Delete s, cvt; unused.
 *      E. A. Alongi        05-Aug-1992     Mods to catch user signal
 *                                          for csi_trace_flag toggle.
 *	D. B. Farmer	    17-Aug-1993	    Changes for bull port
 *					    handle SIGPIPE
 *      E. A. Alongi        28-Sep-1993     Replace signal() with sigaction().
 *      Mike Williams       01-Jun-2010     Included stdlib.h to remedy warning.
 */

/*      Header Files: */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"
#include "system.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static  char    *st_src = __FILE__;
static  char    *st_module = "csi_sighdlr";

/*      Procedure Type Declarations: */

void 
csi_sighdlr (
    int sig                            /* value of signal that was trapped */
)
{

    struct sigaction	action;		/* needed for calls to sigaction() */

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, 1, (unsigned long) sig);
#endif /* DEBUG */

#ifdef ADI
        adiclose(csi_adi_ref);          /* Close ADI connection */
#endif /* ADI */


    switch (sig) {
     case SIGTERM:
        csi_shutdown();                 /* shut down the csi */
        break;

#ifdef ADI                      /* These signals only possible for OSLAN */
     case SIGCLD:
        /* CSI co-process died */
        csi_shutdown();                 /* shut down the csi */
        break;
     case SIGHUP:
        /* CSI parent process died */
        break;
#endif /* ADI */

      /* toggle packet tracing */
     case SIGUSR1:

         /* toggle: if it's on, turn if off; if it's off, turn it on. */
         csi_trace_flag = (csi_trace_flag) ? FALSE : TRUE;
	     return;

     case SIGPIPE:
	/* writing to closed socket - report error */
	csi_broke_pipe = 1;
	fprintf(stdout, "CSI broken pipe\n");
	return;

     default:
        MLOGCSI((STATUS_SUCCESS, st_module,  "unknown", 
	  MMSG(1025, "Unexpected signal caught, value:%d"), sig));
        return;
    } 


    /* setup for call to sigaction() */

    /* ingore sigterm so doesn't return out of exit() call */
    action.sa_handler = (SIGFUNCP) SIG_IGN;
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

    if (sigaction(SIGTERM, &action, NULL) < 0) {
        fprintf(stdout, 
            "CSI termination error, may require operator intervention\n");
    }

    exit(0); /* terminate the csi */
}
