static char SccsId[] = "@(#)cl_qm_qcreat.c	5.3 10/12/94 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_qcreate
 *
 *
 * Description:
 *
 *    Creates a new queue by allocating memory for a queue control block (QCB)
 *    and then entering the address of the QCB into the master control block
 *    (MCB).  Specifically, cl_qm_qcreate() performs the following functions:
 *
 *       o  If qm_mcb is NULL, QM is not initialized.  cl_qm_qcreate() returns
 *          zero.
 *       o  If the number of queues currently defined (qm_mcb->status.queues)
 *          equals the maximum number of queues permitted
 *          (qm_mcb->status.max_queues), cl_qm_qcreate() returns zero.
 *       o  sizeof(QM_QCB) bytes of memory (plus the length of the optional
 *          remarks string) are allocated.  If the allocation is unsuccessful,
 *          cl_qm_qcreate() returns zero.
 *       o  If the allocation is successful, the various QCB fields are
 *          initialized and the address of the QCB is placed into
 *          qm_mcb->qcb[] at the next available position.  cl_qm_qcreate()
 *          returns the index of qm_mcb->qcb[] as the queue identifier.
 *
 *
 * Return Values:
 *
 *    0               A new queue could not be defined for one of the
 *                    following reasons:
 *                       - QM is not initialized.
 *                       - The maximum number of queues has been reached.
 *                       - Unable to allocate memory for a QCB.
 *
 *    {1:USHRT_MAX}   A new queue has been defined and its identity is the
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
 *    A test driver is supplied within this module which performs the
 *    following tests:
 *
 *       o  QM not initialized
 *       o  number of queues defined has reached max_queues
 *       o  NULL remarks
 *       o  short remarks (length=0, 1, 2, 3, 4)
 *       o  long remarks (MAX_QM_REMARK-4, MAX_QM_REMARK, MAX_QM_REMARK+4).
 *          In testing both short and long remarks, the field of trailing
 *          NULLs is checked.
 *
 *
 * Revision History:
 *
 *      D.E. Skinner            4-Oct-1988      Original.
 *      D. F. Reed              13-Jan-1989     Moved MAX macro to cl_qm.h.
 *	D. A. Myers		12-Oct-1994	Porting changes
 */


/*
 * Header Files:
 */


#include "flags.h"
#include "system.h"
#include <stdlib.h>
#include <string.h>
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

QM_QID 
cl_qm_qcreate (
    unsigned short max_members,    /* Maximum number of members that may be    */
    char *remarks        /* Ptr to a string describing this queue.   */
)
                                /*   A value of NULL or a ptr to a null     */
                                /*   string indicates that no remarks are   */
                                /*   defined.                               */
{
    QM_QCB       *qcb;
    QM_QID        qid             = 0;
    unsigned int  remarks_length  = 0;
    unsigned int  qcb_size        = sizeof(QM_QCB);

    if (qm_mcb && (qm_mcb->status.queues < qm_mcb->status.max_queues))  {
        if (remarks)
            /*
             *  Allocate additional space for remarks in units of
             *  sizeof(int) bytes
             */
            remarks_length = ((MAX((int)strlen(remarks), QM_MAX_REMARK) + 1) /
                sizeof(int) ) * sizeof(int);
    
        qcb_size += remarks_length;

        if ((qcb = (QM_QCB *) calloc(1, qcb_size)) != NULL) {
            /*
             *  Locate the first empty spot in the master control block
             */
            for (qid=1;
                 (qid <= qm_mcb->status.max_queues) && (qm_mcb->qcb[qid-1]);
                 ++qid)
                ;
    
            if (qid > qm_mcb->status.max_queues)  {
                qm_mcb->status.queues = qm_mcb->status.max_queues;
                qid = 0;
            }
            else  {
                qm_mcb->qcb[qid-1] = qcb;
                ++(qm_mcb->status.queues);
                (void)time(&(qm_mcb->status.modified));
                qcb->status.qid         = qid;
                qcb->status.max_members = max_members;
                (void)time(&(qcb->status.created));
                if (remarks)
                    strncpy ((char *) qcb->status.remarks, remarks,
                             QM_MAX_REMARK);
            }
        }
    }

    return qid;
}

