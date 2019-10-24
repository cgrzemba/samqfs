static char SccsId[] = "@(#)cl_qm_mcreat.c	5.2 5/3/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_mcreate
 *
 *
 * Description:
 *
 *    Creates a new queue member by allocating memory for the member and then
 *    adding the member to the linked list of members at the specified
 *    position.  Specifically:
 *
 *       o  If qm_mcb is NULL, QM is not initialized.  cl_qm_mcreate() returns
 *          zero.
 *       o  If the specified queue is undefined, cl_qm_mcreate returns zero.
 *       o  If the maximum number of members
 *          (qm_mcb->qcb[queue]->status.max_members) is non-zero and the 
 *          number of members (...->status.members) has reached the limit,
 *          ql_qm_mcreate() returns zero.
 *       o  Locate the point in the linked list of members according to the
 *          position and member arguments.  If the location cannot be found,
 *          cl_qm_mcreate() returns zero.
 *       o  Using function cl_qm_create(), create a new queue member of the
 *          specified size and splice into the linked list of members at the
 *          specified point.
 *
 *
 * Return Values:
 *
 *    0               A new queue member could not be created for one of the
 *                    following reasons:
 *                       - QM is not initialized.
 *                       - The specified queue is undefined.
 *                       - cl_qm_create() is unable to create a new member.
 *
 *    {1:USHRT_MAX}   A new member has been defined and its identity is the
 *                    return value.
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

#define  BEFORE  TRUE
#define  AFTER   FALSE


/*
 * Global and Static Variable Declarations:
 */


/*
 * Procedure Type Declarations:
 */

QM_MID 
cl_qm_mcreate (
    QM_QID queue,      /* Identifier of the queue to which the new     */
    QM_POS position,   /* Specifies where the new member should be     */
    QM_MID member,     /* Specifies the point of reference within the  */
    unsigned short size       /* Size of the data area (in bytes).            */
)
{
    QM_QCB    *qcb;
    QM_MEMBER *mbr  = (QM_MEMBER *) NULL;

    if (qm_mcb                                                 &&
        queue                                                  &&
        (queue <= qm_mcb->status.max_queues)                   &&
        ((int)(QM_POS_A) < (int)(position))                    &&
        ((int)(position) < (int)(QM_POS_Z))                    &&
        (qcb = qm_mcb->qcb[queue-1]) != NULL                   &&
        (  !qcb->status.max_members      ||
           (qcb->status.max_members  &&
            (qcb->status.members < qcb->status.max_members)))    )  {

        switch (position)  {

            case QM_POS_FIRST :  mbr = cl_qm_create(qcb, BEFORE,
                                                    qcb->first, size);
                                 break;

            case QM_POS_LAST  :  mbr = cl_qm_create(qcb, AFTER,
                                                    qcb->last, size);
                                 break;

            case QM_POS_PREV  :  if ((mbr = cl_qm_find(qcb, QM_POS_MEMBER,
				   qcb->status.last)) != NULL)
                                     mbr = cl_qm_create(qcb, BEFORE,
                                                        mbr, size   );
                                 break;

            case QM_POS_NEXT  :  if ((mbr = cl_qm_find(qcb, QM_POS_MEMBER,
				   qcb->status.last)) != NULL)
                                     mbr = cl_qm_create(qcb, AFTER,
                                                        mbr, size  );
                                 break;

            case QM_POS_BEFORE:  if ((mbr = cl_qm_find(qcb, QM_POS_MEMBER,
				   member)) != NULL)
                                     mbr = cl_qm_create(qcb, BEFORE,
                                                        mbr, size   );
                                 break;

            case QM_POS_AFTER :
            case QM_POS_MEMBER:  if ((mbr = cl_qm_find(qcb, QM_POS_MEMBER,
				   member)) != NULL)
                                     mbr = cl_qm_create(qcb, AFTER,
                                                        mbr, size  );
                                 break;
	    default:
		break;
        }
    }

    return (mbr ? mbr->status.mid : 0);
}
