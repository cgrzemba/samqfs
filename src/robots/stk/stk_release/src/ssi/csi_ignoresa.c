static char SccsId[] = "@(#)csi_ignoresa.c	5.3 11/9/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_ignoresa()/ cl_inform() private version
 *
 * Description:
 *
 *      Function used in SSI ONLY to effectively disregard calls to cl_inform()
 *      by the CSI and the common_library routines.  It does so by creating
 *      its own cl_inform() function, which does nothing.  This is done because 
 *      the SSI does not require use of an ACSSA.  Messages sent to the ACSSA
 *      using cl_inform() are erroneous when used in the SSI or client side of
 *      the software architecture.
 *
 * Return Values:
 *
 *      NONE
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
 *      Must be loaded in an SSI before the common library routines.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       13-Mar-1989.    Created.
 *      J. A. Wishner       04-Oct-1991.    Changed to a void function to match
 *                                          new definition for cl_inform..
 *      
 *     
 */


/*
 *      Header Files:
 */
#include "cl_pub.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

/*
 *      Procedure Type Declarations:
 */

void 
cl_inform (
    STATUS message_status,         /* unsolicited message status */
    TYPE type,                   /* denotes type of identifier */
    IDENTIFIER *identifier,             /* identifier of component involved */
    int error                  /* internal error code */
)
{
#ifdef DEBUG
    if TRACE(0)
        cl_trace("test ssi version: cl_inform()",       /* routine name */
                 4,                                     /* parameter count */
                 (unsigned long)message_status,
                 (unsigned long)type,
                 (unsigned long)identifier,
                 (unsigned long)error);
#endif /* DEBUG */

    return;

}




