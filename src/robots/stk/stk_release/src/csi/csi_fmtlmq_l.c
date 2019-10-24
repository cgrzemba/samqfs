static char SccsId[] = "@(#)csi_fmtlmq_l.c	5.6 2/9/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *      csi_fmtlmq_log()
 *
 * Description:
 *      Function formats data stored on the ACSLM connectin queue for writing
 *      to the event log.  This is called with a pointer to the data area of a
 *      queue member which is of type CSI_HEADER.  Extracts and formats the 
 *      following information from the csi header:
 *
 *      o return address, 
 *      o port number, 
 *      o ssi_identifier, 
 *      o protocol type,
 *      o connection type
 *
 * Return Values:
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *      Returns a formatted string message in "string".
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       05-Jan-1989.    Created.
 *      J. A. Wishner       15-May-1989.    Bugfix.  Made so CSI_HEADER* is 
 *                                          passed instead of char *datap.  Log
 *                                          entries printed were hosed, as was
 *                                          the stack.
 *      J. W. Montgomery    29-Sep-1990.    Modified for OSLAN.
 *      J. A. Wishner       20-Oct-1991.    Delete st_src; unused.
 *      E. A. Alongi        30-Oct-1992     Replaced bcopy with memcpy.
 *      E. A. Alongi        04-Nov-1992     When passing error message info,
 *                                          convert ulong Internet address to
 *                                          dotted-decimal equiv using inet_ntoa
 *	Emanuel A. Alongi   01-Oct-1993.    Corrected flint errors.
 *	Emanuel A. Alongi   11-Nov-1993.    Cleaned up flint detected errors.
 *	Emanuel A. Alongi   09-Feb-1994.    Corrected parameter to inet_ntoa()
 *					    and added its prototype.
 */


/*      Header Files: */
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include "cl_pub.h"
#include "csi.h"
#include "system.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static  char *st_module = "csi_fmtlmq_log()";

/*      Global and Static Prototype Declarations: */

#ifndef ADI
char *inet_ntoa(struct in_addr in);
#endif

/*      Procedure Type Declarations: */

void 
csi_fmtlmq_log (
    CSI_HEADER *cs_hdrp,       /* pointer to data area, must be csi header */
    char *stringp,       /* place to place formatted string */
    int maxsize       /* max size of string buffer */
)
{
#ifndef ADI
    struct in_addr netaddr;   /* input parameter to inet_ntoa() */
#endif

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, 3, (unsigned long) cs_hdrp, (unsigned long) stringp,
         (unsigned long) maxsize);
#endif /* DEBUG */

#ifndef ADI /* the RPC code: */

    memcpy((char *) &netaddr.s_addr,
			(const char *) &cs_hdrp->csi_handle.raddr.sin_addr,
							sizeof(unsigned long));

    /* convert ulong inet addr to dotted-decimal notation for log msg */
    sprintf(stringp, "Dropping from Queue: Remote Internet address: %s,"
      "Port: %d\n, ssi_identifier: %ld, Protocol: %d, Connect type: %d",
	    inet_ntoa(netaddr), cs_hdrp->csi_handle.raddr.sin_port,
	    cs_hdrp->ssi_identifier, cs_hdrp->csi_proto, cs_hdrp->csi_ctype);

#else /* ADI */ /* the OSLAN code: */

    sprintf(stringp, "Dropping from Queue: Adman name:%s, ssi_id entifier:%ld,"
      "Protocol:%d, Connect type:%d", 
        cs_hdrp->csi_handle.client_name, cs_hdrp->ssi_identifier,
        cs_hdrp->csi_proto, cs_hdrp->csi_ctype);

#endif /* ADI */
}
