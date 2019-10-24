
#ifndef lint
static char SccsId[] = "@(#)csi_xreqsumm.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xreqsummary()
 *
 * Description:
 *
 *      Routine serializes/deserializes an XXXX structure.
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
static char     *st_module = "csi_xreqsummary()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xreqsummary (
    XDR *xdrsp,                         /* xdr handle structure */
    REQ_SUMMARY *sump                          /* requests */
)
{

    register int dispo;                 /* disposition loop counter */ 
    register int cmd;                   /* command loop counter */ 

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) sump); /* argument list */
#endif /* DEBUG */

    for (cmd = 0; cmd < (int)MAX_COMMANDS; cmd++) {

        /* translate the matrix of requests[][] (type MESSAGE_ID) */
        for (dispo=0; dispo < (int)MAX_DISPOSITIONS; dispo++) {

            /* translate requests */
            if (!csi_xmsg_id(xdrsp, &sump->requests[cmd][dispo])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xmsg_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }

        } /* end of loop on command */

    } /* end of loop on disposition */

    return(1);

}
