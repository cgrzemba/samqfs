static char SccsId[] = "@(#)csi_net_send.c	5.16 1/21/95 (c) 1993 StorageTek";
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *  csi_net_send()
 *
 * Description:
 *      Controlling module for initiating network calls for the
 *  purpose of transmitting storage server packets.  This version only 
 *  supports RPC calls via TCP or UDP.  Input is tracked globally,
 *  and if detected, places a network packet on the network ouput queue,
 *  and returns to get the input.  This is done to prevent loss of packets
 *  at input.  
 *
 *  This routine can be called in two modes.  The first mode,
 *  CSI_FLUSH_OUTPUT_QUEUE, can be called to send any output that has
 *  been queue'd up as a result of input being detected during the sending
 *  process.  The second mode, CSI_NORMAL_SEND, is used for sending a 
 *  regular packet for the first time.  For CSI_NORMAL_SEND mode, the
 *  new packet is immediately placed at the end of the queue.  The ordering
 *  of packets is thus guaranteed, since the packets on the queue are
 *  naturally older than the new one just placed at the end of the queue.
 *  
 *
 *  First, checks to validate parameters and performs initializations.
 *
 *  If a new packet is passed in, it gets stamped with a transaction id.
 *  This is done at the top of this routine because the new packet is
 *  first placed at the end of the queue.
 *
 *  BEGIN LOOP:
 *
 *  In a loop, accesses each member of the output queue, starting in the
 *  first position, which is the oldest, and then attempts to send (flush)
 *  this member from the queue.
 *
 *  Gets the first member from the queue.
 *
 *  Checks for pending input.
 *
 *      If input is pending and this is a normal send operation as 
 *      opposed to a pure "flush" operation, the current new packet
 *      was already placed on the queue, so return to service input.
 *      The new packet will now be pending along with everything else
 *      on the queue. (Since this is a loop, messages on the queue are
 *      not removed from the queue until after it is sent or a send
 *      attempted too many times - and thus the message gets dropped.
 *      For more specifics, read on.)
 *
 *  The next member on the queue is also located and saved, and becomes
 *  the current member in the next iteration of the loop.   This is done
 *  because of a problem in the queueing routines where a linear
 *  next-member-search through the queue is undefined after a member has
 *  been deleted.
 *
 *  The currently queue'd network buffer member is retireved from the queue.
 *
 *      If STATUS_QUEUE_FAILURE is returned, then a message is logged
 *      and STATUS_QUEUE_FAILURE is returned to the caller.
 *  
 *  Obtains pointers into the csi header portion of the packet.
 *
 *  If the previous network transmission failed (badlist_enabled==TRUE),
 *  blindly assumes that there was a problem with the SSI/CSI where the
 *  previous message was sent, and continues to search the queue until
 *  a message for a different ssi is detected.  It is assumed that sending
 *  a message to a different ssi will keep things rolling.
 *
 *  Calls st_net_send() which actually handles a specific connections
 *  context. It is desireable to keep the upper level routine,
 *  csi_net_send() independent of any particular connection context in
 *  order that queueing of network output can be independent of the
 *  specific context employed, the ultimate goal being to make this routine
 *  easy to port.
 *
 *  St_net_send() will call csi_adi_send() to send the packet if 
 *  conditionally compiled with -DADI.
 *
 *      If the message is successfully sent or the maximum number of
 *      tries at sending it has been reached, then the current network
 *      buffer is removed from the queue.  If the buffer was removed
 *      from the queue due to the maximum number of tries being
 *      attempted, then the logging routine is selected to be passed
 *      to csi_freeqmem() in order that pertinent network information
 *      can be logged to the event log.
 *
 *      If the message failed for another reason the badlist_enabled
 *      flag is set = TRUE, a problem with the current client is
 *      assumed, the loop will attempt to locate a message on the
 *      queue for a different ssi.
 *  
 *  END LOOP
 *
 *  Returns STATUS_SUCCESS to the caller.
 *
 * Return Values:
 *      STATUS_SUCCESS      - Response successfully sent
 *  STAUS_RPC_FAILURE   - RPC callback to client failed
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *  NONE
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       10-Jan-1989.    Created.
 *      J. A. Wishner       30-May-1989.    Change so will queue up output
 *                      if a network timeout.
 *                      Retry logic moved up here from
 *                      csi_rpccall().  Transaction id
 *                      stamp also moved here as a result.
 *      J. S. Alexander     29-May-1990.    Added #ifdef  STATS.
 *	H. W. Whitener	    13-Nov-1991.    Added cl_chk_input
 *	A. W. Steere	    14-Nov-1991.    reFix wrong# of args 
 *					    cl_qm_mlocate().
 *      E. A. Alongi        05-Aug-1992     Added ADI changes from R3.0.1.
 *      E. A. Alongi        23-Sep-1992     Modifications to put the new
 *                                          (non-null) packet on the queue
 *                                          first, before flush of pending
 *                                          output.
 *      E. A. Alongi        30-Oct-1992     Replaced bcopy w/ memcpy.
 *      E. A. Alongi        06-Nov-1992     When passing error message info,
 *                                          convert ulong Internet address to
 *                                          dotted-decimal equiv using inet_ntoa
 *	Emanuel A. Alongi   16-Aug-1993     Modifications to convert to dynamic
 *					    variables.
 *	Emanuel A. Alongi   17-Aug-1993     Flint changes.
 *	Emanuel A. Alongi   01-Oct-1993     Cleanup flint detected warings.
 *	Emanuel A. Alongi   12-Nov-1993     Corrected all flint detected errors.
 *	Emanuel A. Alongi   16-Nov-1993     Ifndef'ed out the rpc call to
 *					    csi_rpccall() so we can eliminate
 *					    this routine from the ADI Makefiles.
 *	Emanuel A. Alongi   09-Feb-1994     Corrected the parameter to inet_ntoa
 *	Emanuel A. Alongi   10-Mar-1994     Eliminated #ifdef sun section which
 *					    used S_un.S_addr form to access
 *					    struct in_addr field.  This can be
 *					    s_addr which is #defined as
 *					    S_un.S_addr in netinet/in.h.
 *      Kenneth J. Stickney 24-Jan-1994     Changes to support network
 *                                          failure notification by SSI.
 *                                          See Toolkit BR#51 for more details.
 *      K. J. Stickney      26-Jan-1994     Redo checkin because used
 *                                          wrong form of command.
 *                                          Also fixed bug where message
 *                                          logging using the wrong status 
 *                                          variable.
 *	Mike Williams	03-May-2010	Modified to support 64-bit compile by 
 *					changing x_addr to be uint32_t.  This
 *					field must always hold 4 bytes.
 */

