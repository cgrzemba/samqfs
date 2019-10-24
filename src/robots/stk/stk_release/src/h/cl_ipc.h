/* SccsId @(#)cl_ipc.h	1.2 1/11/94  */
#ifndef _CL_IPC_H_
#define _CL_IPC_H_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Functional Description:
 *
 *    
 *    Defines all common library ipc function prototypes.
 *
 *
 * Modified by:
 *
 *	Ken Stickney           14-Jun-1993	Original.
 */


/*
 * Header Files:
 */


/*
 * Procedure Type Declarations:
 */


STATUS
cl_ipc_create( char    *sock_name);     /* pointer to input socket name */


STATUS
cl_ipc_destroy();

int 
cl_select_input(int nfds,    /* number of file descriptors to monitor */
                int *fds,    /* pointer to array of descriptors */
                long tmo);   /* time_out value (>= 0), */
			     /* or wait indefinitely (-1) */

int
cl_ipc_open( char    *sock_name_in,     /* pointer to input socket name */
             char    *sock_name_out);   /* pointer to "bind" socket name */


STATUS
cl_ipc_read( char    buffer[],          /* pointer to input buffer area */
             int     *byte_count);      /* pointer to actual byte_count */


STATUS
cl_ipc_send( char    *sock_name,        /* destination socket name */
                                        /* If sock_name is NULL, then this */
                                        /* procedure was called to flush */
                                        /* the existing Write_Queue.  */
             char    *buffer,           /* output buffer pointer */
             int     byte_count,        /* number of bytes to write */
             int     retry_count);      /* number of retries before failure */


STATUS
cl_ipc_write( char    *sock_name,       /* destination socket name */
              char    *buffer,          /* output buffer pointer */
              int     byte_count);      /* number of bytes to write */


STATUS
cl_ipc_xmit( char    *sock_name,        /* destination socket name */
             char    *buffer,            /* output buffer pointer */
             int     byte_count);         /* number of bytes to write */
#endif

