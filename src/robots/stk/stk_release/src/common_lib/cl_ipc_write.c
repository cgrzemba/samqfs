static char SccsId[] = "@(#)cl_ipc_write.c	5.4 11/5/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_ipc_write
 *
 * Description:
 *
 *      Common routine for sending IPC messages.  Caller specifies destination
 *      socket name, buffer address and buffer size.  The buffer is assumed to
 *      begin with an IPC_HEADER structure.  This routine calls cl_ipc_send()
 *      to attempt the actual IPC send.
 *
 *      If an error occurs, cl_inform() is called with message_status
 *      STATUS_IPC_FAILURE.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_COUNT_TOO_LARGE  Message size greater than MAX_MESSAGE_SIZE.
 *      STATUS_COUNT_TOO_SMALL  Message size smaller that size of IPC header.
 *      STATUS_INVALID_VALUE    sock_name not [1..SOCKET_NAME_SIZE] chars long.
 *      STATUS_IPC_FAILURE      I/O failure.
 *      STATUS_PROCESS_FAILURE  Invalid parameter values.
 *
 * Implicit Inputs:
 *
 *      my_sock_name                    For cl_inform().
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
 *      Verified in ut_ipc_check.
 *
 * Revision History:
 *
 *      D. F. Reed              27-Sep-1988     Original.
 *
 *      D. A. Beidle            23-Nov-1991.    R3 IBR#20 - Added check for
 *          destination socket name length greater than SOCKET_NAME_SIZE.
 *          Updated code to standards.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <string.h>                     /* ANSI-C compatible */
#include <errno.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "cl_ipc_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */


/*
 *      Global and Static Variable Declarations:
 */
int             max_retries = MAX_RETRY;

static  char   *self = "cl_ipc_write";


/*
 *      Procedure Type Declarations:
 */


STATUS 
cl_ipc_write (
    char *sock_name,                     /* destination socket name */
    void *buffer,                        /* output buffer pointer */
    int byte_count                     /* number of bytes to write */
)
{
    int         i;
    IDENTIFIER  identifier;             /* for cl_inform() */
    STATUS      ret;


#ifdef DEBUG
    if TRACE(0) {
        cl_trace(self, 3,               /* routine name and parameter count */
                 (unsigned long)sock_name,
                 (unsigned long)buffer,
                 (unsigned long)byte_count);
    }
#endif /* DEBUG */


    /* check parameters */
    CL_ASSERT(self, (sock_name) && (buffer));


    /* check length of destination socket name */
    if (((i = strlen(sock_name)) < 1) || (i > SOCKET_NAME_SIZE)) {
        return (STATUS_INVALID_VALUE);
    }


    /* use cl_ipc_send for actual i/o attempt */
    ret = cl_ipc_send(sock_name, buffer, byte_count, max_retries);


    /* if ipc failure, need to inform the acssa */
    if (ret == STATUS_IPC_FAILURE) {

        /* log the event */
        MLOG((MMSG(123,"%s: Sending message to socket %s failed on \"%s\""), self, 
                                sock_name, strerror(errno)));


        /* send message to acssa */
        strncpy(identifier.socket_name, sock_name, SOCKET_NAME_SIZE);
        cl_inform(STATUS_IPC_FAILURE, TYPE_IPC, &identifier, errno);
    }

    return (ret);
}
