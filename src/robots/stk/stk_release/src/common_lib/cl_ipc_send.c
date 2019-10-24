static char SccsId[] = "@(#)cl_ipc_send.c	5.3 9/29/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_ipc_send
 *
 * Description:
 *
 *      Common routine for sending IPC messages and maintaining a
 *      Write queue for the process:  
 *      Caller specifies destination socket name, buffer address, 
 *      buffer size and retry count.  The buffer is assumed to begin with 
 *      an IPC_HEADER structure.  If the socket name is NULL, then this 
 *      procedure has been invoked to flush the Write queue.
 *      If the Write queue is empty, attempt (retry_count times) to 
 *      transmit the buffer via cl_ipc_xmit().  Return if the message is
 *      transmitted successfully.  If an attempt fails and data exists on 
 *      input descriptor list (fd_list),
 *      then queue this outbound message and return to process the input.
 *      If the Write queue already contains outbound messages, attempt
 *      to flush the queue in FIFO order.  If a message is transmitted 
 *      successfully, it is deleted from the queue.  Once again, if any input
 *      data is present, return to handle this input.  Update and retain
 *      retry status for the current outbound message.
 *              
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_INVALID_VALUE    Invalid socket name passed.
 *      STATUS_IPC_FAILURE      I/O failure.
 *
 * Implicit Inputs:
 *
 *      cl_write_q      Write Queue for this process.
 *      sd_in           Process' socket descriptor (>= 0 if open).
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      Failures detected here are not logged due to the possibility
 *      of infinite recursion.
 *
 * Module Test Plan:
 *
 *      Verified in ipc_test.c
 *
 * Revision History:
 *
 *      D. F. Reed              28-Oct-1988     Original.
 *
 *      D. F. Reed              09-Mar-1989     Added signal acknowledgement
 *          to accommodate problems with sunos handling of unix-domain sockets.
 *
 *      D. F. Reed              14-Mar-1989     Modified to use internet
 *          sockets.
 *
 *      D. F. Reed              23-Mar-1989     Modified to only perform host
 *          name lookup once to improve performance.
 *
 *      D. F. Reed              23-Mar-1989     Modified to add retry_count
 *          as input parameter.
 *
 *      D. F. Reed              31-Mar-1989     Modified to use streams.
 *          Acknowledgement signal is eliminated.
 *
 *      Jim Montgomery          04-May-1989     Completely reorganized to 
 *          support the Write Queue.  Most of what was previously done in this
 *          module has been moved to cl_ipc_xmit().
 *
 *      D. F. Reed              23-May-1989     Modified to allow having
 *          multiple input descriptors checked for activity.
 *
 *      J. A. Wishner           15-Jun-1989     Modified to handle pending
 *          non-blocking socket connections coming from cl_ipc_xmit().
 *
 *      Jim Montgomery          02-Aug-1989     Use ALIGNED_BYTES when defining 
 *          char arrays.
 *
 *      D. A. Beidle            25-Nov-91.      IBR#151 - Changed to return 
 *          STATUS_IPC_FAILURE if the socket is open, check socket name length.
 *      Mitch Black     06-Dec-2004     Incompatible pointer-to-int
 *                                      comparison fixed.  cl_qm_create
 *                                      returns an int, so check for 0
 *                                      rather than NULL.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>                      /* ANSI-C compatible */
#include <string.h>                     /* ANSI-C compatible */
#include <errno.h>
#include <unistd.h>

#include "cl_pub.h"
#include "cl_ipc_pub.h"
#include "cl_qm.h"


/*
 *      Defines, Typedefs and Structure Definitions:
 */

typedef struct {
    ALIGNED_BYTES sock_name[(SOCKET_NAME_SIZE + sizeof (ALIGNED_BYTES) - 1) /
			    sizeof(ALIGNED_BYTES)];
    ALIGNED_BYTES buffer   [(MAX_MESSAGE_SIZE + sizeof (ALIGNED_BYTES) - 1) /
			    sizeof(ALIGNED_BYTES)];
    int  byte_count;
    int  retry_count;
} IPC_MSG;

/*
 *      Global and Static Variable Declarations:
 */
extern QM_QID   cl_write_q;

/*
 *      Procedure Type Declarations:
 */
static int write_q_add (char *sock_name, void *buffer, int byte_count,
			int retry_count);
static void write_q_del (QM_MID mid);


STATUS 
cl_ipc_send (
    char *sock_name,                     /* destination socket name */
    void *buffer,                        /* output buffer pointer */
    int byte_count,                     /* number of bytes to write */
    int retry_count                    /* number of retries before failure */
)
{
    QM_QCB     *qcb;
    QM_MID      mid;
    IPC_MSG    *mp;
    int         i;
    STATUS      s;

#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_ipc_send",         /* routine name */
                 4,                     /* parameter count */
                 (unsigned long)sock_name,
                 (unsigned long)buffer,
                 (unsigned long)byte_count,
                 (unsigned long)retry_count);