#ifdef TEST


#ifdef NULL
#undef NULL
#endif
#include <stdio.h>


#define  MAX_QUEUES  5   /* Maximum number of queues for this test */

void     main              ();
BOOLEAN  test_1            ();
BOOLEAN  test_2            ();
BOOLEAN  test_3            ();
BOOLEAN  test_rem          ();
BOOLEAN  validate_qcb      ();
BOOLEAN  validate_remarks  ();


void 
main (
    int argc,
    char *argv[]
)
{
    BOOLEAN  passed  = FALSE;
    int      i;
    int      j;

    if (!cl_qm_init((unsigned short)MAX_QUEUES, "cl_qm_qcreate() test"))
        printf ("unable to initialize QM\n");

    else  {
        passed = test_1() && test_2() && test_3();

        for (i=0;  passed && (i < sizeof(int)+2);  ++i)
            passed = passed && test_rem(i+1, i);

        for (i=QM_MAX_REMARK-sizeof(int)-2, j=100;
                 passed && (i < QM_MAX_REMARK+sizeof(int)+2);  ++i, ++j)
            passed = passed && test_rem(j, i);
    }

    exit (!passed);
}



/*
 * test_1:  check that cl_qm_qcreate() recognizes when the queue manager
 *          is not initialized.
 */

BOOLEAN 
test_1 (void)
{
    BOOLEAN  successful  = TRUE;
    QM_MCB  *dummy;

    dummy = qm_mcb;
    qm_mcb = (QM_MCB *) NULL;

    if (cl_qm_qcreate((unsigned short)0, NULL))  {
        printf ("cl_qm_qcreate() did not recognize that the queue manager ");
        printf ("was not initialized\n");
        successful = FALSE;
    }

    qm_mcb = dummy;

    return successful;
}



/*
 * test_2:  check that cl_qm_qcreate() rejects an invocation where
 *          the number of queues defined has reached max_queues.
 */

BOOLEAN 
test_2 (void)
{
    int      i;
    BOOLEAN  successful  = FALSE;

    for (i=0;  (i < MAX_QUEUES+3) && cl_qm_qcreate((unsigned short)0, NULL);  ++i)
        ;

    if (i < MAX_QUEUES)  {
        printf ("cl_qm_qcreate() failed to create the permitted maximum ");
        printf ("number of queues ... failed after creating %d queues, ", i);
        printf ("maximum permitted=%d\n", MAX_QUEUES);
    }
    else if (i > MAX_QUEUES)  {
        printf ("cl_qm_qcreate() failed to stop creating queues when it ");
        printf ("reached max_queues ... stopped after creating %d queues, ",
                i);
        printf ("maximum permitted=%d\n", MAX_QUEUES);
    }
    else  {
        successful = TRUE;
        for (i=0;  successful && (i < MAX_QUEUES);  ++i)
            successful = successful && validate_qcb(i+1, 0, NULL);
    }

    for (i=0;  i < MAX_QUEUES;  ++i)
        if (qm_mcb->qcb[i])  {
            free(qm_mcb->qcb[i]);
            qm_mcb->qcb[i] = (QM_QCB *) NULL;

        }
    qm_mcb->status.queues = 0;

    return successful;
}



/*
 * test_3:  check that cl_qm_qcreate() correctly creates a queue control block
 *          when the remarks pointer contains NULL.
 */

BOOLEAN 
test_3 (void)
{
    int      i;
    QM_QID   qid;
    BOOLEAN  successful  = FALSE;

    if ( !(qid = cl_qm_qcreate((unsigned short)10, NULL)) )
        printf ("cl_qm_qcreate(10, NULL) failed\n");
    else
        successful = validate_qcb(qid, 10, NULL);

    for (i=0;  i < MAX_QUEUES;  ++i)
        if (qm_mcb->qcb[i])  {
            free(qm_mcb->qcb[i]);
            qm_mcb->qcb[i] = (QM_QCB *) NULL;
        }
    qm_mcb->status.queues = 0;

    return successful;
}



