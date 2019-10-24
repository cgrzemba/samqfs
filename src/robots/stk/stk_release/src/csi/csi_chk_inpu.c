#ifndef lint
static char SccsId[] = "@(#)csi_chk_inpu.c	5.4 11/9/93 ";
#endif
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_chk_input (aka: cl_chk_input)
 *
 * Description:
 *
 *                             ** IMPORTANT **
 *
 *          This CSI/SSI routine is a direct replacement for the common
 *          library routine cl_chk_input() and therefore MUST have the
 *          same entry point name, parameters and return values as
 *          cl_chk_input().  Any change to the parameters and return
 *          values of cl_chk_input() must be propogated to this routine
 *          and vice versa.  This is one case where a violation of the
 *          module naming guidelines is allowed.
 *
 *      This routine checks for input activity on the list of file descriptors
 *      in the global array fd_list.  If no input activity occurs within the
 *      timeout period supplied by the caller, a zero (0) value is returned to
 *      the caller.  If input is present on any of the file descriptors in
 *      fd_list, a non-zero value (1) is returned to the caller.
 *
 *      The difference between this routine and cl_chk_input() is that this
 *      routine rebuilds fd_list from sd_in and the RPC read file descriptor
 *      mask before calling cl_select_input().  cl_chk_input() uses the
 *      existing contents of fd_list before calling cl_select_input().
 *
 *      fd_list must be rebuilt for CSIs and SSIs before any select() call
 *      because some of the transient RPC descriptors placed in fd_list may
 *      have expired in the course of processing network messages.  The RPC
 *      descriptors are placed in fd_list to maintain the priority processing
 *      of input over output.
 *
 *      Because common library routines invoke cl_chk_input() before sending a
 *      message, it is the most central, unambiguous, point to rebuild fd_list
 *      for CSI/SSI use and still have the shortest possible window between a
 *      fd_list update and the actual select() call.  This method also results
 *      in the least amount of changes to the ACSLS product.
 *
 * Return Values:
 *
 *      0,  no input activity detected.
 *      1,  input active.
 *
 * Implicit Inputs:
 *
 *      sd_in           Socket descriptor of module's general IPC socket.
 *      svc_fds         RPC read file descriptor bit mask (sizeof(int) bits).
 *      svc_fdset       RPC read file descriptor bit mask (sizeof(fd_set) bits).
 *
 * Implicit Outputs:
 *
 *      fd_list         List of input descriptors.
 *      n_fds           Number of descriptors in fd_list.
 *
 * Considerations:
 *
 *      To avoid excessive recursive calls, event and trace logging are not
 *      performed in this routine.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      D. A. Beidle            03-Dec-1991     Original.
 *      E. A. Alongi            30-Oct-1992     Replace 0 with '\0' in call to
 *                                              memset().
 *      E. A. Alongi            12-Feb-1993     Eliminated DEC compatible 32 bit
 *                                              descriptor mask assignment to
 *                                              the variable read_mask.
 */

/*
 *      Header Files:
 */
#include <string.h>
#include <rpc/rpc.h>

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


int 
cl_chk_input (                       /* DON'T CHANGE ENTRY POINT NAME    */
    long tmo                            /* timeout value in sec. (>= 0), or */
)
                                        /* wait indefinitely (-1) */
{
    int     i;
    fd_set  read_mask;                  /* read file descriptor bit mask    */


    /* set read mask to RPC input FDs */
    FD_ZERO(&read_mask);
    read_mask = svc_fdset;


    /* set read mask to include CSI's IPC socket descriptor */
    FD_SET(sd_in, &read_mask);


    /* initialize array of descriptors */
    memset((char *)fd_list, '\0', sizeof(fd_list));
    for (n_fds = i = 0; i < FD_SETSIZE; i++) {
        if (FD_ISSET(i, &read_mask)) {
            fd_list[n_fds++] = i;
        }
    }


    /* check for input */
    if (cl_select_input(n_fds, fd_list, tmo) >= 0) {

        /* input exists */
        return (1);
    }


    /* no input present on any descriptor */
    return (0);
}
