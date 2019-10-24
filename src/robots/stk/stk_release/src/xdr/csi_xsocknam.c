
#ifndef lint
static char SccsId[] = "@(#)csi_xsocknam.c	5.3 11/9/93 (c) 1989 StorageTek";
#endif
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      csi_xsockname()
 *
 * Description:
 *
 *      Routine serializes/deserializes an XXXX structure.
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
 * Revision History:
 *
 *      ? ?             1989            Created.
 *      Mike Williams   01-Jun-2010     Included ml_pub.h to remove warnings.
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
static char     *st_module = "csi_sockname()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xsockname (
    XDR *xdrsp,                         /* xdr handle structure */
    char *socknamep                     /* ptr to socket name */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) socknamep);    /* argument list */
#endif /* DEBUG */

    /* translate the socket name */
    if (!xdr_string(xdrsp, &socknamep, SOCKET_NAME_SIZE)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_string()",
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }


    return(1);

}