BOOLEAN 
test_rem (
    unsigned int max_members,
    unsigned int remarks_length
)
{
    int      i;
    char     remarks [2*QM_MAX_REMARK];
    char     c;
    QM_QID   qid;
    BOOLEAN  successful  = FALSE;

    if (remarks_length < 2*QM_MAX_REMARK)  {

        for (i=0, c='a'-1;  i < 2*QM_MAX_REMARK;
                            remarks[i++] = (c =  ++c > 'z' ? 'a' : c) )
            ;
        remarks[remarks_length] = '\0';

        if (qid = cl_qm_qcreate((unsigned short)max_members, remarks))
            successful = validate_qcb(qid, max_members, remarks);
    }

    for (i=0;  i < MAX_QUEUES;  ++i)
        if (qm_mcb->qcb[i])  {
            free(qm_mcb->qcb[i]);
            qm_mcb->qcb[i] = (QM_QCB *) NULL;
        }
    qm_mcb->status.queues = 0;

    return successful;
}



BOOLEAN 
validate_qcb (
    QM_QID qid,
    unsigned short max_members,
    char *remarks
)
{
    int      i;
    int      num_queues;
    BOOLEAN  successful  = FALSE;

    if (qid)  {
        if (qm_mcb->qcb[qid-1]->first)
            printf ("QM_QCB.first is non-NULL\n");
        else if (qm_mcb->qcb[qid-1]->last)
            printf ("QM_QCB.last is non-NULL\n");
        else if (qm_mcb->qcb[qid-1]->status.qid != qid)  {
            printf ("queue ID saved in queue control block disagrees with ");
            printf ("expected queue ID ... expected=%u, saved=%u",
                    qid, qm_mcb->qcb[qid-1]->status.qid);
        }
        else if (qm_mcb->qcb[qid-1]->status.max_members != max_members)  {
            printf ("max_members saved in queue control block disagrees ");
            printf ("with the expected max_members ... expected=%u, saved=%u",
                    max_members, qm_mcb->qcb[qid-1]->status.max_members);
        }
        else if (qm_mcb->qcb[qid-1]->status.last)
            printf ("last member ID is non-zero\n");
        else if (!(qm_mcb->qcb[qid-1]->status.created))
            printf ("date/time created is NULL\n");
        else if (qm_mcb->qcb[qid-1]->status.audited)
            printf ("date/time last audited is non-NULL\n");

        else  {
            for (i=0, num_queues=0;  i < MAX_QUEUES;  ++i)
                if (qm_mcb->qcb[i])
                    ++num_queues;
            if (qm_mcb->status.queues != num_queues)  {
                printf ("number of queues in master control block ");
                printf ("disagrees with the computed number of queues ... ");
                printf ("MCB value=%u, computed=%d\n",
                        qm_mcb->status.queues, num_queues);
            }
            else
                successful = validate_remarks (qid, remarks);
        }
    }

    return successful;
}



BOOLEAN 
validate_remarks (
    QM_QID qid,
    char *remarks
)
{
    int           i;
    char         *p;
    BOOLEAN       successful  = FALSE;
    unsigned int  expected_nulls;   /* Number of nulls expected to trail    */
                                    /*   remarks.                           */
    unsigned int  expected_chars;   /* Number of remarks characters         */
                                    /*   expected.                          */

    if (remarks)
        expected_chars = MAX(strlen(remarks), QM_MAX_REMARK);
    else
        expected_chars = 0;
    expected_nulls = sizeof(int) - expected_chars%sizeof(int);

    for (i = 0,  p = qm_mcb->qcb[qid-1]->status.remarks;
         i < expected_chars;  ++i, ++p)
        if (!(*p))  {
            printf ("unexpected null character at position %d of ", i);
            printf ("QM_QCB->status.remarks\n");
            break;
        }

    if (i >= expected_chars)  {
        for (i=0;  i < expected_nulls;  ++i, ++p)  {
            if (*p)  {
                printf ("unexpected non-null character 0x%X at position %d ",
                       (int) *p, expected_chars+i);
                printf ("of QM_QCB->status.remarks\n");
                break;
            }
        }
        successful = (i >= expected_nulls);
    }

    return successful;
}
#endif