/*      Header Files: */

#include <string.h>
#include <stdio.h>
#ifdef SOLARIS
#include <netinet/in.h>
#endif
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"
#include "dv_pub.h"
#include "system.h"

/*  Defines, Typedefs and Structure Definitions: */
#define SSI_BADLIST_SIZE 200            /* max# of ssi's on badlist */

/*      Global and Static Variable Declarations: */
static char *st_module  = "csi_net_send()";
static int   st_seq_num = 0;            /* transaction sequence # */

/*      Global and static prototypes: */
static STATUS st_rpc_qout(CSI_MSGBUF *netbufp, STATUS status);
static STATUS st_net_send(CSI_MSGBUF *netbufp);

#ifndef ADI
char *inet_ntoa(struct in_addr in);
#endif

STATUS 
csi_net_send (
    CSI_MSGBUF *newpakp,  /* net packet buffer for new packet */
    CSI_NET_SEND_OPTIONS options   /* options */
)
{

    CSI_HEADER *cs_hdrp;            /* csi header */
    CSI_MSGBUF *netbufp;            /* network packet buffer for flush */
    STATUS      status = STATUS_SUCCESS;    /* return status */
    STATUS      api_status = STATUS_SUCCESS; /* status returned from 
						csi_ssi_api() */
    BOOLEAN badlist_enabled=FALSE;  /* TRUE to look for next ssi packet*/
    BOOLEAN     on_badlist;         /* TRUE to look for next ssi packet*/
    QM_MID      m_id;               /* member id number current node on Q */
    QM_MID      next_m_id;          /* member id number next node on Q */
    QM_QID      q_id;               /* id of current queue working with */
    CSI_VOIDFUNC log_func;          /* pointer to logging function */
    int         i;                  /* loop counter/index */
    int         cur_ssi_badlist_size=0;     /* current size of bad ssi list */
    CSI_XID     ssi_badlist[SSI_BADLIST_SIZE]; /* bad ssi send list */
    long        retries;            /* # of times for network retry - no retries
				       for ICL/ADI */
#ifndef ADI
    char        cvt[256];           /* general string conversion buffer */
    struct in_addr netaddr;         /* for conversion of inet addr to
				     * dotted-decimal notation */
    uint32_t x_addr;                /* temp storage for return address */
#endif


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, (int) 2, (unsigned long) newpakp,
         (unsigned long) options);

    /* bug trap, normal send requires a packet */
    if ((CSI_MSGBUF *)NULL == newpakp && CSI_NORMAL_SEND == options)
    {
        MLOGDEBUG(0, 
	  (MMSG(497, "%s: %s: status:%s; failed: %s\nbug trap"),
	  CSI_LOG_NAME, st_module, cl_status(STATUS_QUEUE_FAILURE), st_module));
    }
