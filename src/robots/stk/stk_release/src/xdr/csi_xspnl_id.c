
#ifndef lint
static char SccsId[] = "@(#)csi_xspnl_id.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xspnl_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes an SUBPANELID structure.
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
static char     *st_module = "csi_xspnl_id()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xspnl_id (
    XDR *xdrsp,                         /* xdr handle structure */
    SUBPANELID *spidp                         /* sub panel id */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) spidp);        /* argument list */
#endif /* DEBUG */

    /* translate the panel_id */
    if (!csi_xpnl_id(xdrsp, &spidp->panel_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xpnl_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the begin_row */
    if (!csi_xrow(xdrsp, &spidp->begin_row)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xrow()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the begin_col */
    if (!csi_xcol(xdrsp, &spidp->begin_col)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xcol()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the end_row */
    if (!csi_xrow(xdrsp, &spidp->end_row)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xrow()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the end_col */
    if (!csi_xcol(xdrsp, &spidp->end_col)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xcol()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
