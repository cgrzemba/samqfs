#ifndef lint
static char SccsId[] = "@(#) %full_name:  1/csrc/acs_sel_input.c/5 %";
#endif
/*
 *
 * Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *      acs_select_input()
 *
 * Description:
 *      acs_select_input() waits timeout seconds for a response from the SSI
 *      A timeout value of -1 causes acs_select_input() to block
 *        indefinitely.
 *      A timeout value of 0  effects a poll of response availability.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE.
 *
 * Parameters:
 * 
 *   timeout -   Length of time to wait for response.   
 * 
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *     This module is machine dependent. The function signature will
 *     remain constant over platforms, but the code contained herein
 *     will vary.
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Ken Stickney         04-Jun-1993    Original
 *    Ken Stickney         06-Nov-1993    Added param section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright. Changes for new error
 *                                        messaging. 
 *    Ken Stickney         31-Aug-1994    Got rid of call to cl_select_input(),
 *                                        makes direct call to select(), if 
 *                                        return from select = 0, sets status
 *                                        to STATUS_PENDING. Fix for bug BR#37.
 *    Ken Stickney         6-Jan-1995     Added ifdef'ed call to include
 *                                        select.h for AIX and Solaris.
 *    Ken Stickney         19-Jan-1995    Fix for Toolkit BR#52. Added
 *                                        call to acs_trace_exit().
 *    Scott Siao           30-Apr-1998    Added acsReturn for acs_trace_exit.
 *    Van Lepthien         22-Aug-2001    (added to base code for 2.2)
 *    Hemendra(Wipro)      29-Dec-2002    Added Linux Support by including
 *                                              sys/select.h for Linux.
 *    Mitch Black          28-Mar-2004     Corrected handling of EINTR to
 *                      retry rather than cancel.  Needed for AIX, as BSD
 *                      based OS's handle the retry at the system level.
 *    Mike Williams        27-May-2010    Included prototype for memset.
 */

#include "acssys.h"
#include "flags.h"
#include "system.h"
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef AIX
#include <sys/select.h>
#endif
#ifdef SOLARIS
#include <sys/select.h>
#endif
#ifdef LINUX
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "ml_pub.h"

#undef SELF
#define SELF "acs_select_input"
#undef ACSMOD
#define ACSMOD 205

STATUS acs_select_input (int tmo)
{

    int nfds, maxd;
    fd_set readfds;
    int *fdp;                           /* descriptor pointer */
    struct timeval timeout, *tmop;

    STATUS acsReturn = STATUS_SUCCESS;

    acs_trace_entry();

    /* initialize the fd_set */
    FD_ZERO(&readfds);
 
    /* set up requested descriptors */
    for (nfds = maxd = 0, fdp = &sd_in; nfds < 1; nfds++, fdp++) {
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
 
    /* hang on select - retry if EINTR (system call interrupted) */
    /* if < 0, retry if appropriate, else return error */
    while ((nfds = select(maxd + 1, &readfds, (fd_set*)NULL, (fd_set*)NULL, tmop)) < 0)
    {
        /* EINTR is allowed for cancel and terminate */
        if (errno == EINTR) {
            MLOG((MMSG(509, "acs_select_input: select() returned EINTR (errno=%d); Retrying...\n"), errno));
            continue;
        } else { 
            /* error ... log it and fail */
            MLOG((MMSG(510, "acs_select_input: select() failed, errno=%d\n"), errno));
            return(STATUS_IPC_FAILURE);
        }
    }
 
    /* timeout? */
    if (nfds == 0) return(STATUS_PENDING);
 
    /* just in case ... */
    if (nfds != 1) {
        /* error ... log it */
        MLOG((MMSG(510, "acs_select_input: select() failed, nfds=%d\n"), nfds));
        return(STATUS_IPC_FAILURE);
    }
 
    acs_trace_exit(acsReturn);

    return(STATUS_SUCCESS);
}
