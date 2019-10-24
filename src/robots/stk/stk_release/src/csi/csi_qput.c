static char SccsId[] = "@(#)csi_qput.c	5.4 11/15/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_qput()
 *
 * Description:
 *
 *      Function places queue data passed onto the designated queue.
 *
 *      Calls cl_qm_mcreate() to create a node on the queue where the new
 *      member will be added.  Passes queue id for queue that will receive
 *      the new member.
 *
 *              If 0 == cl_qm_mcreate(), the member couldn't be added.  Calls
 *              csi_logevent() with STATUS_QUEUE_FAILURE and message
 *              MSG_QUEUE_MEMBADD_FAILURE concatenated with the queue id.
 *
 *      Calls cl_qm_access() to get a pointer to the data area on the queue.
 *      
 *              If NULL == cl_qm_access(), (error) calls csi_logevent() with
 *              STATUS_QUEUE_FAILURE and message MSG_QUEUE_MEMBADD_FAILURE
 *
 *      Calls bcopy to copy the csi_header structure to the queue member's 
 *      data area.
 *
 *      If the caller passed in a pointer to a member id, then return him the
 *      member id.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS          - put return address on the connection queue
 *      STATUS_QUEUE_FAILURE    - failure accessing or adding to the connection
 *                                queue
 *      STATUS_PROCESS_FAILURE  - memory could not be allocated for queue member
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
 *      J. A. Wishner       30-May-1989.    Set member id returned only if 
 *                                          member id pointer passed in is not
 *                                          null. If it is null, the caller
 *                                          doesn't want it.
 *      E. A. Alongi        30-Oct-1992.    Replaced bcopy with memcpy.
 *      E. A. Alongi        15-Nov-1993.    Cleaned up flint warnings.
 *      
 *     
 */


/*
 *      Header Files:
 */
#include <string.h>
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_qput()";


/*
 *      Procedure Type Declarations:
 */

STATUS 
csi_qput (
    QM_QID q_id,                  /* id of queue data will be put on */
    void *q_datap,               /* data object/structure to put on queue */
    int size,                  /* size of data object to put on queue */
    QM_MID *m_id                  /* member id to be returned */
)
{

    char        *dp;            /* pointer to data area on the queue */
    QM_MID       member;        /* member id */


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 4,                             /* parameter count */
                 (unsigned long) q_id,          /* argument list */
                 (unsigned long) q_datap,       /* argument list */
                 (unsigned long) size,          /* argument list */
                 (unsigned long) m_id);         /* argument list */
#endif /* DEBUG */


    /* create a queue member node */
    if (0 == (member = cl_qm_mcreate(q_id, QM_POS_LAST, 0, size))) {
        MLOGCSI((STATUS_QUEUE_FAILURE,  st_module, "cl_qm_mcreate()", 
	  MMSG(952, "Can't add member to queue Q-id:%d"), q_id));
        return(STATUS_QUEUE_FAILURE);
    }

    /* find the data area of this member */
    if (NULL == (dp = (char *) (cl_qm_maccess(q_id, member)))) {
        MLOGCSI((STATUS_QUEUE_FAILURE,  st_module, "cl_qm_maccess()", 
	  MMSG(952, "Can't add member to queue Q-id:%d"), q_id));
        return(STATUS_QUEUE_FAILURE);
    }

    /* copy the queue data to the data area */
    memcpy(dp, q_datap, size);

    /* set return member id if caller wants it */
    if (m_id != (QM_MID *)NULL)
        *m_id = member;

    return(STATUS_SUCCESS);

}
