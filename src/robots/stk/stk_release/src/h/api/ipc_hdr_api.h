/*  SccsId      @(#)ipc_hdr_api.h	5.1 10/28/93  */
#ifndef _IPC_HDR_API_
#define _IPC_HDR_API_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      definition of the IPC data structure and related variables.  
 *      most of these structures are common to csi and acslm, 
 *      and are included by both header files.  This file contains
 *	definitions also needed by the ACSAPI.
 *
 * Modified by:
 *
 *	E. A. Alongi	28-Oct-1993	Original.
 *
 */

#define HOSTID_SIZE  12       /* We use a size of 12 to ensure that the
				 ipc_header is smaller than the csi_header.
				 There is a bug in the code that handles
				 the opposite situation. */
typedef struct {
    char name[HOSTID_SIZE];   /* This is \0 terminated if it is shorter than
				 HOSTID_SIZE, otherwise there is no ending \0 */
} HOSTID;

typedef struct {                        /* IPC header */
    unsigned long   byte_count;         /*   message length, including header */
    TYPE            module_type;        /*   sending module type */
    unsigned char   options;            /*   see defs.h */
    unsigned long   seq_num;            /*   message sequence number */
    char            return_socket[SOCKET_NAME_SIZE];
                                        /*   sender's input socket name */
    unsigned int    return_pid;         /*   sender's PID */
    unsigned long   ipc_identifier;     /*   used for message sync */
    TYPE            requestor_type;     /*   used by persistent processes */
    HOSTID          host_id;            /*   id of the originating host */
} IPC_HEADER;

#endif /* _IPC_HDR_API_ */