#endif /* DEBUG */

    q_id = csi_ni_out_qid;

    /* if a new packet is passed in, stamp it with a transaction id */
    if (NULL != newpakp && CSI_NORMAL_SEND == options) {
        cs_hdrp  = (CSI_HEADER *) CSI_PAK_NETDATAP(newpakp);

        /* stamp the transaction number id tag */
        memset((char *) &cs_hdrp->xid, '\0', sizeof(CSI_XID));

#ifndef ADI /* RPC code */
        x_addr = ntohl(cs_hdrp->csi_handle.raddr.sin_addr.s_addr);
        memcpy((char *) cs_hdrp->xid.addr,(char *)&x_addr,sizeof(x_addr));

#else /* ADI code */
        strncpy((char *)cs_hdrp->xid.client_name,
	    (char *)cs_hdrp->csi_handle.client_name, CSI_ADI_NAME_SIZE);
        cs_hdrp->xid.proc    = cs_hdrp->csi_handle.proc;
#endif /* ADI */

        cs_hdrp->xid.pid     = csi_pid;
        cs_hdrp->xid.seq_num = st_seq_num++;

        /* first put the packet on the queue, then flush the pending output */
        if ((status = st_rpc_qout(newpakp, STATUS_SUCCESS)) != STATUS_SUCCESS)
            return(status);
    }

    /* flush pending output - if newest packet was not null, then it should
     * have been put at end of queue in the previous code segment.  Now loop
     * through each of the queue's entries.
     */
    for (m_id = cl_qm_mlocate(q_id,QM_POS_FIRST, (QM_MID) 0);
                                                 0 != m_id; m_id = next_m_id) {

        if (cl_chk_input(0) == 1) { /* if input pending, service it */
            return(STATUS_PENDING);
        }

        /* get the client output that was buffered */
        if ((status = csi_qget(csi_ni_out_qid, m_id, (void **)&netbufp))
							   != STATUS_SUCCESS) {
            MLOGCSI((status,  st_module, "csi_qget()", 
	      MMSG(941, "Undefined message detected:discarded")));
            return(STATUS_QUEUE_FAILURE);
        }

        /* locate the next member on the queue */
        next_m_id = cl_qm_mlocate(q_id, QM_POS_NEXT, (QM_MID) 0);

        /* set pointers to access the ssi transaction identifier */
        cs_hdrp  = (CSI_HEADER *) CSI_PAK_NETDATAP(netbufp);

        /* if due to previous network error, must get next ssi, do so */
        if (badlist_enabled) {

            /* don't send packet if its ssi is already on the bad list */
            for (i = 0, on_badlist = FALSE; i < cur_ssi_badlist_size; i++) {

                /* see if this ssi is already on bad list */
                if (csi_ssicmp(&ssi_badlist[i], &cs_hdrp->xid) == 0) {

#ifdef DEBUG
#ifndef ADI /* debug RPC code */
                    MLOGDEBUG(0,
		       (MMSG(539,"%s: %s: Host on Bad list xid.addr = %lu"),
                        CSI_LOG_NAME, st_module,
			(unsigned long)cs_hdrp->xid.addr));
#else /* debug ADI code */
                    MLOGDEBUG(0,
		      (MMSG(540,
		      "%s: %s: Host on Bad list xid.client_name = %s"),
                      CSI_LOG_NAME, st_module, cs_hdrp->xid.client_name));
#endif /* ADI */
#endif /* DEBUG */
                    
                    on_badlist = TRUE;
                    break;
                }
            }

            /* if found ssi on badlist, don't send, get another off queue */
            if (on_badlist)
                continue;
        }

        /* flush queue'd network output */ 
	status = st_net_send(netbufp);

#ifdef ADI /* ADI code */

        /* set retry count to zero for ICL */
        retries = 0;
#else /* RPC code */

    	/* get retries for RPC */
    	if (dv_get_number(DV_TAG_CSI_RETRY_TRIES, &retries) !=
		      					      STATUS_SUCCESS ) {
	    retries = (long) CSI_DEF_RETRY_TRIES;
    	}
#endif /* ADI */

        /* if sent msg or done too many tries, delete this dude from queue */
        if (STATUS_SUCCESS == status || 
            			(long) netbufp->q_mgmt.xmit_tries >= retries) {

            /* log if dropping a failed message from the queue */
            if (STATUS_SUCCESS == status)
                log_func = CSI_NO_LOGFUNCTION;
            else
                log_func = (CSI_VOIDFUNC)csi_fmtniq_log;

#ifdef SSI /* SSI code */
#ifndef ADI /* the RPC version */
            if (STATUS_NI_FAILURE == status) {
                api_status = csi_ssi_api_resp(netbufp, status);
                if ( api_status != STATUS_SUCCESS ) {
                    MLOGCSI((api_status,  st_module, "csi_ssi_api_resp()", 
	              MMSG(MUNK, "Cannot send NI_FAILURE response to client")));
                    return(api_status);
                }
            }
#else /* the ADI code */
                /* if necessary */
#endif  /* ADI */

#endif /* SSI */

            /* record dropping of packet due to a timeout */
            if (STATUS_NI_TIMEDOUT == status) {

#ifdef SSI /* SSI code */

                /* just before the packet is discarded, the SSI must send back
                 * the appropriate response with status STATUS_NI_TIMEDOUT to
                 * the requestor.
                 */

#ifndef ADI /* the RPC version */
                api_status = csi_ssi_api_resp(netbufp, status);
                if ( api_status != STATUS_SUCCESS ) {
                    MLOGCSI((api_status,  st_module, "csi_ssi_api_resp()", 
	              MMSG(MUNK, "Cannot send NI_TIMEDOUT response to client")));
                    return(api_status);
                }
#else /* the ADI code */
                /* if necessary */
#endif  /* ADI */

#endif /* SSI */

                /* all retries have been done, so discard packet */
#ifndef ADI /* RPC code */
                memcpy((char *)&netaddr.s_addr,
		       (char*)&cs_hdrp->csi_handle.raddr.sin_addr,
		       sizeof(unsigned long));

                /* convert ulong inet address to dotted-decimal notation for
		 * log msg */
                MLOGCSI((status,  st_module,  "st_net_send()", 
		  MMSG(1022, "Cannot send message to NI:discarded, %s\n"
		  "Errno = %d (%s)\nRemote Internet address: %s, Port: %d"), 
                        "Network timeout", 0, "none", inet_ntoa(netaddr),
                        cs_hdrp->csi_handle.raddr.sin_port));
#else /* ADI code */
                MLOGCSI((status,  st_module,  "st_net_send()", 
		  MMSG(1023, "Cannot send message to NI:discarded, %s\n"
		  "Adman name:%s"), 
                        "Network timeout", 
                        cs_hdrp->csi_handle.client_name));
#endif /* ADI */
            }

#ifdef DEBUG
            {
		CSI_REQUEST_HEADER *rp;

                rp  = (CSI_REQUEST_HEADER *) CSI_PAK_NETDATAP(netbufp);
                MLOGDEBUG(0, 
		   (MMSG(541, "%s: %s: Removing command:%s mid:%d, seq#:%d"),
                   CSI_LOG_NAME, st_module, 
		   cl_command(rp->message_header.command),
                   m_id, cs_hdrp->xid.seq_num));
            }
#endif /* DEBUG */

            /* delete message from queue and record drop */
            if (csi_freeqmem(q_id, m_id, log_func) != STATUS_SUCCESS) {
               MLOGCSI((STATUS_QUEUE_FAILURE,  st_module, "csi_freeqmem()", 
		 MMSG(943, "Can't delete Q-id:%d, Member:%d"), q_id,m_id));

                /* serious error, for robustness, bail out doing nothing */
                return(STATUS_QUEUE_FAILURE);
            }
        } else {

            /* could not talk to this ssi, try send to another if in queue */
            badlist_enabled = TRUE;

            /* put this ssi on the bad ssi send list if list not full */
            if (cur_ssi_badlist_size >= SSI_BADLIST_SIZE) {

                /* if too many ssi send errors, get out, try later */
                return(STATUS_SUCCESS);
            } else {

                ssi_badlist[cur_ssi_badlist_size++] = cs_hdrp->xid;

#ifdef DEBUG
#ifndef ADI /* debug RPC code */
                memcpy((char *)&netaddr.s_addr,
		      (char*)&cs_hdrp->csi_handle.raddr.sin_addr,
		      sizeof(unsigned long));

                /* convert ulong inet addr to dotted-decimal notation for
		 * log msg */
                sprintf(cvt, 
                "Put on NI badlist - m_id:%d, pid:%d, "
			"address:%lu = %s, port:%d",
                        m_id, cs_hdrp->xid.pid, netaddr.s_addr,
			inet_ntoa(netaddr),
			cs_hdrp->csi_handle.raddr.sin_port);
	       CSI_DEBUG_LOG_NETQ_PACK((CSI_REQUEST_HEADER*)cs_hdrp,cvt,status);
#else /* debug ADI code */
               MLOGDEBUG(0,
		   (MMSG(542, 
                   "%s: %s: status:%s;\n"
		   "Put on NI badlist - m_id:%d, pid:%d, Adman name:%s"),
                   CSI_LOG_NAME, st_module, cl_status(status),
		   m_id, cs_hdrp->xid.pid, cs_hdrp->csi_handle.client_name));
#endif /* ADI */
#endif /* DEBUG */
            }
        }
    } /* end loop for flushing network output queue */

