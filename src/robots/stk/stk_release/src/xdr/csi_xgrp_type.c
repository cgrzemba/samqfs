#ifndef lint
static char SccsId[] = "@(#)csi_xgrp_type.c   1.0 14/2/02 ";
#endif
/*
 * Copyright (2002, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xgrp_type()
 *
 * Description:
 *
 *      Routine serializes/deserializes GROUP_TYPE.
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
 *      S. L. Yee           31-Aug-1998     New module created in NCS 4.0  @E59
 *      J. E. Jahn          18-Sep-1998     Ported to MVS/CSC 4.0 from LS. @E59
 *      S. L. Siao          14-Feb-2002     Changed message logging for TK
 */
/**ENDPROLOGUE**************************************************************/


/*
 *      Header Files:
 */
#include <stdio.h>
#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"
 
 
 
/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_xgrp_type()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t
csi_xgrp_type(
XDR     *xdrsp,                         /* xdr handle structure */
GROUP_TYPE    *grptype)                 /* type */
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) grptype);      /* argument list */
#endif /* DEBUG */

    /* translate group_type */
    if (!xdr_enum(xdrsp, (enum_t *) grptype)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,
                st_module,
                "xdr_enum()",
		MMSG(928, "message translation failure")));
        return(0);
    }

    return(1);

}

