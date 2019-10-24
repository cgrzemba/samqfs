static char SccsId[] = "@(#) %full_name:  1/csrc/cl_ipc_xmit.c/6 %";
#ifndef lint
static char *_csrc = "@(#) %filespec: cl_ipc_xmit.c,6 %  (%full_filespec: cl_ipc_xmit.c,6:csrc:1 %)";
#endif
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      cl_ipc_xmit
 *
 * Description:
 *
 *      Common routine for transmitting IPC messages.  Caller specifies
 *      destination socket name, buffer address and buffer size.
 *      The buffer is assumed to begin with an IPC_HEADER structure.
 *      Performs rudimentary byte_count validation.
 *      This routine fills in ipc_header.module_type and
 *      ipc_header.return_socket based on global module definitions.
 *      Assumes external (global) definitions for input socket name
 *      (my_sock_name) and the calling module type (my_module_type).
 *      A socket is created (sdout) and set up so as not to block on
 *      connect().  A connect() is issued and the data is written.
 *      No retries are attempted in the event of failure.  This function
 *      is handled in cl_ipc_send().
 *      Events are not logged to prevent runaway recursion.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_COUNT_TOO_SMALL  Byte_count specified is < sizeof(IPC_HEADER).
 *      STATUS_COUNT_TOO_LARGE  Byte_count specified is > MAX_MESSAGE_SIZE.
 *      STATUS_IPC_FAILURE      I/O failure.
 *      STATUS_PENDING          Connection was attempted, but returned
 *                              as in-progress.
 *      STATUS_PROCESS_FAILURE  Signal registration failure, or gethost*()
 *                              failure.
 *
 * Implicit Inputs:
 *
 *      my_sock_name            Input socket name.
 *      my_module_type          Calling module type.
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      Failures detected here are not logged due to the possibility
 *      of infinite recursion.
 *
 * Module Test Plan:
 *
 *      Verified in ipc_test.c
 *
 * Revision History:
 *
 *      Jim Montgomery      04-May-1989    Original.
 *      D. F. Reed          16-Jun-1989    Added STATUS_PENDING return if
 *                      connection cannot be established right away.
 *      J. A. Wishner       01-Aug-1989    Deleted retry count parm, not used
 *                      (related to bug in cl_ipc_send() which called this
 *                      without retry count).
 *      D. L. Trachy        03-May-1990    Added well know socket names ACSLOCK
 *                      and ACSMV
 *      J. W. Montgomery    17-May-1990    Added well know name ACSSV
 *      J. W. Montgomery    24-May-1990    Changed ACSMV to ACSMT.
 *      D. L. Trachy        12-Jun-1990    Added SHARED_IPC mechanism
 *      D. A. Beidle            28-Sep-1991     Moved alternate socket name
 *                                              selection to cl_get_sockname().
 *                                              lint cleanup.
 *	H. I. Grapek        04-Jan-1994    Fixed warning for memcpy.
 *      K. J. Stickney      23-Dec-1994    Ifdef'ed out fcntl call to
 *                                         make connect call block, for
 *                                         Solaris port.
 *      Van Lepthien        27-Aug-2001    Added debugging code
 *      Scott Siao          20-Mar-2002    Fixed debug code, was missing ;
 *         
 *      Hemendra	    02-Feb-2003	   Added Linux support. Use of blocking 
 *      				   socket for Linux, as it is for Solaris.
 *      Mike Williams       02-Jun-2010    Included stdlib.h to remedy warnings.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <errno.h>                      /* ANSI-C compatible */
#include <stdio.h>                      /* ANSI-C compatible */
#include <string.h>                     /* ANSI-C compatible */
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <memory.h>
#include <stdlib.h>

#ifndef IPC_SHARED
#include <netinet/in.h>
#ifdef SOLARIS
#include <sys/file.h>
#endif
#ifdef AIX
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <netdb.h>
#endif /* IPC_SHARED */

#include "cl_pub.h"
#include "sblk_defs.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

static unsigned long seq_number = 0;

#ifndef IPC_SHARED
static  struct  sockaddr_in dest;       /* destination socket struct */
static  BOOLEAN     host_init = FALSE;  /* host address initialized flag */ 
#endif /* IPC_SHARED */

/*
 *      Procedure Type Declarations:
 */


STATUS 
cl_ipc_xmit (
    char *sock_name,                     /* destination socket name */
    void *buffer,                        /* output buffer pointer */
    int byte_count                     /* number of bytes to write */
)
{
    IPC_HEADER *ipc;                    /* pointer to ipc_header of data */
    
#ifdef DEBUG
    int ii;
    char * cbuff;
#endif
    
#ifdef IPC_SHARED
    int                 error;
    struct  sockaddr_un name;
    int                 shared_memory_block;
#else /* IPC_SHARED */
    int                 cnt;
    char                hostname[128];
    struct hostent     *hp, *gethostbyname();
    int                 sdout;
#endif /* IPC_SHARED */

#ifdef DEBUG
    cbuff = buffer;
#endif

#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_ipc_xmit", 3,      /* routine name and parameter count */
                 (unsigned long)sock_name,
                 (unsigned long)buffer,
                 (unsigned long)byte_count);

    /* check environment for well-known name changes */
    sock_name = cl_get_sockname(sock_name);
