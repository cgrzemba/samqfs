
#ifndef lint
static char SccsId[] = "@(#)csi_xvol_id.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xvol_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes a VOLID structure.
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
static char     *st_module = "csi_xvol_id()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xvol_id (
    XDR *xdrsp,                         /* xdr handle structure */
    VOLID *volidp                        /* volume id */
)
{

    char *cp;           /* used on call to xdr_bytes since need a ptr-to-ptr */
    unsigned int size;  /* size of the identifier to be decoded */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) volidp);       /* argument list */
#endif /* DEBUG */

    /* translate volid */
    cp = volidp->external_label;
    size = sizeof(volidp->external_label);
    if (!xdr_bytes(xdrsp, &cp, &size, size)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_bytes()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);

}
