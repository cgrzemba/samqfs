
#ifndef lint
static char SccsId[] = "@(#)csi_xpnl_id.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xpnl_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes a PANELID structure.
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
static char     *st_module = "csi_xpnl_id()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xpnl_id (
    XDR *xdrsp,                         /* xdr handle structure */
    PANELID *pnlidp                        /* panelid */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) pnlidp);       /* argument list */
#endif /* DEBUG */

    /* translate the lsm_id */
    if (!csi_xlsm_id(xdrsp, &pnlidp->lsm_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xlsm_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the panel */
    if (!csi_xpnl(xdrsp, &pnlidp->panel)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xpnl()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
