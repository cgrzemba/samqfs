static char SccsId[] = "@(#)csi_qget.c	5.6 3/21/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_qget()
 *
 * Description:
 *
 *      Function retrieves an object from a CL_QM type of queue and returns a
 *      pointer to it.  Accesses the queue using the member_id passed in.
 *
 *      Calls cl_qm_mstatus() to get the size of the queue member.
 *
 *              If NULL == cl_qm_mstatus(), calls csi_logevent() with
 *              MSG_QUEUE_STATUS_FAILURE, and returns STATUS_QUEUE_FAILURE
 *              to the caller.
 *
 *      Calls cl_qm_access() with the id number of the queue, using 
 *      member_id to locate the queue member.
 *
 *              If NULL == cl_qm_maccess(), calls csi_logevent() with
 *              STATUS_QUEUE_FAILURE with message MSG_LOCATE_QMEMBER_FAILURE.  
 *              Returns STATUS_QUEUE_FAILURE to the caller.
 *
 *      Assign the pointer to the data to the callers pointer.
 *
 *      Calls free() to release memory allocated by cl_qm_mstatus() for the
 *      queue status descriptor structure.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS          - Return address retrieved and returned.
 *      STATUS_QUEUE_FAILURE    - Return address couldn't be retrieved from 
 *                                queue.
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       12-Jan-1989.    Created.
 *      E. A. Alongi        06-Aug-1992.    R3.0.1 changes - reset errno and
 *                                          print if error after function call.
 *      E. A. Alongi        15-Nov-1993.    Made flint error free.
 *      
 *     
 */


/*
 *      Header Files:
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_qget()";


/*
 *      Procedure Type Declarations:
 */

STATUS 
csi_qget (
    QM_QID q_id,      /* ID of queue to be accessed */
    QM_MID m_id,      /* queue member id */
    void **q_datap    /* place where to put the address of the data */
)
{

     void        *dp;            /* pointer to data area on the queue */
     QM_MSTATUS  *qstat;         /* ptr allocation 4 Q status by cl_qm_mstatus*/


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 3,                             /* parameter count */
                 (unsigned long) q_id,          /* argument list */
                 (unsigned long) m_id,          /* argument list */
                 (unsigned long) q_datap);      /* argument list */
#endif /* DEBUG */

#ifdef DEBUG
    /* bug trap */
    if (NULL == q_datap) {
        MLOGDEBUG(0,
	  (MMSG(497, "%s: %s: status:%s; failed: %s\nbug trap"),
	  CSI_LOG_NAME, st_module, cl_status(STATUS_QUEUE_FAILURE), st_module));
        return(STATUS_QUEUE_FAILURE);
    }
#endif

    errno = 0;
    /* find out how large the data are of this member is */
    if ((qstat = cl_qm_mstatus(q_id, m_id)) == (QM_MSTATUS *) NULL) {
        MLOGCSI((STATUS_QUEUE_FAILURE,  st_module,  "cl_qm_mstatus()", 
	  MMSG(948, "Can't get queue status Errno:%d, Q-id:%d, Member:%d"),
	  errno, q_id, m_id));
        return(STATUS_QUEUE_FAILURE); 
    }

    /* find the data area of this member */
    if ((dp = (void *)(cl_qm_maccess(q_id, m_id))) == NULL) {
        MLOGCSI((STATUS_QUEUE_FAILURE,  st_module,  "cl_qm_maccess()", 
	  MMSG(950, "Can't locate queue Q-id:%d, Member:%d"), q_id, m_id));
        return(STATUS_QUEUE_FAILURE);
    }

    /* return the pointer to the data to the caller */
    *q_datap = dp;

    /* free memory allocated by cl_qm_mstatus */
    (void) free(qstat);

    return(STATUS_SUCCESS);
}
