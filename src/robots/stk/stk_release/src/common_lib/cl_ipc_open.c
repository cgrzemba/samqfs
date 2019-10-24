/*
 * Copyright (C) 1989,2011, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      cl_ipc_open
 *
 * Description:
 *
 *      Common routine to create/bind IPC socket.  sock_name_in is the
 *      requested port number for a UDP internet socket.  if it is "0",
 *      the system picks the port number on the bind() call.
 *
 * Return Values:
 *
 *      >= 0, socket descriptor opened and bind performed on.
 *      <0,   socket() or bind() failed.
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
 *      Verified in ipc_test.c
 *
 * Revision History:
 *
 *      D. F. Reed          14-Mar-1989     Original.
 *      D. F. Reed          31-Mar-1989     Modified to use internet
 *                                          stream sockets.
 *      D. L. Trachy        13-Jun-1990     Modified to use Datagram
 *      D. A. Beidle        28-Sep-1991     Lint cleanup.
 *      D. A. Beidle        13-Nov-1992     R4BR#303 - Fixed memory
 *                                          corruption cause by sprintf() into
 *                                          buffer shorter that source string.
 *      K. J. Stickney      12-Dec-1994     Explicit cast of name in 
 *                                          call to getsockname() for
 *                                          Solaris port.
 *      Anton Vatcky        01-Mar-2002     Added correct type definition
 *                                          for nsize for Solaris and AIX.
 *      Mitch Black         07-Dec-2004     Changed port from an unsigned short 
 *                                          to an int, and cast in the htons() call.
 *                                          Otherwsie the check for port < 0 was
 *                                          a possible bug source 
 *                                          (it was superfluous).
 *      Mike williams       04-May-2010     Changed nsize to socklen_t for 64-bit
 *                                          compile.
 *      Joseph Nofi         15-Jul-2011     Added htonl() to convert long to sin_addr. 
 *
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifndef IPC_SHARED
#include <netinet/in.h>
#endif /* IPC_SHARED */

#include <string.h>                     /* ANSI-C compatible */
#include <stdio.h>                      /* ANSI-C compatible */
#include <unistd.h>
#include <stdlib.h>

#include "cl_pub.h"
#include "cl_ipc_pub.h"

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
cl_ipc_open (
    char *sock_name_in,                  /* pointer to input socket name */
    char *sock_name_out                 /* pointer to "bind" socket name */
)
{

#ifdef IPC_SHARED
    struct  sockaddr_un name;
    int                 pid;            /* pid of process used for any socket */
#else
    struct  sockaddr_in name;
#ifdef AIX
    socklen_t           nsize;
#else
    socklen_t           nsize;
#endif /* nsize */
    int                 on = 1;
    int         port;
#endif /* IPC_SHARED */

    int                 sd;             /* socket descriptor */
    char                sock_name[SOCKET_NAME_SIZE];


#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_ipc_open",         /* routine name */
                 2,                     /* parameter count */
                 (unsigned long)sock_name_in,
                 (unsigned long)sock_name_out);
#endif /* DEBUG */


    /* make local copy of socket name for later modification */
    strcpy(sock_name, sock_name_in);

    /* create output socket first */
#ifdef IPC_SHARED
    if ((sd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0)
        return(-1);
#else
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        return(-1);

    /* check port number specified */
    if ((port = atoi(sock_name)) < 0)
        return(-1);
#endif /* IPC_SHARED */


    /* now, for the bind */
#ifdef IPC_SHARED
    /* if sock name in is ANY_PORT use the pid */
    if (strcmp (sock_name, ANY_PORT) == 0) {
        pid = getpid();
        sprintf (sock_name,"PID%d",pid);
    }

    /* build datagram name */
    name.sun_family = PF_UNIX;
    strcpy(name.sun_path, DATAGRAM_PATH);
    strcat(name.sun_path, sock_name);

    /* unlink the sock name if it exists */
    (void) unlink (name.sun_path);
   
    /* copy socket name over */
    strcpy (sock_name_out, sock_name);
#else
    name.sin_family = PF_INET;
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    name.sin_port = htons((unsigned short)port);

    /* if port already selected, set re-use option */
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
        return(-1);
#endif /* IPC_SHARED */

    if (bind(sd, (struct sockaddr *)&name, sizeof(name)) < 0)
        return(-1);

#ifndef IPC_SHARED
    /* figure out the port number used */
    nsize = sizeof(name);
    if (getsockname(sd, (struct sockaddr *)&name, &nsize) < 0)
        return(-1);

    sprintf(sock_name_out, "%d", ntohs(name.sin_port));
#endif /* IPC_SHARED */

    return(sd);
}

