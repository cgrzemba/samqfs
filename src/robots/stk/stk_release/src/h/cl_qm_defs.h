#ifndef  _CL_QM_DEFS_
#define  _CL_QM_DEFS_
/*
 * Copyright (C) 1988,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 * Functional Description:
 *
 *    Definitions of all common library queue manager (QM) objects and
 *    functions visible to a client application.
 *
 *
 * Modified by:
 *
 *      D.E. Skinner             4-Oct-1988.    Original.
 *      Jim Montgomery           2-Aug-1989.    Use ALIGNED_BYTES instead of
 *                          BYTE.
 *      D. A. Beidle            13-Oct-1991.    Correct cl_qm_term declaration.
 *      Mike Williams           04-May-2010     For 64-bit compiles, remarks is
 *                                              now an array lenght of 1.
 *
 */


/*
 * Header Files:
 */

#ifndef _DEFS_
#include "defs.h"
#endif


/*
 * Defines, Typedefs and Structure Definitions:
 */

typedef  unsigned char   BYTE;     /*                           {0x00:0xFF} */
typedef  unsigned short  QM_QID;   /* Queue ID                {1:USHRT_MAX} */
typedef  unsigned short  QM_MID;   /* Member ID               {1:USHRT_MAX} */


typedef  enum {
    QM_POS_A = 0 ,                 /* First constant in list                */
    QM_POS_FIRST ,                 /* First member in the queue             */
    QM_POS_LAST  ,                 /* Last member in the queue              */
    QM_POS_PREV  ,                 /* Before the last-accessed member       */
    QM_POS_NEXT  ,                 /* After the last-accessed member        */
    QM_POS_BEFORE,                 /* Before the specified member           */
    QM_POS_AFTER ,                 /* After the specified member            */
    QM_POS_MEMBER,                 /* At the specified member               */
    QM_POS_Z                       /* Last constant in list                 */
} QM_POS;


typedef  struct qm_mstatus {       /*-- QUEUE MEMBER STATUS & CONTROL INFO -*/
    QM_MID          mid;           /* Identifier of this member.            */
                                   /*   {0:USHRT_MAX}                       */
    unsigned short  size;          /* Size of this member's data area (in   */
                                   /*   bytes).  {sizeof(int):USHRT_MAX}    */
    time_t          created;       /* Date/time member was created.         */
    time_t          modified;      /* Date/time of last change to member.   */
                                   /*   Elements contain NULL if not yet    */
                                   /*   modified.                           */
} QM_MSTATUS;


typedef  struct qm_qstatus {       /*----- QUEUE STATUS & CONTROL INFO -----*/
    QM_QID          qid;           /* Identity of this queue.               */
    unsigned short  max_members;   /* Maximum number of members permitted   */
                                   /*   in this queue.  If zero, no limit   */
                                   /*   exists.  {0:USHRT_MAX}              */
    unsigned short  members;       /* Number of members currently defined.  */
                                   /*   {0:USHRT_MAX}                       */
    QM_MID          last;          /* ID of the member in this queue which  */
                                   /*   was last referenced by a QM         */
                                   /*   function.  {0:USHRT_MAX}            */
    time_t          created;       /* Date/time queue was created.          */
    time_t          modified;      /* Date/time of last change to QCB       */
                                   /*   (i.e., member created or deleted).  */
                                   /*   Elements contain NULL if not yet    */
                                   /*   modified.                           */
    time_t          audited;       /* Date/time of last audit.  Elements    */
                                   /*   contain NULL if audit not yet       */
                                   /*   performed.                          */
    ALIGNED_BYTES   remarks[1];    /* Null-terminated string describing     */
                                   /*   this queue.  Always a multiple of   */
                                   /*   sizeof(int) chars.                  */
} QM_QSTATUS;


typedef  struct qm_status  {       /*------ QM STATUS & CONTROL INFO ------*/
    unsigned short  max_queues;    /* Maximum number of queues permitted.  */
                                   /*   {1:USHRT_MAX}                      */
    unsigned short  queues;        /* Number of queues currently defined.  */
                                   /*   {0:USHRT_MAX}                      */
    time_t          created;       /* Date/time queue manager was          */
                                   /*   initialized.                       */
    time_t          modified;      /* Date/time of last change to MCB      */
                                   /*   (i.e., created or deleted).        */
                                   /*   Elements contain NULL if not yet   */
                                   /*   modified.                          */
    time_t          audited;       /* Date/time of last audit.  Elements   */
                                   /*   contain NULL if audit not yet      */
                                   /*   performed.                         */
    char           *remarks;       /* Ptr to a string describing this QM   */
                                   /*   session.  If NULL, no remarks are  */
                                   /*   defined.                           */
} QM_STATUS;


/*
 * Procedure Type Declarations:
 */

int 		cl_qm_audit(void);
BOOLEAN 	cl_qm_init(unsigned short max_queues, char *remarks);
ALIGNED_BYTES 	cl_qm_maccess(QM_QID queue, QM_MID member);
QM_MID 		cl_qm_mcreate(QM_QID queue, QM_POS position, QM_MID member,
			      unsigned short size);
BOOLEAN 	cl_qm_mdelete(QM_QID queue, QM_MID member);
QM_MID 		cl_qm_mlocate(QM_QID queue, QM_POS position, QM_MID member);
QM_MSTATUS	*cl_qm_mstatus(QM_QID queue, QM_MID member);
int 		cl_qm_qaudit(QM_QID queue);
QM_QID 		cl_qm_qcreate(unsigned short max_members, char *remarks);
BOOLEAN 	cl_qm_qdelete(QM_QID queue);
QM_QSTATUS 	*cl_qm_qstatus(QM_QID queue);
QM_STATUS 	*cl_qm_status(void);
BOOLEAN 	cl_qm_term(void);


#endif

