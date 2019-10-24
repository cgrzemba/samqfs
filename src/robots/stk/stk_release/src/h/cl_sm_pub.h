/* SccsId @(#)cl_sm_pub.h	1.2 1/11/94  */
#ifndef _CL_SM_PUB_
#define _CL_SM_PUB_
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Common definitions for modules comprising the common library state
 *      machine.
 *
 * Modified by:
 *
 *      D. A. Beidle            20-Mar-1991     Original.
 *
 *      D. A. Beidle            17-Oct-1991.    Created MAX_TIMEOUT definition
 *          which limits timeout periods to 10^8 seconds.  This is the maximum
 *          timeout period allowed by select().
 *
 *      D. A. Beidle            26-Oct-1991.    Altered event and state enum
 *          lists for eject and enter processing.
 *
 *      D. A. Beidle            12-Nov-1991.    Altered event and state enum
 *          lists for acscm processing.
 *
 *      D. A. Beidle            14-Nov-1991.    IBR#249 - Added fields to task
 *          queue entry to save previous event and timeout values upon task
 *          activation, changed CL_SM_ACTIVATE macro to save prior timeout and
 *          event, added CL_SM_CONTINUE macro to resume task suspension for
 *          remainder of prior timeout period.
 *
 *      J. S. Alexander         19-Feb-1992.    IBR#416 - Added WAIT_ZERO
 *          #define to imediately retry the task with only enough wait built
 *          into the state machine.
 */

/*
 *      Header Files:
 */

#include <limits.h>                     /* Needed for LONG_MAX */
#include <time.h>                       /* Needed for time()   */

#ifndef _CL_QM_DEFS_
#include "cl_qm_defs.h"
#endif

#ifndef _LH_DEFS_
#include "lh_defs.h"
#endif

/*
 *      Defines, Typedefs and Structure Definitions:
 */
#define MAX_TIMEOUT     100000000L      /* max select() timeout (10^8 secs) */
#define NOW             time((time_t *)0)
#define WAIT_ACTIVITY   10              /* wait for activity to complete    */
#define WAIT_FOREVER    LONG_MAX        /* wait for a long time             */
#define WAIT_RETRY      RETRY_TIMEOUT   /* wait a bit before retrying task  */
#define WAIT_ZERO       0               /* don't wait before retrying task  */


#define CL_SM_ACTIVATE(tqep, nevt) do {     /* activate another task    */  \
    tqep->prev_event = tqep->event;         /*   save previous event    */  \
    tqep->prev_timeout = tqep->timeout;     /*   save previous timeout  */  \
    tqep->event = nevt;                     /*   set new event          */  \
    tqep->timeout = NOW;                    /*   set task to expire next*/  \
} while (0)

#define CL_SM_CONTINUE() do {               /* resume task suspension   */  \
    cl_tqep->event = cl_tqep->prev_event;   /*   restore timeout event  */  \
    cl_tqep->timeout = cl_tqep->prev_timeout;/*  restore timeout period */  \
    if (1) return SME_SUSPEND;                                              \
} while (0) /*lint -unreachable */

#define CL_SM_EXIT()  do {                  /* abort state machine      */  \
    if (1) return SME_EXIT;                                                 \
} while (0) /*lint -unreachable */

#define CL_SM_SUSPEND(period, tevt)  do {   /* suspend current task     */  \
    cl_tqep->event = tevt;                  /*   set timeout event      */  \
    cl_tqep->timeout = (time_t)(period);    /*   set timeout period     */  \
    cl_tqep->timeout += (cl_tqep->timeout == WAIT_FOREVER) ? 0 : NOW;       \
    if (1) return SME_SUSPEND;                                              \
} while (0) /*lint -unreachable */

#define CL_SM_SWITCH(ntbl, nstt, nevt) do { /* switch state tables      */  \
    cl_tqep->table = ntbl;                  /*   set new state table    */  \
    cl_tqep->state = nstt;                  /*   set new state          */  \
    cl_tqep->event = nevt;                  /*   set new event          */  \
    cl_tqep->timeout = NOW;                                                 \
    if (1) return SME_SWITCH;                                               \
} while (0) /*lint -unreachable */

#define CL_SM_TERMINATE() do {              /* terminate current task   */  \
    if (1) return SME_TERMINATE;                                            \
} while (0) /*lint -unreachable */


