static char SccsId[] = "@(#)csi_main.c      5.6 11/12/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      main()
 *
 * Description:
 *      Main driver for the CSI.
 *
 *      o Initialize ipc environment.
 *      o Set trace_module to TRACE_CSI.
 *      o If conditionally compiled for -DADI:
 *          - fork the CSI co-process and do the following for the child:
 *              initialize co-process
 *              loop (infinitely) reading input from the OSLAN network.
 *      o Initialize CSI process.
 *      o Initiate main processing loop.
 *      o Terminate.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      Implicit imputs dictated by cl_proc_init().  See this routine for
 *      global variables that are set by it and its callee's.
 *
 * Module Test Plan:
 *
 *      See CSI Unit Test Plan
 *
 * Revision History:
 *
 *      J. A. Wishner       01-Jan-1989     Created.
 *      J. A. Wishner       06-Jun-1989     Delete extraneous logging call after
 *                                          call to csi_shutdown.
 *      J. W. Montgomery    14-Jun-1990     Version 2.
 *      E. A. Alongi        12-Oct-1993     Replaced SSI type from command line
 *                                          with TYPE_SSI.
 *      E. A. Alongi        12-Nov-1993     Corrected all flint detected errors.
 *      Joseph Nofi         15-Jun-2011     XAPI support;
 *                                          Add GLOBAL_SRVCOMMON_SET invocation.
 *
 */


/*      Header Files: */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"
#include "system.h"
#include "srvcommon.h"

#ifdef ADI
#include <signal.h>
#endif

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static  char *st_module = "csi_main()";


/*      Procedure Type Declarations: */
#undef SELF
#define SELF "csi_main"

int 
main (
    int  argc,    /* number of command line arguments */
    char **argv    /* arguments (1)=input socket (2) "trace" (3) module type */
)
{
    STATUS       s = STATUS_SUCCESS;
    TYPE         type;
    char         typeString[9];
    int          i;

#ifdef ADI
    int          pid;
#endif /* ADI */

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, argc, (unsigned long) argv);
#endif /* DEBUG */

#ifdef SSI
    type = TYPE_SSI;
    strcpy(typeString, "SSI");
#else  
    type = TYPE_CSI;       
    strcpy(typeString, "CSI");
#endif /* SSI */

    GLOBAL_SRVCOMMON_SET(typeString);

    TRMSG("Entered; type=%s (%d), argc=%d\n",
          typeString,
          type,
          argc);

    for (i = 0;
        i < argc;
        i++)
    {
        TRMSG("arg[%d]=%s\n",
              i,
              argv[i]);
    }

    if (cl_proc_init(type, argc, argv) != STATUS_SUCCESS) {
        exit(1);
    }

    trace_module = TRACE_CSI; /* global common library owned trace module */

#ifdef ADI
    if ((pid = fork()) == 0) {
        /* Child - CSI/SSI co-process for OSLAN */
        (void) csi_netinput();
        exit(0);
    } else {
        /* Parent - Store co-process pid so it can be killed in csi_shutdown() */
        csi_co_process_pid = pid;
    }
#endif /* ADI */

    if ((s = csi_init()) != STATUS_SUCCESS) {
#ifdef ADI
        /* Send SIGHUP to the co-process */
        (void) kill(csi_co_process_pid, SIGHUP);
#endif /* ADI */
        MLOGCSI((s,  st_module,  "main()", 
      MMSG(1021, "Initiation of CSI Failed")));
        exit(1);
    }
    csi_process(); /*  main processing function */
    csi_shutdown(); /* perform shutdown of the CSI */
    exit (0);
}
