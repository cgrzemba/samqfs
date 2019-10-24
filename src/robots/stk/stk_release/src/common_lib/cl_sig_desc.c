static char SccsId[] = "@(#)cl_sig_desc.c	5.2 12/20/93 ";
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *	cl_sig_desc
 *
 * Description:
 *
 *   Function to return a string which describes a signal number.
 *
 * Revision History:
 *
 *	David Farmer	11-Nov-1993  Original.
 *	Hemendra		03-Jan-2002	 Added Linux specific signals.
 *								 SIGIOT = SIGABRT, SIGSTKFLT,
 *								 SIGCLD = SIGCHLD, SIGPOLL = SIGIO,
 *								 SIGUNUSED = SIGSYS?
 *								 The following signals are not applicable for
 *								 Linux.
 *								 SIGEMT & SIGDANGER
 */

/* ----------- Header Files -------------------------------------------- */

#include "flags.h"
#include "system.h"

#include <stdio.h>
#include <sys/signal.h>

/* ----------- Defines, Typedefs and Structure Definitions ------------- */

/* ----------- Global and Static Variable Declarations ----------------- */

/* ----------  Procedure Declarations ---------------------------------- */


    

/*--------------------------------------------------------------------------
 *
 * Name:
 *	cl_sig_desc
 *
 * Description:
 *
 *   This function returns the correct string to describe a passed in
 *   signal number.
 *
 * Return Values:
 *
 *  char * containing description of signal
 *
 * Parameters:
 *
 *     sig		signal to be translated
 *
 * Implicit Inputs:	NONE
 *
 * Implicit Outputs:	NONE
 *
 * Considerations:	NONE
 *
 */


const char *
cl_sig_desc(const int sig)
{
    static char buf[20]; /* buffer for the unknown signal */

    switch (sig) {
      case SIGHUP: 
	return "SIGHUP - hangup, generated when terminal disconnects";
      case SIGINT: 
	return "SIGINT - interrupt, generated from terminal special char";
      case SIGQUIT: 
	return "SIGQUIT - quit, generated from terminal special char";
      case SIGILL: 
	return "SIGILL - illegal instruction (not reset when caught)";
      case SIGTRAP: 
	return "SIGTRAP - trace trap (not reset when caught)";
      case SIGABRT: 
	return "SIGABRT - abort process";
#ifndef LINUX
      case SIGEMT: 
	return "SIGEMT - EMT intruction";
#endif
      case SIGFPE: 
	return "SIGFPE - floating point exception";
      case SIGKILL: 
	return "SIGKILL - kill (cannot be caught or ignored)";
      case SIGBUS: 
	return "SIGBUS - bus error (specification exception)";
      case SIGSEGV: 
	return "SIGSEGV - segmentation violation";
      case SIGSYS: 
	return "SIGSYS - bad argument to system call";
      case SIGPIPE: 
	return "SIGPIPE - write on a pipe with no one to read it";
      case SIGALRM: 
	return "SIGALRM - alarm clock timeout";
      case SIGTERM: 
	return "SIGTERM - software termination signal";
      case SIGURG: 
	return "SIGURG - urgent contition on I/O channel";
      case SIGSTOP: 
	return "SIGSTOP - stop (cannot be caught or ignored)";
      case SIGTSTP: 
	return "SIGTSTP - interactive stop";
      case SIGCONT: 
	return "SIGCONT - continue (cannot be caught or ignored)";
      case SIGCHLD: 
	return "SIGCHLD - sent to parent on child stop or exit";
      case SIGTTIN: 
	return "SIGTTIN - background read attempted from control terminal";
      case SIGTTOU: 
	return "SIGTTOU - background write attempted to control terminal";
      case SIGIO: 
	return "SIGIO - I/O possible, or completed";
      case SIGXCPU: 
	return "SIGXCPU - cpu time limit exceeded";
      case SIGXFSZ: 
	return "SIGXFSZ - file size limit exceeded";
      case SIGWINCH: 
	return "SIGWINCH - window size changed";
      case SIGUSR1: 
	return "SIGUSR1 - user defined signal 1";
      case SIGUSR2: 
	return "SIGUSR2 - user defined signal 2";
      case SIGPROF: 
	return "SIGPROF - profiling time alarm";
	
      /* Here are signals that are not always defined. Only include
	 them if they are defined */
#ifdef SIGPWR
      case SIGPWR: 
	return "SIGPWR - power-fail restart";
#endif
#ifdef SIGDANGER
      case SIGDANGER: 
	return "SIGDANGER - system crash imminent; free up some page space";
#endif
#ifdef SIGVTALRM
      case SIGVTALRM: 
	return "SIGVTALRM - virtual time alarm";
#endif
	
      /* Here are the machine specific signals that are unlikely
	 to exist on other machines.  */
#ifdef AIX
      case SIGMSG: 
	return "SIGMSG - input data is in the HFT ring buffer";
      case SIGMIGRATE: 
	return "SIGMIGRATE - migrate process";
      case SIGPRE: 
	return "SIGPRE - programming exception";
      case SIGVIRT: 
	return "SIGVIRT - AIX virtual time alarm";
      case SIGGRANT: 
	return "SIGGRANT - HFT monitor mode granted";
      case SIGRETRACT: 
	return "SIGRETRACT - HFT monitor mode should be relinguished";
      case SIGSOUND: 
	return "SIGSOUND - HFT sound control has completed";
      case SIGSAK: 
	return "SIGSAK - secure attention key";
#endif /* AIX */

#ifdef LINUX
	  case SIGSTKFLT:
	return "SIGSTKFLT - Stack fault.";
#endif		

      default:
	sprintf (buf, "Unknown signal %d", sig);
	return buf;
    }
}