#endif /* DEBUG */

    /* fill in ipc_header */
    ipc = (IPC_HEADER *)buffer;
    ipc->byte_count = byte_count;
    ipc->module_type = my_module_type;
    ipc->options = 0;
    ipc->seq_num = ++seq_number;
    strncpy(ipc->return_socket, my_sock_name, SOCKET_NAME_SIZE);
    ipc->return_pid = getpid();

#ifdef IPC_SHARED

    /* construct name of socket to send to */
    name.sun_family = PF_UNIX;
    strcpy(name.sun_path, DATAGRAM_PATH);
    strcat(name.sun_path, sock_name);
 
    /* attempt to place packet into shared memory */
    /* cl_sblk_write returns -1 on error, 0 on pending, or packet_number */
    shared_memory_block = cl_sblk_write(buffer, byte_count, 1);
    switch (shared_memory_block) {
        case -1:                /* error in shared memory */
            return STATUS_PROCESS_FAILURE;
        case 0:
            return STATUS_PENDING;
        default:
            break;
    }

    /* attempt to send header */ 
    if (sendto(sd_in, (char *)&shared_memory_block, sizeof(shared_memory_block),
               0, (struct sockaddr *)&name, sizeof(struct sockaddr_un)) < 0) {

        /* copy errno over before sblk call */
        error = errno;
 
        /* remove the message just placed into shared memory */
        if (cl_sblk_remove(shared_memory_block) != TRUE) {
#ifdef DEBUG
            fprintf(stderr,
                    "cl_ipc_xmit: Unable to remove shared memory message.\n");
#endif /* DEBUG */
            return STATUS_PROCESS_FAILURE;
        }

        /* return depending on error code */
        switch (error) {
            case ENOBUFS:
            case EINTR:
                return STATUS_PENDING;

            case ENOENT:
            case ECONNREFUSED:
                return STATUS_IPC_FAILURE;

            default:
#ifdef DEBUG
                fprintf(stderr,
                        "cl_ipc_xmit: Sendto failure errno = %d, to = %s\n",
                        errno, sock_name);
#endif /* DEBUG */
                return STATUS_PROCESS_FAILURE;
        }
    
    }
    return STATUS_SUCCESS;

#else /* IPC_SHARED */

    /* attempt to xmit data */
    /* current implementation has all processes on same host, */
    /* so do host name lookup only once */
    if (!host_init) {
        if (gethostname(hostname, sizeof(hostname)) < 0)
            return(STATUS_PROCESS_FAILURE);  /* Don't retry */

        if ((hp = gethostbyname(hostname)) == 0)
            return(STATUS_PROCESS_FAILURE);  /* Don't retry */

        memcpy ((char *)&dest.sin_addr, hp->h_addr, hp->h_length);
        dest.sin_family = PF_INET;
        host_init = TRUE;
    }
    dest.sin_port = htons((unsigned short)atoi(sock_name));

    if ((sdout = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        return STATUS_PROCESS_FAILURE;  /* Don't retry */
    }
#ifndef SOLARIS 
#ifndef LINUX
    if (fcntl(sdout, F_SETFL, FNDELAY) < 0) {
        close(sdout);
        return STATUS_PROCESS_FAILURE;  /* Don't retry */
    }
#endif
#endif

    if (connect(sdout, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
#ifdef DEBUG
	printf("cl_ipc_xmit_d: socket=%d connect() error=%d.\n",sdout,errno);
#endif
        /* check errno to see if retry is reasonable */
        switch (errno) {
         case EBADF:
         case ENOTSOCK:
         case EAFNOSUPPORT:
         case EISCONN:
         case ENETUNREACH:
         case EADDRNOTAVAIL:
         case EFAULT:
            close(sdout);
            return STATUS_PROCESS_FAILURE;  /* Don't retry */

         case EINPROGRESS:
            close(sdout);
            return STATUS_PENDING;

         default:
            close(sdout);
            return STATUS_IPC_FAILURE;
        }
    }
    
#ifdef DEBUG
                printf("cl_ipc_xmit_d: socket=%d write(%d) \n%d: ",sdout,byte_count,sdout);
                for (ii = 0 ; ii < byte_count ; ii++)
                {
                    if (cbuff[ii] < 16)
                        printf(" 0%X",(char) cbuff[ii]);
                    else
                        printf(" %X",(char) cbuff[ii]);
        	    if ((ii+1)%8 == 0)
                        printf("   ");
        	    if ((ii+1)%32 == 0)
                        printf("\n%d: ",sdout);
                } 
                printf("\n\n");

#endif	 

    if ((cnt = write(sdout, buffer, byte_count)) != byte_count) {
        if (cnt < 0) {
        
#ifdef DEBUG
	    printf("cl_ipc_xmit_d: socket=%d write() error=%d.\n",sdout,errno);
#endif

            /* check errno to see if retry is reasonable */
            switch (errno) {
             case EBADF:
             case ENOTSOCK:
             case EFAULT:
             case EMSGSIZE:
                close(sdout);
                return STATUS_PROCESS_FAILURE;  /* Don't retry */

             default:
                break;
            }
        }
        ipc->options |= RETRY; /* mark retry in ipc header */
        close(sdout);
        return STATUS_IPC_FAILURE;
    }

    close(sdout);
    return STATUS_SUCCESS;
#endif /* IPC_SHARED */
}