typedef enum {                          /* state machine event codes        */
/*  v                v,                    limit 18 chars for readability   */
    SME_FIRST = 0     ,                 /* illegal */
    SME_CANCELLED     ,                 /* cancel rcv'd while task active   */
    SME_CAN_REQUESTED ,                 /* cancel rcv'd while task inactive */
    SME_CAP_BUSY      ,                 /* CAP in use by other host|process */
    SME_CAP_CLOSED    ,                 /* CAP closed while task suspended  */
                            /*  5 */
    SME_CAP_EMPTY     ,                 /* CAP contains no cartridges       */
    SME_CAP_ERROR     ,                 /* error affecting CAP only occured */
    SME_CAP_FULL      ,                 /* CAP has no available empty cells */
    SME_CAP_OPEN      ,                 /* LH CAP request failed, door open */
    SME_CELL_EMPTY    ,                 /* found cell unexpectedly empty    */
                            /* 10 */
    SME_CELL_FULL     ,                 /* found cell unexpectedly full     */
    SME_CLEANUP       ,                 /* time to clean up queue contents  */
    SME_COMPLETE      ,                 /* action completed successfully    */
    SME_CONFIG_ERROR  ,                 /* device not configured in library */
    SME_EJECT_COMPLETE,                 /* eject processing complete        */
                            /* 15 */
    SME_ENTER_COMPLETE,                 /* enter processing complete        */
    SME_EXIT          ,                 /* special - abort state machine    */
    SME_INTERMEDIATE  ,                 /* intermediate response received   */
    SME_LIBRARY_BUSY  ,                 /* LH too busy to handle request    */
    SME_LIBRARY_FAIL  ,                 /* library hardware failed          */
                            /* 20 */
    SME_LSM_ERROR     ,                 /* error affecting LSM only occured */
    SME_PROCESS_FAIL  ,                 /* LH protocol error detected       */
    SME_RECOV_COMPLETE,                 /* recovery complete on device      */
    SME_REQUEST_ERROR ,                 /* error affecting request occured  */
    SME_REQ_OVERRIDDEN,                 /* LM request overridden by another */
                            /* 25 */
    SME_SERVER_IDLE   ,                 /* CAP opened while server idled    */
    SME_START         ,                 /* task started                     */
    SME_SUSPEND       ,                 /* special - suspend task           */
    SME_SWITCH        ,                 /* special - switch state tables    */
    SME_TERMINATE     ,                 /* special - delete task queue entry*/
                            /* 30 */
    SME_TIMED_OUT     ,                 /* timeout period expired           */
    SME_TIMED_OUT2    ,                 /* timeout period expired in SCL    */
    SME_VOLUME_ERROR  ,                 /* error affecting volume occured   */
    SME_VOL_MISPLACED ,                 /* label didn't match expected VSN  */
    SME_WAKEUP        ,                 /* reactivated by parent task       */
                            /* 35 */
    SME_LAST                            /* illegal */
} CL_SM_EVENT;


typedef enum {                          /* state machine state codes        */
/*  v                v,                    limit 18 chars for readability   */
    SMS_FIRST = 0     ,                 /* illegal - table name specifier   */
    SMS_CANCEL_RETRY  ,                 /* retry sending LH cancel request  */
    SMS_CANCEL_WAIT   ,                 /* wait for LH cancel response      */
    SMS_CAP_WAIT1     ,                 /* wait for CAP activity completion */
    SMS_CAP_WAIT2     ,                 /* wait for CAP activity completion */
                            /*  5 */
    SMS_CAT_CELL1     ,                 /* issue LH catalog cell request    */
    SMS_CAT_CELL2     ,                 /* issue LH catalog cell request    */
    SMS_CAT_DRIVE     ,                 /* issue LH catalog drive request   */
    SMS_CAT_RETRY1    ,                 /* retry sending LH catalog request */
    SMS_CAT_RETRY2    ,                 /* retry sending LH catalog request */
                            /* 10 */
    SMS_CAT_RETRY3    ,                 /* retry sending LH catalog request */
    SMS_CAT_RETRY4    ,                 /* retry sending LH catalog request */
    SMS_CAT_WAIT1     ,                 /* wait for LH catalog response     */
    SMS_CAT_WAIT2     ,                 /* wait for LH catalog response     */
    SMS_CAT_WAIT3     ,                 /* wait for LH catalog response     */
                            /* 15 */
    SMS_CAT_WAIT4     ,                 /* wait for LH catalog response     */
    SMS_EJECT_COMPLETE,                 /* complete eject processing        */
    SMS_EJECT_RETRY   ,                 /* retry sending LH eject request   */
    SMS_EJECT_WAIT1   ,                 /* wait for LH unlock response      */
    SMS_EJECT_WAIT2   ,                 /* wait for LH CAP open response    */
                            /* 20 */
    SMS_EJECT_WAIT3   ,                 /* wait for LH CAP closed response  */
    SMS_END           ,                 /* final task state                 */
    SMS_ENTER_COMPLETE,                 /* complete enter processing        */
    SMS_ENTER_RETRY   ,                 /* retry sending LH enter request   */
    SMS_ENTER_WAIT1   ,                 /* wait for LH unlock response      */
                            /* 25 */
    SMS_ENTER_WAIT2   ,                 /* wait for LH CAP open response    */
    SMS_ENTER_WAIT3   ,                 /* wait for LH CAP closed response  */
    SMS_MOVE_RETRY    ,                 /* retry sending LH move request    */
    SMS_MOVE_VOLUME   ,                 /* issue LH move request            */
    SMS_MOVE_WAIT     ,                 /* wait for LH move response        */
                            /* 30 */
    SMS_RELEASE_RETRY1,                 /* retry sending LH release request */
    SMS_RELEASE_WAIT1 ,                 /* wait for LH release response     */
    SMS_RESERVE_RETRY ,                 /* retry sending LH reserve request */
    SMS_RESERVE_WAIT1 ,                 /* wait for resource availability   */
    SMS_RESERVE_WAIT2 ,                 /* wait for LH reserve response     */
                            /* 35 */
    SMS_RESERVE_WAIT3 ,                 /* wait for reservation of all CAPs */
    SMS_START         ,                 /* initial task state               */
    SMS_STATUS_RETRY1 ,                 /* retry sending LH status request  */
    SMS_STATUS_RETRY2 ,                 /* retry sending LH status request  */
    SMS_STATUS_WAIT1  ,                 /* wait for LH status response      */
                            /* 40 */
    SMS_STATUS_WAIT2  ,                 /* wait for LH status response      */
    SMS_UNLOCK_RETRY  ,                 /* retry sending LH unlock request  */
    SMS_UNLOCK_WAIT   ,                 /* wait for LH unlock response      */
    SMS_VARY_RETRY    ,                 /* retry sending LH vary request    */
    SMS_VARY_WAIT     ,                 /* wait for LH vary response        */
                            /* 45 */
    SMS_WAIT          ,                 /* wait for event                   */
    SMS_LAST                            /* illegal - state table terminator */
} CL_SM_STATE;


