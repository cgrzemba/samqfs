static char SccsId[] = "@(#)csi_qinit.c	5.6 2/4/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_qinit()
 *
 * Description:
 *
 *      Routine to initialize the CSI remote connections queue (connect table).
 *
 *      Initializes the remote connection table by calling cl_qm_init() with 
 *      CSI_MAXQUEUES and CSI_CONNECTQ_REMARKS as parameters.
 *
 *              If FALSE == cl_qm_init(), csi_qinit() calls
 *              csi_logevent() with a status of STATUS_QUEUE_FAILURE,
 *              and message MSG_QUEUE_CREATE_FAILURE.  Returns 
 *              STATUS_QUEUE_FAILURE.
 *
 *      Creates the queue itself by calling cl_qm_qcreate() with
 *      CSI_MAXMEMB_CONNECTQ and CSI_CONNECTQ_NAME as parameters.
 *
 *              If 0 == cl_qm_qcreate() (an error), csi_qinit() calls
 *              csi_logevent() with a status of STATUS_QUEUE_FAILURE, and
 *              message MSG_QUEUE_CREATE_FAILURE.  Returns 
 *              STATUS_QUEUE_FAILURE.
 *
 *              Else if cl_qm_qcreate() > 0, then csi_qinit() places the
 *              queue id returned in the global variable csi_lm_qid.
 *
 *      Return STATUS_SUCCESS.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS                  - Connection queue successfully created.
 *      STATUS_QUEUE_FAILURE            - Failed to initialize connection queue.
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      (QM_QID)        csi_lm_qid      - ID of csi LM connection queue table
 *
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
 *      J. A. Wishner       01-Jan-1989.    Created.
 *	E. A. Alongi	    15-Nov-1993.    Made flint warning free.
 *      
 *     
 */


/*
 *      Header Files:
 */
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"


/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_qinit()";
static BOOLEAN   st_qcb_initialized = FALSE;  /* TRUE if q cntl block created */

/*
 *      Procedure Type Declarations:
 */

STATUS 
csi_qinit (
    QM_QID *q_id,                  /* id returned for queue created */
    unsigned short max_members,           /* max size of queue */
    char *name                  /* name of queue */
)
{
    

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 0,                             /* parameter count */
                 q_id,                          /* parameter count */
                 (unsigned long) 0);            /* argument list */
#endif /* DEBUG */

    /* initialize the master queue structure if haven't done so */
    if (!st_qcb_initialized) {

        /* initialize the master queue structure */
        if (cl_qm_init(CSI_MAXQUEUES, CSI_QCB_REMARKS) != TRUE) {
            MLOGCSI((STATUS_QUEUE_FAILURE, st_module,  "cl_qm_init()", 
	      MMSG(951, "Queue creation failure")));
            return(STATUS_QUEUE_FAILURE);
        }
        st_qcb_initialized = TRUE;
    }

    /* create the queue itself */
    if ((*q_id = cl_qm_qcreate(max_members, name)) == 0) {
        MLOGCSI((STATUS_QUEUE_FAILURE, st_module,  "cl_qm_qcreate()", 
	  MMSG(951, "Queue creation failure")));
        return(STATUS_QUEUE_FAILURE);
    }

    return(STATUS_SUCCESS);

}
