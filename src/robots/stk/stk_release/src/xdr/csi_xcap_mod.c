static char SccsId[] = "@(#)csi_xcap_mod.c	5.3 11/9/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xcap_mode()
 *
 * Description:
 *
 *      Routine serializes/deserializes a CAP_MODE.
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
 *      J. A. Wishner           03-Oct-91       Created for release 3 version 2.
 *
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
static char     *st_module = "csi_xcap_mode()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xcap_mode (
    XDR *xdrsp,                                 /* xdr handle structure */
    CAP_MODE *cap_mode                          /* cap_mode */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) cap_mode);     /* argument list */
#endif /* DEBUG */

    /* translate cap_mode */
    if (!xdr_enum(xdrsp, (enum_t *)cap_mode)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_enum()",
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
