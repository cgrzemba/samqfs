static char SccsId[] = "@(#)cl_qm_create.c	5.5 12/2/93 (c) 1991 StorageTek";
/*
 * Copyright (C) 1988,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *    cl_qm_create
 *
 *
 * Description:
 *
 *    Allocates memory for a new queue member and splices it into the queue
 *    member chain for the specified queue.  Specifically:
 *
 *       o  If the size of the data area is less than the minimum allocation
 *          size of QM_MEMBER (currently sizeof(int)), cl_qm_create() returns
 *          NULL.
 *       o  If the size of the data area plus the size of the queue member
 *          header (QM_MEMBER) would exceed USHRT_MAX, cl_qm_create() returns
 *          NULL.  The arguments of the memory allocation function calloc()
 *          limit the total number of bytes allocated to USHRT_MAX or less.
 *       o  An ID is allocated for the new queue member.  If no IDs are
 *          available (i.e., there are USHRT_MAX queue members defined),
 *          cl_qm_create() returns NULL.
 *       o  Memory is allocated for the new queue member.  If unable to
 *          allocate memory, cl_qm_create() returns NULL.
 *       o  The status structure for the queue is updated to reflect the new
 *          member.
 *       o  The member ID, size and date/time of creation are placed into the
 *          status structure of the member header.
 *       o  If the reference member argument is NULL, this is the first member
 *          in the queue.  Set the first and last pointer in the queue control
 *          block to point to this member and return with a pointer to the new
 *          member.
 *       o  A reference member is specified.  Add the new member before or
 *          after the specified reference member by changing the linkage
 *          pointers in the member headers of the reference, previous/next
 *          member, and the new member.  Return with a pointer to the new
 *          member.
 *
 *
 * Return Values:
 *
 *    NULL         A new queue member could not be created for one of the
 *                 following reasons:
 *
 *    non-NULL     A new member has been defined and the return value is a
 *                 pointer to the member.
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
 *    The lifetimes of the members of a queue should be approximately the
 *    same, otherwise allocate_mid() may be forced into its time-consuming
 *    search of the entire queue for an unused member ID.
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
 *    Mike Williams        5-Jan-2011     When allocating memory for a queue
 *                                        member, allocate entire size request
 */


/*
 * Header Files:
 */

#include "flags.h"
#include "system.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "cl_pub.h"
#include "ml_pub.h"
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

static QM_MID allocate_mid (QM_QCB *qcb);

QM_MEMBER *
cl_qm_create (
    QM_QCB *qcb,      /* Ptr to the queue control block of the queue to */
    BOOLEAN before,   /* Specifies whether the new member should be     */
    QM_MEMBER *member,   /* Specifies the point of reference within the    */
    unsigned short size     /* Size of the data area (in bytes).              */
)
{
    QM_MID     mid;
    QM_MEMBER *mbr  = (QM_MEMBER *) NULL;

    if (qcb                                                              &&
        (((long) size) < ((long) USHRT_MAX - (long) sizeof(QM_MEMBER)))  &&
        (mid = allocate_mid(qcb)) != 0                                   &&
        (mbr = (QM_MEMBER *) calloc(1, sizeof(QM_MEMBER) + size)) != NULL)
        {
        
        ++(qcb->status.members);
        (void)time (&(qcb->status.modified));

        mbr->status.mid = mid;
        mbr->status.size = size;
        (void)time (&(mbr->status.created));

        if (member)  {
            if (before)  {
                if (member->prev)  {
                    member->prev->next = mbr;
                    mbr->prev = member->prev;
                }
                else
                    qcb->first = mbr;
                mbr->next = member;
                member->prev = mbr;
            }
            else  {
                if (member->next)  {
                    member->next->prev = mbr;
                    mbr->next = member->next;
                }
                else
                    qcb->last = mbr;
                member->next = mbr;
                mbr->prev = member;
            }
        }

        else
            qcb->first = qcb->last = mbr;
    }
    if(!mbr) {
       MLOGDEBUG(0,(MMSG(499, "cl_qm_mcreate: errno = %d (\"%s\")"),
	errno, strerror(errno)));
    }
    return mbr;
}

static QM_MID 
allocate_mid (
    QM_QCB *qcb
)
{
    long       i;
    BYTE      *table;
    QM_MID     mid  = 0;
    QM_MID     t;
    QM_MEMBER *p;

    /*
     *   The queue is empty ... if first member, start everything at 1
     */
    if (!(qcb->status.members) && !(qcb->lowest))
        mid = qcb->lowest = qcb->highest = 1;

    else  {

        /*
         *   An ID is available because highest has not yet bumped into
         *   lowest.
         */
        t = (qcb->highest == USHRT_MAX)  ?  1 : qcb->highest + 1;
        if (t != qcb->lowest)
            mid = qcb->highest = t;

        /*
         *   All ID's appear to be used, perhaps there is a hole.  Scan the
         *   chain of members, recording each member ID in a table, and then
         *   scan the table backwards to see if an unused ID exists.
         */
        else if ((table = (BYTE *) calloc(USHRT_MAX, 1)) != NULL) {
            for (p = qcb->first;  p;  p = p->next)
                *(table + p->status.mid - 1) = 1;
            for (i=USHRT_MAX-1;  (i >= 0) && *(table+i);  --i)
                ;
            if (i >= 0) {
                mid = (QM_MID)(i+1);
                *(table + mid - 1) = 1;
            }
            /*
             *   Update lowest to eliminate future holes.
             */
            while (qcb->lowest != qcb->highest)
                if (*(table + qcb->lowest - 1)) break;
                else qcb->lowest = (qcb->lowest == USHRT_MAX)  ?
                                    1 : qcb->lowest + 1;
            free (table);
        }
    }

    return mid;
}
