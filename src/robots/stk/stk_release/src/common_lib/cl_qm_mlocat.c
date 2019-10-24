static char SccsId[] = "@(#)cl_qm_mlocat.c	5.2 5/3/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_mlocate
 *
 *
 * Description:
 *
 *    Locates a queue member according to the supplied position reference
 *    information.  Specifically:
 *
 *       o  If qm_mcb is NULL, QM is not initialized.  cl_qm_mcreate() returns
 *          zero.
 *       o  If the specified queue is undefined, cl_qm_mcreate() returns zero.
 *
 *
 * Return Values:
 *
 *    0               A queue member could not be found for one of the
 *                    following reasons:
 *                       - QM is not initialized.
 *                       - The specified queue is undefined.
 *                       - A member could not be found that satifies the
 *                         specified position reference.
 *    {1:USHRT_MAX}   A member has been found and its identity is the return
 *                    value.
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
 *    NONE
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
 */


/*
 * Header Files:
 */

#include "flags.h"
#include "system.h"
#include <stdio.h>

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

QM_MID 
cl_qm_mlocate (
    QM_QID queue,      /* Identifier of the queue to be searched.              */
    QM_POS position,   /* Specifies how to search for the member.              */
    QM_MID member     /* Specifies the point of reference within the queue    */
)
                    /*   where the search is performed.                     */
{
    QM_QCB    *qcb;
    QM_MEMBER *mbr  = (QM_MEMBER *) NULL;

    if (qm_mcb  &&  queue                     &&
        (queue <= qm_mcb->status.max_queues)  &&
        (qcb = qm_mcb->qcb[queue-1]) != NULL)
        mbr = cl_qm_find(qcb, position, member);

    return (mbr ? mbr->status.mid : 0);
}
