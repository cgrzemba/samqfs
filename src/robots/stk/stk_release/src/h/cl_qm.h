#ifndef  _CL_QM_
#define  _CL_QM_
/*
 * Copyright (C) 1988,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 * Functional Description:
 *
 *    
 *    Defines all common library queue manager (QM) internal data objects.
 *
 *
 * Modified by:
 *
 *	D.E. Skinner		4-Oct-1988	Original.
 *	D. F. Reed		13-Jan-1989	Added MAX macro (fixed).
 *      Jim Montgomery          02-Aug-1989     Use ALIGNED_BYTES instead of
 *                                              BYTE.
 *      E. A. Alongi            03-Dec-1992     Add pre-processor directive
 *                                              to conditionally define MAX.
 *      Mike Williams           04-May-2010     For 64-bit compile, data, guard,
 *                                              and remarks are now an array
 *                                              lenght of 1.
 */


/*
 * Header Files:
 */

#ifndef  _CL_QM_DEFS_
#include "cl_qm_defs.h"
#endif


/*
 * Defines, Typedefs and Structure Definitions:
 */

#ifndef  MAX
#define  MAX(a,b)  ((a)>(b)?(a):(b))
#endif /* MAX */

#ifndef  USHRT_MAX
#define  USHRT_MAX  65535          /* Maximum value of an unsigned short.   */
#endif

#define  QM_MAX_REMARK  128        /* Maximum number of characters          */
                                   /*   permitted in a remarks string,      */
                                   /*   INCLUDING the terminating null.     */

typedef  struct member  {          /*--------- QUEUE MEMBER FORMAT ---------*/
    struct member  *prev;          /* Ptr to previous member in queue.      */
                                   /*   NULL if this member is the head.    */
    struct member  *next;          /* Ptr to next member in queue.  NULL if */
                                   /*   this member is the tail.            */
    QM_MSTATUS      status;        /* Status and control information.       */
    ALIGNED_BYTES   data[1];       /* Storage for data.                     */
    ALIGNED_BYTES   guard[1];      /* Null bytes to prevent string over_runs*/
    } QM_MEMBER;

typedef  struct  {                 /*------ QUEUE CONTROL BLOCK FORMAT -----*/
    QM_MEMBER      *first;         /* Ptr to first member in queue.  NULL   */
                                   /*   if queue is empty.                  */
    QM_MEMBER      *last;          /* Ptr to last member in queue.  NULL if */
                                   /*   queue is empty.                     */
    QM_MID          lowest;        /* Lowest queue member ID presently in   */
                                   /*   use.                                */
    QM_MID          highest;       /* Highest queue member ID presently in  */
                                   /*   use.                                */
    QM_QSTATUS      status;        /* Control and status information.       */
    } QM_QCB;

typedef  struct  {                 /*----- MASTER CONTROL BLOCK FORMAT -----*/
    QM_STATUS       status;        /* Control and status information.       */
    QM_QCB         *(qcb [1]);     /* Array of pointers to queue control    */
                                   /*   blocks (QCBs).  The number of array */
                                   /*   elements is defined in              */
                                   /*   status.max_members.  An element     */
                                   /*   containing NULL indicates there is  */
                                   /*   no associated QCB.                  */
    ALIGNED_BYTES   remarks[1];    /* Null-terminated string describing     */
                                   /*   this session of the QM.  Always a   */
                                   /*   multiple of sizeof(int) characters. */
    } QM_MCB;

extern QM_MCB      *qm_mcb;        /* Ptr to master control block (MCB).    */
                                   /*   If NULL, QM has not been            */
                                   /*   initialized.                        */

/*
 * Procedure Type Declarations:
 */

QM_MEMBER *cl_qm_create(QM_QCB *qcb, BOOLEAN before,
			QM_MEMBER *member, unsigned short size);
QM_MEMBER *cl_qm_find(QM_QCB *qcb, QM_POS position, QM_MID member);


#endif

