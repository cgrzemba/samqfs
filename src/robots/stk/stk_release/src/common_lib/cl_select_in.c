static char SccsId[] = "@(#)cl_select_in.c	5.4 11/5/93 (c) 1991 StorageTek";
/*
 * Copyright (C) 1988,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      cl_select_input
 *
 * Description:
 *
 *      Common routine called to check file descriptors (1 or more) for input
 *      activity.  The routine uses the select() system call to pend on any
 *      input activity on the specified descriptors (files and/or sockets).
 *      The caller specifies a time-out period in seconds, where 0 = no
 *      time_out (return immediately) and -1 = pend indefinitely.
 *
 * Return Values:
 *
 *      fds index       index into specified fds array of active
 *                      file/socket descriptor.
 *      -1              no active input, time-out occurred OR select() error
 *                      OR signal received.
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
 *      NONE
 *
 * Module Test Plan:
 *
 *      Verified in select_test.c.
 *
 * Revision History:
 *
 *      D. F. Reed              28-Sep-1988     Original.
 *      J. A. Wishner           01-Aug-1989     Portability.  Conditionally
 *                                              compile fd handling macros
 *                                              (such as NBBY) since defined for
 *                                              some architectures, not others.
 *      J. A. Wishner           04-Aug-1989     Portability. Moved fd macros to
 *                                              defs.h.
 *      Mike Willliams          01-Jun-2010     Added include <string.h> to
 *                                              remove memset warning
 */
/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "cl_pub.h"
#include "ml_pub.h"
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
cl_select_input (
    int nfds,                           /* number of file descriptors to */
    int *fds,                           /* pointer to array of descriptors */
    long tmo                            /* time_out value (>= 0), or wait */
)
                                        /* indefinitely (-1) */
{
    register int i, maxd;
    fd_set readfds;
    int *fdp;                           /* descriptor pointer */
    struct timeval timeout, *tmop;

#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_select_input",     /* routine name */
                 3,                     /* parameter count */
                 (unsigned long)nfds,
                 (unsigned long)fds,
                 (unsigned long)tmo);
#endif /* DEBUG */

    /* validate nfds */
    if ((nfds < 0) || (nfds > FD_SETSIZE))
        return(-1);

    /* initialize the fd_set */
    /*lint -esym(718,bzero) -esym(746,bzero) : suppress msgs for bzero */
    FD_ZERO(&readfds);
    /*lint -restore   : restore messages for bzero */
    
    /* set up requested descriptors */
    for (i = maxd = 0, fdp = fds; i < nfds; i++, fdp++) {
        FD_SET(*fdp, &readfds);
        if (*fdp > maxd) maxd = *fdp;
    }
        
    /* set up timeout */
    if (tmo < 0)
        tmop = (struct timeval *)NULL;
    else {
        timeout.tv_sec = tmo;
        timeout.tv_usec = 0;
        tmop = &timeout;
    }

    /* hang on select */
    i = select(maxd + 1, &readfds, (fd_set*)NULL, (fd_set*)NULL, tmop);

    /* check return */
    if (i < 0) {

        /* EINTR is allowed for cancel and terminate */
        if (errno != EINTR) {

            /* error ... log it */
            MLOG((MMSG(510, "cl_select_input: select() failed, errno=%d\n"), errno));
        }
        return(-1);
    }

    /* timeout? */
    if (i == 0) return(-1);

    /* otherwise, figure out which fd is active */
    for (i = 0, fdp = fds; i < nfds; i++, fdp++)
        if (FD_ISSET(*fdp, &readfds)) break;

    /* just in case ... */
    if (i >= nfds) return(-1);

    return(i);
}
