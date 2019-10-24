static char SccsId[] = "@(#)csi_qclean.c	5.6 3/21/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_qclean()
 *
 * Description:
 *
 *      Function which removes entries from a connection queue table that
 *      have aged beyond a certain point (defined by a startup envrionment
 *      variable called CSI_CONNECT_AGETIME.  Note:  will clean out entire
 *      queue if the ageing time passed in is equal to zero.
 *
 *      Calls time() which returns the current time in seconds past 1970.  
 *      The time is used for comparison purposes when the aging is performed.
 *
 *      Calls cl_qm_mlocate() with parameters which are the id of the queue to 
 *      be cleaned, QM_POS_FIRST to access the first member, and a member id 
 *      variable which is ignored since we are getting the first member.
 *      Cl_qm_mlocate() returns a member id.
 *
 *      In a loop, until the entire queue has been checked:
 *
 *      BEGIN_LOOP:
 *
 *      If the member id == 0, then breaks out of loop, we are either done or
 *      queue is empty.
 *
 *      Calls cl_qm_mlocate() with parameters which are the id of the queue to 
 *      be cleaned, QM_POS_NEXT for the next member in the queue and the member
 *      id of the next member.   See note under "Considerations" about why the
 *      next member-id is retrieved before deleting a particular member.
 *      Cl_qm_mlocate() returns a member id.
 *
 *      Calls cl_qm_mstatus() with the member id returned by csl_qm_mlocate()
 *      to retrieve a status structure (type QM_MSTATUS) containing the
 *      queue member's creation date.
 *
 *              If NULL == cl_qm_mstatus(), calls csi_logevent() 
 *              with STATUS_QUEUE_FAILURE and message MSG_QUEUE_STATUS_FAILURE
 *              concatenated with the queue-id and memember-id being accessed.
 *              Returns STATUS_QUEUE_FAILURE to the caller.
 *
 *      If the "created" date >= the aging time passed in:
 *
 *              Calls csi_free_qmemb() with the queue-id, member-id, and
 *              logging function to log the deletion of a queue member
 *              to the event log.  The member will be dropped from the queue, 
 *              and associated memory freed.
 *
 *      Calls free() with a pointer to the queue memeber status structure 
 *      returned by cl_qm_mstatus() to free up the memory allocated for that 
 *      structure.
 *
 *      Sets the current member id to the value of the next member id.
 *
 *      END_LOOP:
 *
 *      Returns STATUS_SUCCESS
 *
 * Return Values:
 *      STATUS_SUCCESS          - queue cleaning completed
 *      STATUS_QUEUE_FAILURE    - queue access failure
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *      Since according to the CL queue management routine
 *      design the state of current member pointer is undefined
 *      after a deletion, and since we always want the next
 *      queue member, we must save the id of the next member
 *      before deleting a current member...otherwise we would
 *      be unable to find the next member.
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       01-Jan-1989.    Created.
 *      J. A. Wishner       20-Oct-1991.    Fix wrong# of args cl_qm_mlocate().
 *      E. A. Alongi        06-Aug-1992.    R3.0.1 merge - declaration correct-
 *                                          tions, reset errno and print if
 *                                          error detected after function call.
 *      E. A. Alongi        15-Nov-1993.    Flint detected errors corrected.
 */


/*      Header Files: */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static  char *st_module = "csi_qclean()";

/*      Procedure Type Declarations: */

STATUS 
csi_qclean (
    QM_QID q_id,            /* id of queue to be cleaned */
    unsigned long agetime,         /* time to age a connection */
    CSI_VOIDFUNC log_func        /* ptr to logging function to use */
)
{
    time_t       curtime;       /* current time of day */
    QM_MSTATUS  *qstat;         /* holds queue status pointer */
    QM_MID       m_id;          /* id of member to be cleaned */
    QM_MID       next_m_id;     /* id of next member to be cleaned */


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, 3, (unsigned long) q_id, (unsigned long) agetime,
                 (unsigned long) log_func);
#endif /* DEBUG */

    errno = 0;
    curtime = time(0);   /* use current time in aging queue entries */

    /* loop thru each of the queue's entries */
    for (m_id = cl_qm_mlocate(q_id,QM_POS_FIRST, (QM_MID) 0);
                                               0 != m_id; m_id = next_m_id) {
        next_m_id = cl_qm_mlocate(q_id, QM_POS_NEXT, (QM_MID) 0);
        if ((qstat = cl_qm_mstatus(q_id, m_id)) == (QM_MSTATUS *) NULL) {
            MLOGCSI((STATUS_QUEUE_FAILURE,  st_module, "cl_qm_mstatus()", 
	      MMSG(948, "Can't get queue status Errno:%d, Q-id:%d, Member:%d"),
	      errno, q_id, m_id));
            return(STATUS_QUEUE_FAILURE);
        }

        if ((curtime - qstat->created) >= agetime) {

#ifdef DEBUG
           MLOGDEBUG(0, (MMSG(543,
	   "%s: %s: status:%s; failed: %s\n%s agetime = %ld lapse time = %ld"), 
	        CSI_LOG_NAME, st_module, cl_status(STATUS_PROCESS_FAILURE),
		st_module, st_module, agetime, curtime-qstat->created));
#endif

            /* delete queue entry */
            if (csi_freeqmem(q_id, m_id, log_func) != STATUS_SUCCESS) {
                    MLOGCSI((STATUS_QUEUE_FAILURE, st_module, "csi_freeqmem()",
		      MMSG(948,
		        "Can't get queue status Errno:%d, Q-id:%d, Member:%d"),
		        errno, q_id, m_id));
                    return(STATUS_QUEUE_FAILURE);
            }

            /* log the removal of the queue entry */
            MLOGCSI((STATUS_SUCCESS,  st_module,  CSI_NO_CALLEE, 
	      MMSG(949, "Queue cleanup Q-id:%d.   Member:%d removed."),
		q_id, m_id));

            /* re-find next, realign internal queue ptrs after delete */
            next_m_id = cl_qm_mlocate(q_id, QM_POS_MEMBER, next_m_id);
        }
        (void) free(qstat); /* free the space queue manager allocated for
			     * mstatus */
    }
    return(STATUS_SUCCESS);
}
