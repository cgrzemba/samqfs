static char SccsId[] = "@(#)cl_chk_input.c	5.2 5/3/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_chk_input
 *
 * Description:
 *
 *      Common routine to check for input activity.  Uses cl_select_input
 *      with a time-out parameter supplied by the caller.
 *              
 * Return Values:
 *
 *      0,  no input activity detected.
 *      1,  input active.
 *
 * Implicit Inputs:
 *
 *      n_fds           Number of descriptors in fd_list.
 *      fd_list         List of input descriptors.
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      D. F. Reed              30-May-1989     Original.
 */

/*
 *      Header Files:
 */

#include "flags.h"
#include "system.h"
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
cl_chk_input (
    long tmo                            /* time_out value in sec. (>= 0), or */
                                        /* wait indefinitely (-1) */
)
{
    /* check for input */
    if (cl_select_input(n_fds, fd_list, tmo) >= 0) {

        /* Input exists */
        return(1);
    }
    return(0);
}
