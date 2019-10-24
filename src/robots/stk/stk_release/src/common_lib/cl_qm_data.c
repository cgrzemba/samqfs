static char SccsId[] = "@(#)cl_qm_data.c	5.2 5/3/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_data
 *
 *
 * Description:
 *
 *    Defines the internal data objects used by the common library queue
 *    manager (QM) functions.
 *
 *
 * Return Values:
 *
 *    N/A
 *
 *
 * Implicit Inputs:
 *
 *    N/A
 *
 *
 * Implicit Outputs:
 *
 *    N/A
 *
 *
 * Considerations:
 *
 *    NONE 
 *
 *
 * Module Test Plan:
 *
 *    NONE
 *
 *
 * Revision History:
 *
 *    D.E. Skinner         4-Oct-1988.    Original.
 */


/*
 * Header Files:
 */

#include "flags.h"
#include "system.h"
#include "cl_qm.h"

/*
 * Defines, Typedefs and Structure Definitions:
 */


/*
 * Global and Static Variable Declarations:
 */

QM_MCB  *qm_mcb  =      /* Pointer to the master control block (MCB).    */
   (QM_MCB *)0;         /*   If NULL, the QM has not been initialized.   */


/*
 * Procedure Type Declarations:
 */
