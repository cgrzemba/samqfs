static char SccsId[] = "@(#)cl_qm_init.c	5.4 10/12/94 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *    cl_qm_init
 *
 *
 * Description:
 *
 *    Brings the common library queue manager (QM) into existence by
 *    allocating and initializing the master control block (MCB).
 *    Specifically, cl_qm_init() performs the following functions:
 *
 *       o  If qm_mcb is non-NULL, the queue manager is already initialized
 *          and cl_qm_init() reallocates the MCB with sufficient space to
 *          "add" the new max_queues values to the existing max_queues value.
 *       o  If max_queues is less than one, cl_qm_init() returns FALSE.
 *       o  Compute the number of bytes needed for the MCB according to the
 *          value of max_queues and the length of the remarks.
 *       o  Attempt to allocate memory for the MCB.  If unsuccessful,
 *          cl_qm_init() returns FALSE.
 *       o  If memory is successfully allocated, qm_mcb is set to the address
 *          of this memory block and the various MCB members are initialized. 
 *          cl_qm_init() returns TRUE.
 *
 *
 * Return Values:
 *
 *    TRUE     QM was successfully initialized and is ready for service.
 *
 *    FALSE    QM could not be initialized for one of the following reasons:
 *                - QM is already initialized.
 *                - max_queues contained zero.
 *                - Unable to allocate memory for the MCB.
 *
 *
 * Implicit Inputs:
 *
 *    qm_mcb   Pointer to the master control block (MCB).
 *
 *
 * Implicit Outputs:
 *
 *    qm_mcb   If successfully initialized, QM sets qm_mcb to the address of
 *             the MCB.
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
 *       o  QM already initialized (qm_hdr set to non-NULL)
 *       o  max_queues set to zero
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
 *      D. F. Reed              04-May-1989     Modified logic to allow
 *                      multiple qm_init calls to simply update the previous
 *                      MCB, rather than error.
 *      D. F. Reed              15-May-1989     Log event on error from system
 *                      memory allocation request.
 *	D. A. Myers		12-Oct-1994	Porting changes
 */


/*
 * Header Files:
 */

#include "flags.h"
#include "system.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
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

BOOLEAN 
cl_qm_init (
    unsigned short max_queues,   /* Maximum number of queues that may be       */
    char *remarks      /* Ptr to a string describing this QM session.*/
)
                              /*   A value of NULL or a ptr to a null string*/
                              /*   string indicates that no remarks are     */
                              /*   defined.                                 */
{
    unsigned int  i;
    BOOLEAN       initialized     = FALSE;
    unsigned int  remarks_length  = 0;
    unsigned int  mcb_size        = sizeof(QM_MCB);
    QM_MCB        *old_mcb;
    static char *self = "cl_qm_init";

    if (qm_mcb  &&  max_queues)  {
        /*
         *  mcb update ... allocate new memory for update mcb and copy
         *  over previously defined queue info and free old mcb.
         */
        old_mcb = qm_mcb;
        if (remarks)
           /*
            *  Allocate additional space for remarks in units of
            *  sizeof(int) bytes
            */
            remarks_length = ((MAX((int)strlen(remarks), QM_MAX_REMARK) + 1) /
                sizeof(int) ) * sizeof(int);
    
        max_queues += old_mcb->status.max_queues;
        mcb_size += (max_queues-1)*sizeof(QM_QCB *) + remarks_length;

        if ((qm_mcb = (QM_MCB *) calloc(1, mcb_size)) != NULL) {
            qm_mcb->status.max_queues = max_queues;
            qm_mcb->status.created = old_mcb->status.created;
            (void)time(&qm_mcb->status.modified);
            if (remarks)
                qm_mcb->status.remarks =
                    strncpy ((char *) &(qm_mcb->qcb[max_queues]),
                                                         remarks,
                                                   QM_MAX_REMARK );
            initialized = TRUE;
            /*
             *  now, update new qcb with existing qcb elements.
             *  then free up old mcb memory.
             */
            for (i = 0; i < old_mcb->status.max_queues; i++)
                qm_mcb->qcb[i] = old_mcb->qcb[i];
            free(old_mcb);
        }
        else {
            /*
             * memory request failed, log an error.
             */
            MLOG((MMSG(500, "%s: calloc()[1] failed, errno=%d\n"), self, errno));
        }
    }
    else if (!qm_mcb  &&  max_queues)  {
        if (remarks)
           /*
            *  Allocate additional space for remarks in units of
            *  sizeof(int) bytes
            */
            remarks_length = ((MAX((int)strlen(remarks), QM_MAX_REMARK) + 1) /
                sizeof(int) ) * sizeof(int);
    
        mcb_size += (max_queues-1)*sizeof(QM_QCB *) + remarks_length;

        if ((qm_mcb = (QM_MCB *) calloc(1, mcb_size)) != NULL) {
            qm_mcb->status.max_queues = max_queues;
            (void)time(&qm_mcb->status.created);
            if (remarks)
                qm_mcb->status.remarks =
                    strncpy ((char *) &(qm_mcb->qcb[max_queues]),
                                                         remarks,
                                                   QM_MAX_REMARK );
            initialized = TRUE;
        }
        else {
            /*
             * memory request failed, log an error.
             */
            MLOG((MMSG(501, "%s: calloc()[2] failed, errno=%d\n"), self, errno));
        }
    }
    
    return initialized;
}

