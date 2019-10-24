static char SccsId[] = "@(#)cl_qm_qdelet.c	5.2 5/3/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_qdelete
 *
 *
 * Description:
 *
 *    Removes a queue by releasing the memory associated with each queue
 *    member as well as the queue control block (QCB).  Specifically,
 *    cl_qm_qdelete() performs the following functions:
 *
 *       o  If qm_mcb is NULL, QM is not initialized and cl_qm_delete()
 *          returns FALSE.
 *       o  If qm_mcb is non-NULL, qm_mcb->qcb[] is examined to see if the
 *          queue is defined.  If it is not defined, cl_qm_qdelete() returns
 *          FALSE.
 *       o  If the queue is defined, it is emptied by invoking free() against
 *          each queue member.
 *       o  The memory allocated for the QCB is released and the related entry
 *          in qm_mcb->qcb[] is set to NULL.  cl_qm_qdelete() returns TRUE.
 *
 *
 * Return Values:
 *
 *    TRUE       The queue was successfully deleted.
 *
 *    FALSE      The queue was not deleted for one of the following reasons:
 *                  - QM is not initialized.
 *                  - Specified queue does not exist.
 *
 *
 * Implicit Inputs:
 *
 *    qm_mcb     Pointer to the master control block (MCB).
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
#include <time.h>
#include <stdlib.h>

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
cl_qm_qdelete (
    QM_QID queue   /* Identifier of the queue to be deleted. */
)
{
    QM_QCB    *qcb;
    BOOLEAN    deleted  = FALSE;
    QM_MEMBER *mbr;

    if (qm_mcb  &&  queue                     &&
        (queue <= qm_mcb->status.max_queues)  &&
        (qcb = qm_mcb->qcb[queue-1]) != NULL)  {
        for (mbr=qcb->first;  mbr;) {
	    QM_MEMBER *mbr_before_free = mbr;

	    mbr = mbr->next;
            free (mbr_before_free);
	}
        free (qcb);
        qm_mcb->qcb[queue-1] = (QM_QCB *) NULL;
        --(qm_mcb->status.queues);
        (void)time (&(qm_mcb->status.modified));
        deleted = TRUE;
    }

    return deleted;
}
