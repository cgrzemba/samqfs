#ifndef lint
static char SccsId[] = "@(#)csi_xipc_hdr.c	2.2 03/6/02 (c) 2002 StorageTek";
#endif
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      csi_xipc_hdr()
 *
 * Description:
 *
 *      Routine serializes/deserializes an IPC_HEADER structure.
 *
 * Return Values:
 *
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J.A. Wishner    20-Oct-91       Remove st_module; not used.
 *      S.L. Siao       06-Mar-02       In xipc_hdr changed
 *                                      routine for options
 *                                      from uchar to char
 *      Mike Williams   01-Jun-2010     Included ml_pub.h to remove warnings.
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 */


/*
 *      Header Files:
 */
#include "csi.h"
#include "ml_pub.h"
 
 
 
/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_src = __FILE__;
 
/*
 *      Procedure Type Declarations:
 */


bool_t 
csi_xipc_hdr (
    XDR *xdrsp,                         /* xdr handle structure */
    IPC_HEADER *ipchp                         /* ipc header structure */
)
{

    char *sp;           /* used on call to xdr_string since need a ptr-to-ptr */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("csi_xipc_hdr",                        /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) ipchp);        /* argument list */
#endif /* DEBUG */


    /* translate the module type */
    if (!xdr_enum(xdrsp, (enum_t *)&ipchp->module_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "csi_xipc_header",  "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the options */
/*    if (!xdr_u_char(xdrsp, (char *)&ipchp->options)) {*/
    if (!xdr_char(xdrsp, (char *)&ipchp->options)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "csi_xipc_header","xdr_u_char()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the return socket */
    sp = ipchp->return_socket;
    if (!xdr_string(xdrsp, &sp, SOCKET_NAME_SIZE)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "csi_xipc_header","xdr_string()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
        
    }

    /* translate the ipc_identifier */
    if (!xdr_u_long(xdrsp, &ipchp->ipc_identifier)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "csi_xipc_header","xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
