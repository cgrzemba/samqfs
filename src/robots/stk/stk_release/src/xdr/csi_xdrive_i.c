
#ifndef lint
static char SccsId[] = "@(#)csi_xdrive_i.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xdrive_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes a DRIVEID structure.
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
static char     *st_module = "csi_xdrive_id()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xdrive_id (
    XDR *xdrsp,                         /* xdr handle structure */
    DRIVEID *driveidp                      /* drive id structure */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) driveidp);     /* argument list */
#endif /* DEBUG */

    /* translate the panel id */
    if (!csi_xpnl_id(xdrsp, &driveidp->panel_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "csi_xpnl_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }


    /* translate the drive */
    if (!csi_xdrive(xdrsp, &driveidp->drive)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "csi_xdrive_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
