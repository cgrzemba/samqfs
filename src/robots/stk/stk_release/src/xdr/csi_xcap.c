static char SccsId[] = "@(#)csi_xcap.c	5.3 11/9/93 ";
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xcap()
 *
 * Description:
 *
 *      Routine serializes/deserializes a CAP number.
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
 *      H. I. Grapek    22-Aug-1991     Original
 *      Scott Siao      20-Mar-2002     Added type cast to arguments of xdr functions.
 */

/*
 *      Header Files:
 */

#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"
 
/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_src = __FILE__;
static char     *st_module = "csi_xcap()";
 
 
/*
 *      Procedure Type Declarations:
 */
bool_t 
csi_xcap (
    XDR *xdrsp,                         /* xdr handle structure */
    CAP *capp                          /* cap */
)
{
#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2,
                 (unsigned long) xdrsp,
                 (unsigned long) capp);
#endif /* DEBUG */

    /* translate the cap */
    if (!xdr_char(xdrsp, (char *) capp)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_char()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    return(1);
}
