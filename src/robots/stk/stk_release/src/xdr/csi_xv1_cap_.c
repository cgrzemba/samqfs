#ifndef lint
static char SccsId[] = "@(#)csi_xv1_cap_.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xv1_cap_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes a version 0 CAPID structure.
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
 *      J. A. Wishner       03-Oct-1991.    Complete mods release 3.
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
static char     *st_module = "csi_xv1_cap_id()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xv1_cap_id (
    XDR *xdrsp,                                /* xdr handle structure */
    V1_CAPID *capidp                               /* cap_id */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) capidp);       /* argument list */
#endif /* DEBUG */

    /* translate the acs */
    if (!csi_xacs(xdrsp, &capidp->acs)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xacs()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the lsm */
    if (!csi_xlsm(xdrsp, &capidp->lsm)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xlsm()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
