
#ifndef lint
static char SccsId[] = "@(#)csi_xport_id.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xport_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes a PORTID structure.
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
static char     *st_module = "csi_xport_id()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xport_id (
    XDR *xdrsp,                         /* xdr handle structure */
    PORTID *portidp                       /* portid */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) portidp);      /* argument list */
#endif /* DEBUG */

    /* translate the acs */
    if (!csi_xacs(xdrsp, &portidp->acs)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xacs()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the port */
    if (!csi_xport(xdrsp, &portidp->port)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xport()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
