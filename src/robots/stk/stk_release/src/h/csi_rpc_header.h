/* SccsId @(#)csi_rpc_header.h	1.2 1/11/94  */
#ifndef _CSI_RPC_HEADER_
#define _CSI_RPC_HEADER_

/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Header file containing all CSI_HEADER structure-specific definitions.
 *
 * Modified by:
 *
 *      E. A. Alongi    02-Dec-1993.    Original, copied from csi_header.h
 *			with the RPC specific declarations separated out.
 */

/*
 *      Header Files:
 */

/*
 *	Defines, Typedefs and Structure Definitions:
 */

/*
 *      Procedure Type Declarations:
 */


#ifndef _CSI_HEADER_
#include "csi_header.h"
#endif

#define CSI_NETADDR_SIZE	 6	    /* #of bytes in a network address */

typedef struct {
    unsigned long	program;	/* callback program number */
    unsigned long	version;	/* version number */
    unsigned long	proc; 		/* procedure number to call back to */
    struct sockaddr_in	raddr;  	/* return internet address */
} CSI_HANDLE_RPC;

typedef struct {
    unsigned char addr[CSI_NETADDR_SIZE];         /* sender network address */
    unsigned int  pid;		/* sender process id */
    unsigned long seq_num;	/* sender sequence number */
} CSI_XID_RPC;

/*
 * Note:  the xid must stay at the very top of CSI_HEADER in order for
 * duplicate packet comparisons to work in csi_xdrrequest() & csi_xdrresponsed()
 */
typedef struct {
    CSI_XID_RPC		xid;		/* transaction id=net address,pid,seq#*/
    unsigned long	ssi_identifier;	/* identifier for use by SSI */
    CSI_SYNTAX		csi_syntax;	/* type of transfer syntax */
    CSI_PROTOCOL	csi_proto;	/* protocol used */
    CSI_CONNECT     	csi_ctype;	/* type connection management used */
    CSI_HANDLE_RPC	csi_handle;	/* return handle of client */
} CSI_HEADER_RPC;

#endif /* _CSI_RPC_HEADER_ */

