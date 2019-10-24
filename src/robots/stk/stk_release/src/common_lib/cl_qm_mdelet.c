static char SccsId[] = "@(#)cl_qm_mdelet.c	5.2 5/3/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_mdelete
 *
 *
 * Description:
 *
 *    Removes a queue member by unlinking it from the chain of members and
 *    then releases the memory associated with the specified member.
 *    Specifically, cl_qm_mdelete() performs the following functions:
 *
 *       o  If qm_mcb is NULL, QM is not initialized and cl_qm_mdelete()
 *          returns FALSE.
 *       o  If the specified queue is undefined, cl_qm_mdelete() returns
 *          FALSE.
 *       o  If the specified queue member is undefined, cl_qm_mdelete()
 *          returns FALSE.
 *       o  The specified queue member is unlinked from the chain of members
 *          and the neighboring link pointers are adjusted accordingly.
 *       o  The number of currently-defined queue members
 *          (qm_mcb->mcb[queue]->status.members) is decremented by one.
 *       o  If the member ID is either the lowest or highest assigned ID in
 *          the queue, increment qcb->lowest or decrement qcb->highest to
 *          indicate that the ID is available.
 *       o  The memory allocated for the member is released.
 *
 *
 * Return Values:
 *
 *    TRUE      The member was successfully deleted.
 *
 *    FALSE     The member could not be deleted for one of the following
 *              reasons:
 *                 - QM is not initialized.
 *                 - The specified queue is undefined.
 *                 - The specified member is undefined.
 *
 *
 * Implicit Inputs:
 *
 *    qm_mcb    Pointer to the master control block (MCB).
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
#include <stdlib.h>
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

BOOLEAN 
cl_qm_mdelete (
    QM_QID queue,   /* Identifier of the queue from which the member is to be  */
    QM_MID member  /* Identifier of the member to be deleted.                 */
)
{
    QM_QCB    *qcb;
    BOOLEAN    deleted  = FALSE;
    QM_MEMBER *mbr;

    if (qm_mcb  &&  queue  &&  member                   &&
        (queue <= qm_mcb->status.max_queues)            &&
        (qcb = qm_mcb->qcb[queue-1]) != NULL            &&
        (mbr = cl_qm_find(qcb, QM_POS_MEMBER, member)) != NULL)  {

        if (mbr->prev)
            mbr->prev->next = mbr->next;

        if (mbr->next)
            mbr->next->prev = mbr->prev;

        if (qcb->first == mbr)
            qcb->first = mbr->next;

        if (qcb->last == mbr)
            qcb->last = mbr->prev;

        if ((mbr->status.mid == qcb->lowest) &&
            (qcb->lowest != qcb->highest))
            qcb->lowest  = (mbr->status.mid == USHRT_MAX)  ?
                           1 : mbr->status.mid + 1;

        qcb->status.last = 0;
        --(qcb->status.members);
        (void)time (&(qcb->status.modified));

        free (mbr);
        deleted = TRUE;
    }

    return deleted;
}
