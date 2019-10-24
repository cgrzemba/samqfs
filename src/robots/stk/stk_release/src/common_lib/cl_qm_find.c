static char SccsId[] = "@(#)cl_qm_find.c	5.2 5/3/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_find
 *
 *
 * Description:
 *
 *    Locates a queue member according to the supplied position reference
 *    information.
 *
 *       o  If the specified queue is undefined, cl_qm_find() returns NULL.
 *       o  If the position is QM_POS_FIRST, cl_qm_find() returns a pointer to
 *          the first queue member, if any.
 *       o  If the position is QM_POS_LAST, cl_qm_find() returns a pointer to
 *          the last queue member, if any.
 *       o  If the position is QM_POS_PREV and QM_QSTATUS.last is non-NULL,
 *          cl_qm_find() returns a pointer to the last-accessed member's
 *          predecessor.
 *       o  If the position is QM_POS_NEXT and QM_QSTATUS.last is non-NULL,
 *          cl_qm_find() returns a pointer to the last-accessed member's
 *          successor.
 *       o  If the position is QM_POS_BEFORE, QM_POS_AFTER or QM_POS_MEMBER,
 *          perform a linear search of the queue to locate the specified
 *          member.  Do the following if the member is found:
 *             - If the position is QM_POS_BEFORE, cl_qm_find() returns a
 *               pointer to the member's predecessor, if any.
 *             - If the position is QM_POS_AFTER, cl_qm_find() returns a
 *               pointer to the member's successor, if any.
 *             - If the position is QM_POS_MEMBER, cl_qm_find() returns a
 *               pointer to the member.
 *
 *
 * Return Values:
 *
 *    NULL         A queue member could not be found for one of the following
 *                 reasons:
 *                    - The specified queue is undefined.
 *                    - A member could not be found that satifies the
 *                      specified position reference.
 *
 *    non-NULL     The return value is a pointer to the requested queue
 *                 member.
 *
 *
 * Implicit Inputs:
 *
 *    NONE
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

static QM_MEMBER *search (QM_QCB *qcb, QM_POS position, QM_MID member);

QM_MEMBER *
cl_qm_find (
    QM_QCB *qcb,        /* Ptr to the queue control block of the queue to be    */
    QM_POS position,   /* Specifies how to search for the member.              */
    QM_MID member     /* Specifies the point of reference within the queue    */
)
                    /*   where the search is performed.                     */
{
    QM_MEMBER *mbr  = (QM_MEMBER *) NULL;

    if (qcb                                  &&
        ((int)(QM_POS_A) < (int)(position))  &&
        ((int)(position) < (int)(QM_POS_Z))    )  {
        switch (position)  {
            case QM_POS_FIRST:
		mbr = qcb->first;
		break;

            case QM_POS_LAST:
		mbr = qcb->last;
		break;

            case QM_POS_PREV:
		if ((mbr = search(qcb, QM_POS_MEMBER, qcb->status.last)) !=
		  NULL)
		     mbr = mbr->prev;
		 break;

            case QM_POS_NEXT:
		if ((mbr = search(qcb, QM_POS_MEMBER, qcb->status.last)) !=
		  NULL)
		    mbr = mbr->next;
		break;

            case QM_POS_BEFORE:
            case QM_POS_AFTER:
            case QM_POS_MEMBER:
		mbr = search(qcb, position, member);
		break;

            default:
		break;
        }

        if (mbr)
            qcb->status.last = mbr->status.mid;
    }

    return mbr;
}

static QM_MEMBER *
search (
    QM_QCB *qcb,
    QM_POS position,
    QM_MID member
)
{
    QM_MEMBER *mbr;

    for (mbr=qcb->first;  mbr && (mbr->status.mid != member);  mbr=mbr->next)
        ;

    if (mbr)
        switch (position)  {

            case QM_POS_BEFORE:
		mbr = mbr->prev;
		break;

            case QM_POS_AFTER:
		mbr = mbr->next;
		break;

            case QM_POS_MEMBER:
		mbr = mbr;
	        break;

	    default:
		break;
        }

    return mbr;
}
