
#ifndef lint
static char SccsId[] = "@(#)csi_xlocatio.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xlocation()
 *
 * Description:
 *
 *      Routine serializes/deserializes data of type LOCATION.
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
static char     *st_module = "csi_xlocation()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xlocation (
    XDR *xdrsp,                                /* xdr handle structure */
    LOCATION *locp                                 /* location */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) locp);         /* argument list */
#endif /* DEBUG */

    /* translate location */
    if (!xdr_enum(xdrsp, (enum_t *)locp)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
