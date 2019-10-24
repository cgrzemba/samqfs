
#ifndef lint
static char SccsId[] = "@(#)csi_xreq_hdr.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xreq_hdr()
 *
 * Description:
 *
 *      Routine serializes/deserializes a CSI_REQUEST_HDR structure.
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
static char     *st_module = "csi_xreq_hdr()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xreq_hdr (
    XDR *xdrsp,              /* xdr handle structure */
    CSI_REQUEST_HEADER *req_hdr            /* request header */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) req_hdr);      /* argument list */
#endif /* DEBUG */

    /* translate the csi_header */
    if (!csi_xcsi_hdr(xdrsp, &req_hdr->csi_header)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xcsi_hdr()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the message_header */
    if (!csi_xmsg_hdr(xdrsp, &req_hdr->message_header)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xmsg_hdr()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
