static char SccsId[] = "@(#)cl_qm_macces.c	5.2 5/3/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_maccess
 *
 *
 * Description:
 *
 *    Permits access to the data area of a queue member.  Specifically:
 *
 *       o  If qm_mcb is NULL, QM is not initialized and cl_qm_maccess()
 *          returns NULL.
 *       o  If the specified queue is undefined, cl_qm_maccess() returns NULL.
 *       o  If the specified queue member is undefined, cl_qm_maccess()
 *          returns NULL.
 *       o  QM_MEMBER.status.modified is updated with the current date/time
 *          and cl_qm_maccess() returns a pointer to the data area of the
 *          member.
 *
 *
 * Return Values:
 *
 *    NULL        The specified member could not be accessed for one of the
 *                following reasons:
 *                   - QM is not initialized.
 *                   - The specified queue is undefined.
 *                   - The specified member is undefined.
 *
 *    non-NULL    Represents a pointer to the data area of the specified
 *                member.
 *
 *
 * Implicit Inputs:
 *
 *    qm_mcb          Pointer to the master control block (MCB).
 *
 *
 * Implicit Outputs:
 *
 *    NONE
 *
 *
 * Considerations:
 *
 *    The caller should exercise caution when accessing the data portion of
 *    queue members to stay within the boundaries of the data region.  If the
 *    size of the data area is not known to the caller, it may be obtained
 *    using function cl_qm_mstatus().
 *
 *
 * Module Test Plan:
 *
 *    TBS
 *
 *
 * Revision History:
 *
 *    D.E. Skinner         4-Oct-1988.    Original.
 *        Jim Montgomery        02-Aug-1989             Use ALIGNED_BYTES instead of BYTE.
 */


/*
 * Header Files:
 */

#include "flags.h"
#include "system.h"
#include <stdio.h>
#include <time.h>

#include "defs.h"
#include "cl_qm.h"


/*
 * Defines, Typedefs and Structure Definitions:
 */


/*
 * Global and Static Variable Declarations:
 */


/*
 * Procedure Type Declarations:
 */

ALIGNED_BYTES 
cl_qm_maccess (
    QM_QID queue,    /* Identifier of the queue to be accessed.        */
    QM_MID member   /* Identifier of the queue member to be accessed. */
)
{
    ALIGNED_BYTES data  = (ALIGNED_BYTES) NULL;
    QM_QCB        *qcb;
    QM_MEMBER     *mbr;

    if (qm_mcb  &&  queue  &&  member                   &&
        (queue <= qm_mcb->status.max_queues)            &&
        (qcb = qm_mcb->qcb[queue-1]) != NULL            &&
        (mbr = cl_qm_find(qcb, QM_POS_MEMBER, member)) != NULL)  {
        data = (ALIGNED_BYTES)mbr->data;
        (void)time (&(mbr->status.modified));
    }

    return data;
}
