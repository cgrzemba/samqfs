
#ifndef lint
static char SccsId[] = "@(#)csi_xpool_id.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 * Name:
 *      csi_xpool_id()
 *
 * Description:
 *      Routine serializes/deserializes a POOLID structure.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *
 * Module Test Plan:
 *      NONE
 */

/*      Header Files: */
#include "csi.h"
#include "ml_pub.h"
 
/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_src = __FILE__;
static char     *st_module = "csi_xpool_id()";
 
/*    Procedure Type Declarations: */

bool_t 
csi_xpool_id (
    XDR *xdrsp,                          /* xdr handle structure */
    POOLID *poolidp                        /* poolid */
)
{

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp, (unsigned long) poolidp);
#endif /* DEBUG */

    /* translate the pool */
    if (!csi_xpool(xdrsp, &poolidp->pool)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xpool()",
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    return(1);
}
