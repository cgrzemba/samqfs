
#ifndef lint
static char SccsId[] = "@(#)csi_xmedia_t.c	5.4 3/6/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xmedia_type()
 *
 * Description:
 *
 *      Routine serializes/deserializes a data of type MEDIA_TYPE.
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
 *
 *	Emanuel Alongi	14-Jul-1993	Created - a copy of csi_xstatus.c.
 */


/*
 *      Header Files:
 */
#include "csi.h"
#include "csi_xdr_pri.h"
#include "ml_pub.h"
#include "cl_pub.h"
 
 
 
/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_xmedia_type()";
 

/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xmedia_type (
    XDR *xdrsp,                         /* xdr handle structure */
    MEDIA_TYPE *media_type              /* drive type */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) media_type);   /* argument list */
#endif /* DEBUG */

    /* translate status */
    if (!xdr_char(xdrsp, (char *)media_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_char()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