#endif /* DEBUG */


    /* no need to continue if sd_in socket not yet open */
    if (sd_in < 0) {
        errno = EBADF;
        return (STATUS_IPC_FAILURE);
    }


    /* validate sock_name and byte count parameters if socket name passed */
    if (sock_name) {

        /* check length of socket name */
        if (((i = strlen(sock_name)) < 1) || (i > SOCKET_NAME_SIZE))
            return (STATUS_INVALID_VALUE);

        /* check byte_count boundaries */
        if (byte_count < sizeof(IPC_HEADER))
            return (STATUS_COUNT_TOO_SMALL);

        if (byte_count > MAX_MESSAGE_SIZE)
            return (STATUS_COUNT_TOO_LARGE);
    }


    /* ensure queue manager still active */
    if (qm_mcb == (QM_MCB *)NULL) {
        return (STATUS_IPC_FAILURE);
    }


    qcb = qm_mcb->qcb[cl_write_q - 1];         /* Write Queue Control Block */

#ifdef DEBUG
    if (qcb->status.members >= 10) {
        fprintf(stderr,"cl_ipc_send: process:%d has %d write queue members.\n", 
            getpid(), qcb->status.members);
    }
#endif /* DEBUG */

    if (qcb->status.members == 0) {         /* Write Queue empty */
        if (sock_name == (char *)NULL) {
            return STATUS_SUCCESS;            /* Nothing to Flush */
        }

        for (i = 0; i < retry_count; ) {
            switch (cl_ipc_xmit(sock_name, buffer, byte_count)) {
             case STATUS_SUCCESS:
                return STATUS_SUCCESS;          /* We're done */
             case STATUS_PROCESS_FAILURE:
                return STATUS_IPC_FAILURE;      /* Don't do retries */
             case STATUS_PENDING:
                break;                          /* pending connect,no ++retry */
             default:
                i++;
                break;                          /* Stay here & Do retries */
            }
            if (cl_chk_input((long)RETRY_TIMEOUT)) {
                (void)write_q_add(sock_name, buffer, byte_count,
		  retry_count - i);
                return STATUS_SUCCESS;
            }
        }
        return STATUS_IPC_FAILURE;
    }

    /* Write_queue members exist.
     * Add this message to the end of the write queue.
     * Unless we are just trying to flush 
     */
    if (sock_name != (char *)NULL) {
        (void)write_q_add(sock_name, buffer, byte_count, retry_count);
    }

    /* Flush the Write_Queue */
    for (mid = cl_qm_mlocate(cl_write_q, QM_POS_FIRST, 0);
         mid;
         mid = cl_qm_mlocate(cl_write_q, QM_POS_FIRST, 0)) {

        if ((mp = (IPC_MSG *) cl_qm_maccess(cl_write_q, mid)) == NULL) {
            return STATUS_IPC_FAILURE;
        }
        for (i = 0; ; ) {

            s = cl_ipc_xmit((char *)mp->sock_name, mp->buffer, mp->byte_count);

            if (s == STATUS_SUCCESS) {    

                /* Cleanup & Do the next message */
                write_q_del(mid);
                break;
            }

            else if (s == STATUS_PROCESS_FAILURE) {

                /* drop this message & continue */
#ifdef DEBUG
                fprintf(stderr,"cl_ipc_send: PID %d dropping message to socket: %s, STATUS_PROCESS_FAILURE\n", 
                    getpid(), (char *)mp->sock_name);
#endif
                /* force i > retry count so member gets deleted below */
                i = mp->retry_count + 1;
                break;
            }

            else if (s != STATUS_PENDING) {
                /* retry if not done, don't ++count if connect still pending */
                if (++i >= mp->retry_count) 
                    break;
            }

            /* Do retries if no input */
            if (cl_chk_input((long)RETRY_TIMEOUT)) {

                /* do not count retry if we return without time-out */
                return STATUS_SUCCESS;
            }

            /* adjust message retry count if not waiting for pending connect */
            if (s != STATUS_PENDING)
                (mp->retry_count)--;

        } /* end of loop on retries */

        if (i >= mp->retry_count) {

            /* drop this message */
#ifdef DEBUG
            fprintf(stderr,"cl_ipc_send: PID: %d dropping message to socket: %s\n", 
                getpid(), (char *)mp->sock_name);
#endif
            write_q_del(mid);
        }

        /* get another off queue if no pending input */
        if (cl_chk_input(0L)) {
            return STATUS_SUCCESS;
        }

    } /* end of loop on queue access */

    return STATUS_SUCCESS;
}

static int 
write_q_add (
    char *sock_name,                     /* destination socket name */
    void *buffer,                        /* output buffer pointer */
    int byte_count,                     /* number of bytes to write */
    int retry_count                    /* number of retries before failure */
)
{
    QM_MID     mid;
    IPC_MSG    *mp;
    int        size;
    
    /* Queue this message and handle the input */
    size = sizeof(IPC_MSG);
    if ((mid = cl_qm_mcreate(cl_write_q, QM_POS_LAST, 0 , size)) == 0)  {
        return -1;
    }
    if ((mp = (IPC_MSG *) cl_qm_maccess(cl_write_q, mid)) == NULL)  {
        write_q_del(mid);
        return -1;
    }
    strncpy((char *)mp->sock_name, sock_name, SOCKET_NAME_SIZE);
    memcpy ((char *)mp->buffer, buffer, byte_count);
    mp->byte_count = byte_count;
    mp->retry_count = retry_count;
    return 0;
}

static void 
write_q_del (
    QM_MID mid
)
{
    /* Central point of Write_Queue deletions */
    (void)cl_qm_mdelete(cl_write_q, mid);
}
