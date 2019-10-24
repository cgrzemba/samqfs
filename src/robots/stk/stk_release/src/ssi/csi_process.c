static char SccsId[] = "@(#)csi_process.c	5.6 11/12/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_process
 *
 * Description:
 *
 *      Main processing controller function for the CSI.  
 *
 *      The following occurs in a processing loop:
 *
 *      BEGIN_LOOP:
 *
 *      Initializations:
 *
 *              Initializes queue cleaning timer and select timeouts.
 *              Initializes file descriptor mask.
 *              Initializes file descriptor array.
 *
 *      Calls cl_select_input() which detects and manages the demultiplexing
 *      of input to the CSI.  Passes a pointer to a file descriptor mask to be
 *      returned.
 *
 *      If cl_select_input() < 0 then select time'd out.  Check the connection
 *      queue for overaged connections.  This time is chosen since if time'd
 *      out there is not any network/acslm work pending (work which should 
 *      override the performance of this mundane task.).
 *
 *      If cl_select_input < 0
 *
 *              Determines if time to clean out connection queue.  If it is, 
 *              calls csi_qclean() to check the connection queue for outdated 
 *              return address entries which are to be logged and discarded.
 *
 *              If STATUS_SUCCESS != csi_qclean(), disregards this status,
 *              resets the variable for the time the queue was last cleaned.
 *
 *      Calls cl_ipc_send() to flush the IPC internal send queues.      
 *
 *      Checks if there is input pending.  If so, then continues to top of loop
 *      in order to get the input, otherwise, calls csi_net_send() in order to
 *      flush the csi internal network output queues.
 *
 *      Set the file descriptor mask returned from cl_select_input() for use
 *      by RPC svc_getreq() call.
 *
 *      IF input is pending on the ACSLM socket file descriptor, as determined
 *      by examining the file descriptor returned by cl_select_input()
 *      calls csi_ipcdisp() to get that input, which is a response packet in 
 *      ACSLM application format. 
 *
 *              If STATUS_QUEUE_FAILURE == csi_ipcdisp(), disregards this 
 *              and returns to the beginning of the processing loop. (Unable to
 *              get the return address from the connection queue.)  Although
 *              data may be lost, messages will have been logged to 
 *              the event log by a lower level routine.
 *
 *              If STATUS_IPC_FAILURE == csi_ipcdisp(), disregards this 
 *              and returns to the beginning of the processing loop.  (Unable
 *              to read the ACSLM file descriptor.)   Although
 *              data may be lost, messages will have been logged to 
 *              the event log by a lower level routine.
 *
 *              If STATUS_MESSAGE_TOO_SMALL == csi_ipcdisp(), disregards this 
 *              and returns to the beginning of the processing loop.  (Unable
 *              to interpret the response message packet.)  Although
 *              data may be lost, messages will have been logged to both
 *              the event log and the ACSSA by a lower level routine.
 *
 *              If STATUS_INVALID_MESSAGE == csi_ipcdisp(), disregards this 
 *              and returns to the beginning of the processing loop.
 *              (caused by receiving a duplicate message packet.)  Although
 *              data may be lost, messages will have been logged to both
 *              the event log and the ACSSA by a lower level routine.
 *
 *              If STATUS_RPC_FAILURE == csi_ipcdisp(), disregards this 
 *              and returns to the beginning of the processing loop.  (Unable
 *              to complete RPC connection and send to client SSI.)  Although
 *              data may be lost, messages will have been logged to both
 *              the event log and the ACSSA by a lower level routine.
 *
 *              If STATUS_NI_TIMEDOUT == csi_ipcdisp(), disregards this 
 *              and returns to the beginning of the processing loop.  (Client
 *              sent response but did not acknowledge receiving it.)  Although
 *              data may be lost, messages will have been logged to both
 *              the event log and the ACSSA by a lower level routine.
 *
 *      If input is pending on an RPC socket file descriptor: 
 *
 *              Calls svc_getreq() which multiplexes all RPC input and
 *              calls the RPC service dispatcher csi_rpc_dispatch() to service 
 *              incoming NI requests.  Lower level routines handle error event 
 *              logging.  The svc_getreq() function has no return code.
 *
 *      END_LOOP
 *
 * Return Values:
 *
 *      STATUS_PROCESS_FAILURE  - Returned at all times.
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
 *      NONE.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       01-Jan-1989.    Created.
 *      J. A. Wishner       22-Sep-1990.    Changed csi_lminput to csi_ipcdisp.
 *      J. A. Wishner       20-Oct-1991.    remove cvt, status, st_src; unused
 *      E. A. Alongi        30-Oct-1992.    Replace bzero with memset.
 *      E. A. Alongi        12-Feb-1993.    Eliminated DEC compatible 32 bit
 *                    descriptor mask assignment to the variable read_mask and
 *                    a later call to svc_getreq();
 *	Emanuel A. Alongi   28-Jul-1993	    Converted global csi_connect_agetime
 *		      to local variable which is set dynamically. 
 *	Emanuel A. Alongi   12-Nov-1993	    Corrected most flint problems.
 *     
 */