#ifdef TEST


#ifdef NULL
#undef NULL
#endif

#include <stdio.h>


void     main              ();
BOOLEAN  test_1            ();
BOOLEAN  test_2            ();
BOOLEAN  test_3            ();
BOOLEAN  test_rem          ();
BOOLEAN  validate_mcb      ();
BOOLEAN  validate_remarks  ();


void 
main (
    int argc,
    char *argv[]
)
{
    BOOLEAN passed;
    int     i;
    int     j;

    passed = test_1() && test_2() && test_3();

    for (i=0;  passed && (i < sizeof(int)+2);  ++i)
        passed = passed && test_rem(i+1, i);

    for (i=QM_MAX_REMARK-sizeof(int)-2, j=100;
             passed && (i < QM_MAX_REMARK+sizeof(int)+2);  ++i, ++j)
        passed = passed && test_rem(j, i);

    exit (!passed);
}



/*
 * test_1:  check that cl_qm_init() recognizes when the queue manager is
 *          already active.
 */

BOOLEAN 
test_1 (void)
{

    BOOLEAN  successful  = FALSE;
    QM_MCB   *mcb;

    qm_mcb = (QM_MCB *) NULL;

    if (!cl_qm_init ((unsigned short)1, NULL))
        printf ("cl_qm_init() failed to initialize\n");
    else if (qm_mcb->status.max_queues != 1)
        printf ("wrong max_queues, exp=1, got=%d\n", qm_mcb->status.max_queues);
    else if ((mcb = qm_mcb) && (!cl_qm_init ((unsigned short)1, NULL)))
        printf ("cl_qm_init() failed to re_initialize\n");
    else if (qm_mcb->status.max_queues != 2)
        printf ("wrong max_queues, exp=2, got=%d\n", qm_mcb->status.max_queues);
    else if (qm_mcb == mcb)
        printf ("cl_qm_init() did not change the value of qm_mcb\n");
    else
        successful = TRUE;

    free(qm_mcb);
    qm_mcb = (QM_MCB *) NULL;

    return successful;
}



/*
 * test_2:  check that cl_qm_init() rejects an invocation where max_queues
 *          is set to zero.
 */

BOOLEAN 
test_2 (void)
{

    BOOLEAN  successful  = FALSE;

    qm_mcb = (QM_MCB *) NULL;

    if (cl_qm_init ((unsigned short)0, NULL))
        printf ("cl_qm_init() accepted max_queues == 0\n");
    else if (qm_mcb)
        printf ("cl_qm_init() changed the value of qm_mcb\n");
    else
        successful = TRUE;

    return successful;
}



