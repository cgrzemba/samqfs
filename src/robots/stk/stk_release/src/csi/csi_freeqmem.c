static char SccsId[] = "@(#)csi_freeqmem.c	5.4 11/11/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_freeqmem()
 *
 * Description:
 *
 *      Function frees queue and data memory associated with a CSI connection
 *      queue member (which is the information on a particular client
 *      connection).  If the log_fmt_func != NULL, it will log this action in 
 *      the event log.
 *
 *      Calls cl_qm_maccess() to locate the data area belonging to the 
 *      member-id and queue passed in as parameters.
 *
 *              If NULL == cl_qm_maccess(), calls csi_logevent() with
 *              STATUS_QUEUE_FAILURE and message MSG_LOCATE_QMEMBER_FAILURE
 *              concatenated with the queue-id and member-id for which access
 *              was attempted.  Returns STATUS_QUEUE_FAILURE to the caller.
 *
 *      If NULL != log_function, then csi_freeqmem() will call the logging
 *      function, passing a pointer to the data area of the queue member.
 *      This function logs pertinent information being dropped from the queue.
 *      The logging function is of type (void).
 *
 *              Calls log_fmt_func() to format pertinent log queue information
 *              into a message for the event log.
 *
 *      Calls cl_qm_mdelete() to remove a connection entry from the queue and
 *      release the memory associated with its 's control structures.
 *
 *              If TRUE != cl_qm_mdelete(), calls csi_logevent with
 *              STATUS_QUEUE_FAILURE and message MSG_DELETE_QMEMBER_FAILURE
 *              concatenated with the queue-id and member-id for which deletion
 *              was attempted.
 *
 *      Returns STATUS_SUCCESS.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS          - Queue member was successfully deleted.
 *      STATUS_QUEUE_FAILURE    - An error occurred attempting to locate or
 *                                delete the spcified queue member.
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
 *      J. A. Wishner       05-Jan-1989.    Created.
 *      E. A. Alongi        05-Aug-1992     increased CVT_BUFSIZE as a result
 *                                          of Purify run.
 *      
 *     
 */


/*
 *      Header Files:
 */
#include <stdio.h>
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */
#define CVT_BUFSIZE 512

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_freeqmem()";

/*
 *      Procedure Type Declarations:
 */

STATUS 
csi_freeqmem (
    QM_QID queue_id,       /* queue which is to be manipulated */
    QM_MID member_id,      /* id of member on queue */
    CSI_VOIDFUNC log_fmt_func   /* pointer to function to use for logging */
)
{

    char        cvt[CVT_BUFSIZE];       /* general string conversion buffer */
    CSI_HEADER *datap;                  /* ptr to data area of queue member */

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 3,                             /* parameter count */
                 (unsigned long) queue_id,      /* argument list */
                 (unsigned long) member_id,     /* argument list */
                 (unsigned long) log_fmt_func); /* argument list */
#endif /* DEBUG */

    /* locate the data area */
    datap = (CSI_HEADER *) cl_qm_maccess(queue_id, member_id);

    if ((CSI_HEADER *) NULL == datap) {

            /* concatenate the error message with the queue_id and member id */
            MLOGCSI((STATUS_QUEUE_FAILURE,  st_module,  "cl_qm_maccess()", 
	      MMSG(950, "Can't locate queue Q-id:%d, Member:%d"),
	      queue_id,member_id));
            return(STATUS_QUEUE_FAILURE);
    }

    /* if instructed to, log the information being dropped from the queue */
    if ((CSI_VOIDFUNC) NULL != log_fmt_func) {
            (*log_fmt_func)(datap, cvt, CVT_BUFSIZE);
            MLOGCSI((STATUS_QUEUE_FAILURE,  st_module,  CSI_NO_CALLEE, 
	      MMSG(1026,"%s"),cvt));
    }

    /* delete node from the queue */
    if (cl_qm_mdelete(queue_id, member_id) != TRUE) {

            /* concatenate the error message with the queue_id and member id */
            MLOGCSI((STATUS_QUEUE_FAILURE,  st_module,  "cl_qm_mdelete()", 
	      MMSG(943,"Can't delete Q-id:%d, Member:%d"),queue_id,member_id));

    }

    return(STATUS_SUCCESS);

}