typedef CL_SM_EVENT (*CL_SM_ARP)(void *);      /* action routine pointer  */
typedef STATUS      (*CL_SM_MHP)(char *, int); /* message handler pointer */

/*
 * The following structure comprises one element of a message handler table.
 */
typedef struct {                        /* message handler table definition */
    TYPE            sender;             /*   module type of message sender  */
    CL_SM_MHP       handler;            /*   pointer to message handler     */
} CL_SM_HANDLER;

/*
 *  The following structure comprises one element of a state table.  A state 
 *  table is an array of these structures, bounded by entries containing
 *  SMS_FIRST and SMS_LAST in the state field and SME_FIRST and SME_LAST in
 *  the event field.
 */
typedef struct {                        /* state table definition           */
    CL_SM_STATE     state;              /*   current state                  */
    CL_SM_EVENT     event;              /*   current event (stimulus)       */
    CL_SM_ARP       action;             /*   pointer to action routine      */
    CL_SM_STATE     nxt_state;          /*   next state on action rtn exit  */
} CL_SM_TABLE;

/*
 *  The following structure comprises a task queue entry upon which the state
 *  machine acts.  The state machine scans a state table (defined above) for a
 *  state and event that matches the state and event in the queue entry.  When
 *  a match is found, the corresponding action routine in the state table is
 *  invoked.
 *
 *  When a task is suspended, timeout is set to the time when the task is 
 *  automatically activated with the stimulus in event when no other event
 *  occurs within the timeout period.
 *
 *  When a task is activated, event and timeout are saved in prev_event and
 *  prev_timeout.  This permits an action routine to resume suspension with
 *  the remainder of the prior timeout period and the prior timeout event
 *  (typically used when events occur while waiting for ACSLH responses).
 */
typedef struct {                        /* task queue entry definition      */
    QM_MID          task_id;            /*   task queue member identifier   */
    CL_SM_TABLE    *table;              /*   pointer to state table         */
    CL_SM_STATE     state;              /*   current state                  */
    CL_SM_EVENT     event;              /*   current event (stimulus)       */
    time_t          timeout;            /*   time when task expires         */
    void           *taskp;              /*   pointer to task to reactivate  */
    RESPONSE_STATUS status;             /*   error status from msg handler  */
    CL_SM_EVENT     prev_event;         /*   event prior to activation      */
    time_t          prev_timeout;       /*   timeout prior to activation    */
} CL_SM_TASK;


/*
 *      Global and Static Variable Declarations:
 */

extern  LH_RESPONSE    *cl_lh_response; /* ptr to ACSLH response buffer     */
extern  CL_SM_TASK     *cl_tqep;        /* ptr to current task queue entry  */
extern  QM_QID          cl_tskq;        /* queue ID of task queue           */


/*
 *      Procedure Type Declarations:
 */

char 	*cl_sm_event(CL_SM_EVENT event);
STATUS 	cl_sm_execute(void);
STATUS 	cl_sm_response(char *mbuf, int bcnt);
char 	*cl_sm_state(CL_SM_STATE state);
char 	*cl_sm_table(CL_SM_TABLE *table);
STATUS 	cl_sm_tcreate(CL_SM_TABLE *table, void *taskp, CL_SM_TASK **tqe);
STATUS 	cl_sm_tselect(CL_SM_HANDLER *mhtp);


#endif /* _CL_SM_PUB_ */

