static char SccsId[] = "@(#)csi_xaccess.c	5.3 11/9/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 * Name:
 *      csi_xaccess_id()
 *
 * Description:
 *      Routine serializes/deserializes an ACCESSID structure.
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
#include "cl_pub.h"
#include "ml_pub.h"
 
/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_src = __FILE__;
static char     *st_module = "csi_xaccess_id()";
 
/*    Procedure Type Declarations: */

bool_t 
csi_xaccess_id (
    XDR *xdrsp,                                /* xdr handle structure */
    ACCESSID *accessidp                    /* accessid */
)
{
    char *cp;            /* used on call to xdr_bytes since need a ptr-to-ptr */
    unsigned int   size; /* size of the identifier to be decoded */


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp, (unsigned long) accessidp);
#endif /* DEBUG */

    /* translate user_id */
    cp = accessidp->user_id.user_label;
    size = sizeof(accessidp->user_id.user_label);
    if (!xdr_bytes(xdrsp, &cp, &size, size)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_bytes():1", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate password */
    cp = accessidp->password.password;
    size = sizeof(accessidp->password.password);
    if (!xdr_bytes(xdrsp, &cp, &size, size)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_bytes():1", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    return(1);
}
