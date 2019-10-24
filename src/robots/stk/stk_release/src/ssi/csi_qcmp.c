static char SccsId[] = "@(#)csi_qcmp.c	5.5 11/15/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_qcmp()
 *
 * Description:
 *
 *      Function which compares the contents of a buffer passed in with 
 *      the members of a designated queue over the length of both buffers
 *      equal to a size passed in.  If the contents of the two match,
 *      then 0 is returned, if they don't match 1 is returned, and if there
 *      is an error accessing the queue, a message is logged and -1 returned.
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
 *      Requires that a valid data area can be accessed (program bug trap).
 *
 *              If NULL == pointer-to-data, then logs an error message and
 *              returns -1.
 *
 *      Sets the current member id to the value of the next member id.
 *
 *      END_LOOP:
 *
 *      Returns 1 if the passed in data area doesn't match any on the queue.
 *
 * Return Values:
 *
 *      -1              - error
 *       1              - not a duplicate
 *       0              - is a duplicate
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
 *      NONE            Since according to the CL queue management routine
 *                      design the state of current member pointer is undefined
 *                      after a deletion, and since we always want the next
 *                      queue member, we must save the id of the next member
 *                      before deleting a current member...otherwise we would
 *                      be unable to find the next member.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner   01-Jan-1989.    Created.
 *      H. I. Grapek    31-Aug-1991.    Fixed lint errors
 *      E. A. Alongi    30-Oct-1992.    Replace bcmp with memcmp.
 *      E. A. Alongi    15-Nov-1993.    Made flint error free.
 *      
 *     
 */


/*
 *      Header Files:
 */
#include <string.h>
#include <stdio.h>
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_qcmp()";


/*
 *      Procedure Type Declarations:
 */


int 
csi_qcmp (
    register QM_QID q_id,           /* id of queue to be cleaned */
    void *datap,          /* data to be compared */
    unsigned int size           /* size of data to be compared */
)
{

    register QM_QID      m_id;          /* id of member to be cleaned */
    register QM_QID      next_m_id;     /* id of next member to be cleaned */
             char       *q_datap;       /* ptr to data area of queue member */


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 3,                             /* parameter count */
                 (unsigned long) q_id,          /* argument list */
                 (unsigned long) datap,         /* argument list */
                 (unsigned long) size);         /* argument list */
#endif /* DEBUG */


    /* loop thru each of the queue's entries */
    for (m_id = cl_qm_mlocate(q_id, QM_POS_FIRST, (QM_MID) 0); 0 != m_id;
							    m_id = next_m_id){

        /* locate the next member on the queue */
        next_m_id = cl_qm_mlocate(q_id, QM_POS_NEXT, (QM_MID) 0);

        /* locate the data area */
        q_datap = (char*) cl_qm_maccess(q_id, m_id);
 
        /* error if no data but queue has member */
        if ( (char*) NULL == q_datap ) {

            /* concatenate the error message with the queue id and member id */
            MLOGCSI((STATUS_QUEUE_FAILURE,  st_module, "cl_qm_maccess()", 
	      MMSG(950,"Can't locate queue Q-id:%d, Member:%d"), q_id, m_id));
            return(-1);
        }

        /* if the items are the same, return */
        if (memcmp(datap, q_datap, size) == 0)
            return(0);
    }

    return(1);

}
