static char SccsId[] = "@(#)cl_qm_mstatu.c	5.3 9/29/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_mstatus
 *
 *
 * Description:
 *
 *    Returns information about the current state of the specified queue
 *    member.  Specifically, cl_qm_mstatus() performs the following functions:
 *
 *       o  If qm_mcb is NULL, QM is not initialized.  cl_qm_mstatus()
 *          returns NULL.
 *       o  If the specified queue is not defined, cl_qm_mstatus() returns
 *          NULL.
 *       o  If the specified queue member is not defined, cl_qm_mstatus()
 *          returns NULL.
 *       o  sizeof(QM_MSTATUS) bytes of memory are allocated to contain the
 *          status information.  If the allocation is unsuccessful,
 *          cl_qm_mstatus() returns NULL.
 *       o  The contents of QM_MEMBER.status are copied into the allocated
 *          memory and cl_qm_mstatus() returns a pointer to this block of
 *          memory.
 *
 *
 * Return Values:
 *
 *    NULL       Status could not be returned for one of the following
 *               reasons:
 *                  - QM is not initialized.
 *                  - Specified queue is undefined.
 *                  - Specified queue member is undefined.
 *                  - Sufficient memory could not be allocated to contain the
 *                    status information.
 *
 *    non-NULL   Represents a pointer to a structure of type QM_MSTATUS which
 *               contains the current status of the queue member.
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
 *    It is the responsibility of the caller to release the memory allocated
 *    by cl_qm_mstatus() when it is no longer needed.
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
#include <string.h>

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

QM_MSTATUS *
cl_qm_mstatus (
    QM_QID queue,   /* Identifier of the queue to which this member belongs.   */
    QM_MID member  /* Identifier of the queue member for which status is      */
)
                 /*   desired.                                              */
{
    QM_MSTATUS *mstatus  = (QM_MSTATUS *) NULL;
    QM_QCB     *qcb;
    QM_MEMBER  *mbr;

    if (qm_mcb  &&  queue  &&  member                          &&
        (queue <= qm_mcb->status.max_queues)                   &&
        (qcb = qm_mcb->qcb[queue-1]) != NULL                   &&
        (mbr = cl_qm_find(qcb, QM_POS_MEMBER, member)) != NULL &&
        (mstatus = (QM_MSTATUS *) malloc(sizeof(QM_MSTATUS))) != NULL)

        memcpy ((char *)mstatus, (char *)&(mbr->status), sizeof(QM_MSTATUS));

    return mstatus;
}
