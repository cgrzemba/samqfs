#ifndef lint
static char SccsId[] = "@(#)csi_xidcmp.c	5.3 11/9/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *      csi_xidcmp()
 *
 * Description:
 *      csi_xidcmp() compares its arguments, which are csi transaction id tags,
 *      to find out if the messages sent containing these transaction 
 *      identifiers are duplicates.  Returns 0 if the transaction id tags are
 *      equal, 1 if they are not.
 *
 * Return Values:
 *      1               - transaction id tags are NOT duplicates
 *      0               - transaction id tags ARE duplicates
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       12-Apr-1989.    Created.
 *      J. W. Montgomery    29-Sep-1990.    Modified for OSLAN.
 *      J. A. Wishner       20-Oct-1991.    Delete type, status; unused.
 *      E. A. Alongi        30-Oct-1992.    Replace bcmp with memcmp.
 */

/*      Header Files: */
#include "csi.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static  char *st_module = "csi_xidcmp()";
static  char *st_src = __FILE__;

/*      Procedure Type Declarations: */

int 
csi_xidcmp (
    CSI_XID *xid1,                 /* transaction id structure */
    CSI_XID *xid2                 /* transaction id structure */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xid1, (unsigned long) xid2);
#endif

#ifdef ADI
    if (0 != strncmp(xid1->client_name, xid2->client_name, CSI_ADI_NAME_SIZE)) {
        return(1);                              /* different client names */
    }
    if (xid1->proc != xid2->proc) {
        return(1);                              /* different proc fields */
    }
#else
    if (0 != memcmp(xid1->addr, xid2->addr, sizeof(xid1->addr))) {
        return(1);                              /* different net address */
    }
#endif /* ADI */

    if (xid1->pid != xid2->pid) {
        return(1);                              /* different process id's */
    }
    if (xid1->seq_num != xid2->seq_num) {
        return(1);                              /* different sequence#'s */
    }
    return(0);                                  /* is a duplicate */
}