/*
 * test_3:  check that cl_qm_init() accepts an invocation where the remarks
 *          pointer contains NULL.
 */

BOOLEAN 
test_3 (void)
{

    BOOLEAN  successful  = FALSE;

    qm_mcb = (QM_MCB *) NULL;

    if (!cl_qm_init ((unsigned short)1, NULL))
        printf ("cl_qm_init(1, NULL) failed\n");
    else if (!qm_mcb)  {
        printf ("cl_qm_init(1, NULL) returned TRUE ... ");
        printf ("qm_mcb contains NULL\n");
    }
    else if (qm_mcb->qcb[1])
        printf ("qm_mcb->qcb[1] should contain NULL\n");
    else if (validate_mcb(1, NULL))
        successful = TRUE;

    if (qm_mcb)  {
        free(qm_mcb);
        qm_mcb = (QM_MCB *) NULL;
    }

    return successful;
}



BOOLEAN 
test_rem (
    unsigned int max_queues,
    unsigned int remarks_length
)
{
    int      i;
    char     remarks [2*QM_MAX_REMARK];
    char     c;
    BOOLEAN  successful  = FALSE;

    qm_mcb = (QM_MCB *) NULL;

    if (remarks_length < 2*QM_MAX_REMARK)  {

        for (i=0, c='a'-1;  i < 2*QM_MAX_REMARK;
                            remarks[i++] = (c =  ++c > 'z' ? 'a' : c) )
            ;
        remarks[remarks_length] = '\0';

        successful = cl_qm_init((unsigned short)max_queues, remarks)   &&
                     validate_mcb(max_queues, remarks);
    }

    if (qm_mcb)  {
        free(qm_mcb);
        qm_mcb = (QM_MCB *) NULL;
        }

    return successful;
}



BOOLEAN 
validate_mcb (
    unsigned short max_queues,
    char *remarks
)
{
    int      i;
    BOOLEAN  successful  = FALSE;

    if (qm_mcb)  {

        if (qm_mcb->status.max_queues != max_queues)  {
            printf ("qm_mcb->status.max_queues disagrees with its ");
            printf ("anticipated value ... qm_mcb->status.max_queues=%u, ",
                    qm_mcb->status.max_queues);
            printf ("max_queues=%u)\n", max_queues);
        }
        else if (qm_mcb->status.queues)  {
            printf ("qm_mcb->status.queues is non-zero ... ");
            printf ("qm_mcb->status.queues=%d)\n", qm_mcb->status.queues);
        }
        else if ( !qm_mcb->status.created )
            printf ("qm_mcb->status.created is NULL\n");
        else if ( qm_mcb->status.modified )
            printf ("qm_mcb->status.modified is non-NULL\n");
        else if ( qm_mcb->status.audited )
            printf ("qm_mcb->status.audited is non-NULL\n");

        else  {
            for (i=0;  i < max_queues;  ++i)  {
                if (qm_mcb->qcb[i])  {
                    printf ("qm_mcb->qcb[%d] is non-NULL\n", i);
                    break;
                }
             }
             if (i >= max_queues)
                successful = validate_remarks(max_queues, remarks);
         }
    }

    return successful;
}



BOOLEAN 
validate_remarks (
    unsigned short max_queues,
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

    for (i = 0,  p = (char *) &(qm_mcb->qcb[max_queues]);
         i < expected_chars;  ++i, ++p)
        if (!(*p))  {
            printf ("unexpected null character at position %d of ", i);
            printf ("qm_mcb->remarks\n");
            break;
        }

    if (i >= expected_chars)  {
        for (i=0;  i < expected_nulls;  ++i, ++p)  {
            if (*p)  {
                printf ("unexpected non-null character 0x%X at position %d ",
                       (int) *p, expected_chars+i);
                printf ("of qm_mcb->remarks\n");
                break;
            }
        }
        successful = (i >= expected_nulls);
    }

    return successful;
}
#endif
