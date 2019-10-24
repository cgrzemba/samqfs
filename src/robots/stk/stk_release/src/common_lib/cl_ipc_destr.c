static char SccsId[] = "@(#)cl_ipc_destr.c	5.3 10/4/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_ipc_destroy
 *
 * Description:
 *
 *      Common routine for tearing down the process IPC environment.
 *      Flushes the Write_Queue for this process by calling cl_ipc_send().
 *      Uses close system call on socket descriptor.
 *      If possible, failures are reported to the event_logger.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_IPC_FAILURE      Unable to close socket descriptors or
 *                              unlink socket name.
 *
 * Implicit Inputs:
 *
 *      sd_in                   Input socket descriptor.
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
 *      Verified in ipc_test.c
 *
 * Revision History:
 *
 *      D. F. Reed              27-Sep-1988     Original.
 *
 *      Jim Montgomery          04-May-1989     Added Write_Queue Flush.
 *
 *      D. L. Trachy            14-Jun-1990     Added unlink for datagrams
 *
 *      D. A. Beidle            24-Nov-1991.    IBR#151 - Set sd_in to -1 when
 *          socket is successfully closed to indicate that event logging should
 *          not be attempted.
 */

/*
 *      Header Files:
 */

#include "flags.h"
#include "system.h"
#include <stdio.h>                      /* ANSI-C compatible */
#include <string.h>                     /* ANSI-C compatible */
#include <errno.h>
#include <unistd.h>

#include "cl_pub.h"
#include "cl_ipc_pub.h"
#include "sblk_defs.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */


/*
 *      Global and Static Variable Declarations:
 */


/*
 *      Procedure Type Declarations:
 */


STATUS 
cl_ipc_destroy (void)
{
    char lbuf[MAX_LOG_MSG_SIZE];                     /* event message buffer */
    STATUS ret = STATUS_SUCCESS;


#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_ipc_destroy",      /* routine name */
                 0);                    /* parameter count */
#endif /* DEBUG */


    /* Outbound messages may have be queued so flush the Write_Queue */
    (void)cl_ipc_send((char *)NULL, (char *)NULL, 0, 0);


    /* close socket descriptor */
    if (close(sd_in)) {
#ifdef DEBUG
        fprintf(stderr, "cl_ipc_destroy: close(sd_in) failed on \"%s\".\n",
                strerror(errno));
#endif /* DEBUG */
        ret = STATUS_IPC_FAILURE;
    }

#ifdef IPC_SHARED
    /* build and then unlink datagram file */
    strcpy(lbuf, DATAGRAM_PATH);
    strcat(lbuf, my_sock_name);
    (void) unlink (lbuf);

    /* detach from shared memory segment */
    if (small_preamble) {
        (void) shmdt((char *)small_preamble);
        small_preamble = 0;
    }
#endif /* IPC_SHARED */


    /* mark socket descriptor as closed or invalid */
    sd_in = -1;


    return(ret);
}
