/* SccsId @(#)csi_adi_header.h	1.2 1/11/94  */
#ifndef _CSI_ADI_HEADER_
#define _CSI_ADI_HEADER_

/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Header file containing all CSI_HEADER_ADI structure-specific
 *	definitions.
 *
 * Modified by:
 *
 *      E. A. Alongi    02-Dec-1993.    Original, copied from csi_header.h
 *			with the ADI specific declarations separated out.
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

#define CSI_ADI_NAME_SIZE	32	   /* #of bytes in an ADI network name*/

typedef struct {
    unsigned char client_name[CSI_ADI_NAME_SIZE];
    unsigned long proc;
} CSI_HANDLE_ADI;

typedef struct {
    unsigned char client_name[CSI_ADI_NAME_SIZE]; /* sender network address */
    unsigned long proc;		/* destination procedure number */
    unsigned int  pid;		/* sender process id */
    unsigned long seq_num;	/* sender sequence number */
} CSI_XID_ADI;

/*
 * Note:  the xid must stay at the very top of CSI_HEADER in order for
 * duplicate packet comparisons to work in csi_xdrrequest() & csi_xdrresponsed()
 */
typedef struct {
    CSI_XID_ADI		xid;		/* transaction id=net address,pid,seq#*/
    unsigned long	ssi_identifier;	/* identifier for use by SSI */
    CSI_SYNTAX		csi_syntax;	/* type of transfer syntax */
    CSI_PROTOCOL	csi_proto;	/* protocol used */
    CSI_CONNECT     	csi_ctype;	/* type connection management used */
    CSI_HANDLE_ADI	csi_handle;	/* return handle of client */
} CSI_HEADER_ADI;

#endif /* _CSI_ADI_HEADER_ */

