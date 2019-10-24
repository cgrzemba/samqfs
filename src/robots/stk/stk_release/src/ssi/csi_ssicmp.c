static char SccsId[] = "@(#)csi_ssicmp.c	5.5 11/9/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_ssicmp()
 *
 * Description:
 *
 *      csi_ssicmp() compares its arguments, which are csi transaction id tags,
 *      to find out if the messages sent containing these transaction 
 *      identifiers are from the same ssi.  It returns 0 if they are the same 
 *      ssi, 1 if they are not.
 *
 *
 * Return Values:
 *
 *      1               - transaction id tags are NOT duplicates
 *      0               - transaction id tags ARE duplicates
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
 *      J. A. Wishner       12-Apr-1989.    Created.
 *      J. W. Montgomery    29-Sep-1990.    Modified for OSLAN.
 *      J. A. Wishner       20-Oct-1991.    Delete type, status, st_src; unused.
 *      E. A. Alongi        28-Sep-1992     modifications to differentiate
 *                                          between two distinct ssi's from the
 *                                          same host.
 *	Emanuel A. Alongi   04-Oct-1993.    Corrected errors discovered by flint
 *					    after switching to new compiler.
 */


/*
 *      Header Files:
 */

#include <string.h>
#include "csi.h"
#include "cl_pub.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_ssicmp()";


/*
 *      Procedure Type Declarations:
 */

int 
csi_ssicmp (
    CSI_XID *xid1,                 /* transaction id structure */
    CSI_XID *xid2                 /* transaction id structure */
)
{

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xid1,          /* argument */
                 (unsigned long) xid2);         /* argument */
#endif /* DEBUG */

    
#ifdef ADI 

    /* must be from the same host AND must be the same ssi */
    if (memcmp((const char *) xid1->client_name,
		(const char *) xid2->client_name, CSI_ADI_NAME_SIZE) == 0 &&
                                                     (xid1->pid == xid2->pid)) {

#else /* ADI */

    /* must be from the same host AND must be the same ssi */
    if (memcmp((const char *) xid1->addr, (const char *) xid2->addr,
			sizeof(xid1->addr)) == 0 && (xid1->pid == xid2->pid)) {

#endif /* ADI */

        return(0);  /* is a duplicate */
    }
    return(1);   /* not a duplicate */
}