/*
 *      Header Files:
 */
#include <sys/time.h>
#include <string.h>
#include "cl_pub.h"
#include "csi.h"
#include "cl_ipc_pub.h"
#include "dv_pub.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */
#define CSI_QUEUE_CLEAN_TIMEOUT 300

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_process()";
static  long  st_last_cleaned;

/*
 *	Prototypes:
 */
void svc_getreqset(fd_set *);

/*
 *      Procedure Type Declarations:
 */


void 
csi_process (void)
{

    fd_set       read_mask;             /* file descriptor read mask */
    register int nfds;                  /* # of descriptors or a single fd # */
    register int i;                     /* counter for # of */
    long         tout;                  /* timeout for select call */
    long	 connect_agetime; 	/* aging time for connection */


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, 0, (unsigned long) 0);
#endif /* DEBUG */

    tout = CSI_SELECT_TIMEOUT;   /* set timeout for select call */

    st_last_cleaned = time(0);

    /* get connection aging time */
    if (dv_get_number(DV_TAG_CSI_CONNECT_AGETIME, &connect_agetime) !=
		      					       STATUS_SUCCESS ||
		      connect_agetime < (long) CSI_MIN_CONNECTQ_AGETIME ||
		          connect_agetime > (long) CSI_MAX_CONNECTQ_AGETIME ) {
	connect_agetime = (long) CSI_DEF_CONNECTQ_AGETIME;
    }

    for (;;) {

        /* set read mask to rpc input fds */
        FD_ZERO(&read_mask);
        read_mask = svc_fdset;

        /* set read mask to csi input fd */
        FD_SET(sd_in, &read_mask);

        /* initialize array of descriptors */
        memset((char*)fd_list, '\0', sizeof(fd_list));
        for (n_fds = i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_mask))
                fd_list[n_fds++] = i;
        }

        /* get input from either network or acslm */
        nfds = cl_select_input(n_fds, fd_list, tout);

        /* assume timeout, ignore errors */
        if (nfds < 0) {
            if ((time(0) - st_last_cleaned) >= CSI_QUEUE_CLEAN_TIMEOUT) {
                (void) csi_qclean(csi_lm_qid, connect_agetime, csi_fmtlmq_log);
                st_last_cleaned = time(0);
            }

	    /* flush IPC output */
            (void) cl_ipc_send((char *)NULL, (char *)NULL, 0, 0);

            if (cl_chk_input(0) == 1)           /* May need to service input */
                continue;

            /* flush the network output queue */
            (void) csi_net_send((CSI_MSGBUF *) NULL, CSI_FLUSH_OUTPUT_QUEUE);
            continue;
        }

        /* module input detected */
        if (fd_list[nfds] == sd_in) {

            /* service application level ipc input */
            memset((char *)csi_netbufp->data, '\0', (int)MAX_MESSAGE_SIZE);
            switch (csi_ipcdisp(csi_netbufp)) {

                case STATUS_PENDING:
                    /* input pending */
                    continue;
                case STATUS_QUEUE_FAILURE:
                case STATUS_IPC_FAILURE:
                case STATUS_MESSAGE_TOO_SMALL:
                case STATUS_INVALID_MESSAGE:                /* duplicate */
                case STATUS_RPC_FAILURE:
                case STATUS_NI_TIMEDOUT:
                default:
                    break;

            } /* end of switch */

        }
        else {

            /* service network level rpc input */
            FD_ZERO(&read_mask);
            FD_SET(fd_list[nfds], &read_mask);
            svc_getreqset(&read_mask);
        }

    } /* end of loop */

}




