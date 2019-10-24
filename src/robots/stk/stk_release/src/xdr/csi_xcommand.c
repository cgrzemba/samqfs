static char SccsId[] = "@(#)csi_xcommand.c	5.3 11/9/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xcommand()
 *
 * Description:
 *
 *      Routine serializes/deserializes data of type COMMAND.
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
static char     *st_module = "csi_xcommand()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xcommand (
    XDR *xdrsp,                         /* xdr handle structure */
    COMMAND *comp                          /* command */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) comp);         /* argument list */
#endif /* DEBUG */

    /* translate command */
    if (!xdr_enum(xdrsp, (enum_t *)comp)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
