#ifndef lint
static char SccsId[] = "@(#) %full_name:  1/csrc/t_parent.c/2 %";
#endif
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 * Name:
 *
 *      t_parent.c (main)
 *
 * Description:
 *
 *      Process used to simulate the functionality of the acsss daemon, for
 *      test purposes.  When started up, it prints its own process number
 *      on stdout.  This process number is used as input to other storage
 *      server processes requiring a parent pid at which to direct a
 *      hangup signal (which a basic requirement for the acsss daemon).
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
 *      Process id of this parent process is placed on standard output.
 *
 * Considerations:
 *
 *      Start this up in the background.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       10-Jan-1989. Borrowed from Howard Freeman.
 *	H. I. Grapek	15-Oct-1992	Changed siginterrupt() to kill().
 *					This is ANSI and SYSV compliant.
 *      Mike Williams   02-Jun-2010     Changed main return type to int.
 *                                      Included unistd.h to remedy warnings.
 *
 *
 */

/*
 *      Header Files:
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "../h/lm_structs.h"
#include "../h/acsel.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
int 	parent_pid;
int 	ret_pid;
int 	pid;
union 	wait *wstatus;
struct 	rusage *rusage;
STATUS 	status;

/*
 *      Procedure Type Declarations:
 */
void sighdlr();

int main()
{
    signal(SIGHUP, sighdlr);
    signal(SIGTERM, sighdlr);
    signal(SIGQUIT, sighdlr);
    signal(SIGCHLD, sighdlr);

    kill(getpid(),SIGHUP);

    /* parent process */

    parent_pid = getpid();
    printf("Parent Process ID is: %d \n", parent_pid);

    /* wait for sighup signal first */
    pause();

    printf("Parent Process #%d EXITING NORMALLY\n", parent_pid);
}

/*******************************************************************************
 * Module name : sighdlr()
 *
 * Module desription: This module will handle the rearming of the signals after
 *                    it has been call. It will call the modules needed
 *                    depending on the signal.
 *
 * Inputs: signum
 * Output: none
 * Return: non
 *
 ******************************************************************************/

void 
sighdlr(signum)
    int signum;
{
    signal(signum, sighdlr);

    switch (signum) {

	case SIGHUP:
	    signal(SIGHUP, sighdlr);
	    /*
	     * when sighup is captured, send the request packet to the
	     * process
	     */
	    printf("SIGHUP received \n");
	    break;

	case SIGQUIT:
	    signal(SIGQUIT, sighdlr);
	    break;

	case SIGTERM:
	    signal(SIGTERM, sighdlr);
	    break;

	case SIGCHLD:
	    signal(SIGCHLD, sighdlr);
	    break;

	default:
	    break;
    }
}

