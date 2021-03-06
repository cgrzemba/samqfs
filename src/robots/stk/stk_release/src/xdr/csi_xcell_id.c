static char SccsId[] = "@(#)csi_xcell_id.c	5.3 11/9/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xcell_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes an CELLID structure.
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
#include "cl_pub.h"
#include "ml_pub.h"
 
 
 
/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_src = __FILE__;
static char     *st_module = "csi_xcell_id()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xcell_id (
    XDR *xdrsp,                         /* xdr handle structure */
    CELLID *cellidp                       /* cell_id structure */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) cellidp);      /* argument list */
#endif /* DEBUG */

    /* translate the panelid */
    if (!csi_xpnl_id(xdrsp, &cellidp->panel_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "csi_xpnl_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the row */
    if (!csi_xrow(xdrsp, &cellidp->row)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "csi_xrow()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the column */
    if (!csi_xcol(xdrsp, &cellidp->col)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "csi_xcol()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