#ifdef STATS
    pe_collect(1,"csi_net_send: write_net       ", "/tmp/times.csi");
#endif /* STATS */

    return(STATUS_SUCCESS);
}

/*
 *  This function should not do any queueing.
 *
 *  Stamps the transaction number/id-tag structure for the new message
 *  to be sent.  This is so the receiver can use this information in
 *  validating packet sequencing and uniqueness.
 */
static STATUS 
st_net_send (
    CSI_MSGBUF *netbufp                                   /* network buffer */
)
{
    CSI_REQUEST_HEADER  *cs_req_hdrp;                   /* csi header */
    STATUS               status = STATUS_SUCCESS;       /* return status */

    cs_req_hdrp  = (CSI_REQUEST_HEADER *) CSI_PAK_NETDATAP(netbufp);

    CSI_DEBUG_LOG_NETQ_PACK(cs_req_hdrp, "Sending", STATUS_SUCCESS);

    /* show the attempt to send in the network buffer descriptor */
    netbufp->q_mgmt.xmit_tries++;

    /* make a network call for the appropriate communications context */
    switch(cs_req_hdrp->csi_header.csi_ctype) {

#ifndef ADI /* the rpc code */
        case CSI_CONNECT_RPCSOCK:
            /* make normal rpc call with as of yet un-queue'd packet */
            status = csi_rpccall(netbufp);
            break;

#else /* the adi code */
        case CSI_CONNECT_ADI:
            /* make normal adi call with as of yet un-queue'd packet */
            status = csi_adicall(netbufp);
            break;

#endif /* !ADI */

        default:
            /* invalid communications context */
            status = STATUS_INVALID_COMM_SERVICE;
            MLOGCSI((status, st_module,  CSI_NO_CALLEE, 
	      MMSG(945, "Invalid communications service")));
            break;
    }
    return(status);
}


static STATUS 
st_rpc_qout (
    CSI_MSGBUF *netbufp,           /* network working buffer */
    STATUS status            /* current status */
)
{
    QM_MID  mid;                /* queue member id returned */

    /* queue response packet for future transmission */
    if (csi_qput(csi_ni_out_qid, (char *) netbufp, CSI_MSGBUF_MAXSIZE,
					       &mid) != STATUS_SUCCESS)
        return(STATUS_QUEUE_FAILURE);

#ifdef DEBUG
{
    char    cvt[256];           /* gen string conversion buf */
    sprintf(cvt,"Placed on net queue, Q-id:%d, Member:%d", csi_ni_out_qid, mid);
    CSI_DEBUG_LOG_NETQ_PACK( ((CSI_REQUEST_HEADER *)netbufp->data),cvt,status);
}
#endif /* DEBUG */

    /*
     * Return status passed in, since half the time this is called, it is
     * called when input is pending:  STATUS_PENDING must be preserved upon
     * return to the caller.
     */
    return(status);
}
