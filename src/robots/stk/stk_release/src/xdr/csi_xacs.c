static char SccsId[] = "@(#)csi_xacs.c	5.3 11/9/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_acs()
 *
 * Description:
 *
 *      Routine serializes/deserializes an ACS structure.
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
 * Revision History
 *    Scott Siao     20-Mar-2002     Added type cast to arguments of xdr functions.
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
static char     *st_module = "csi_xacs()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xacs (
    XDR *xdrsp,                         /* xdr handle structure */
    ACS *acsp                          /* acs */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) acsp);         /* argument list */
#endif /* DEBUG */

    /* translate the acs */
    if (!xdr_char(xdrsp, (char *) acsp)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_char()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
