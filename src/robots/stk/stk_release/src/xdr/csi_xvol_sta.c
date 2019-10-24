
#ifndef lint
static char SccsId[] = "@(#)csi_xvol_sta.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xvol_status()
 *
 * Description:
 *
 *      Routine serializes/deserializes an VOLUME_STATUS structure.
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
static char     *st_module = "csi_xvol_status()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xvol_status (
    XDR *xdrsp,                         /* xdr handle structure */
    VOLUME_STATUS *vstatp                        /* volume status structure */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) vstatp);       /* argument list */
#endif /* DEBUG */

    /* translate volid */
    if (!csi_xvol_id(xdrsp, &vstatp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xvol_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate status (type RESPONSE_STATUS) */
    if (!csi_xres_status(xdrsp, &vstatp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xres_status()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
