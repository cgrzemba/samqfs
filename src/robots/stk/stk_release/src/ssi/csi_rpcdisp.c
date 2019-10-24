/**********************************************************************
*
*	C Source:		csi_rpcdisp.c
*	Subsystem:		2
*	Description:	
*	%created_by:	blackm %
*	%date_created:	Tue Dec 07 15:54:31 2004 %
*
**********************************************************************/
#ifndef lint
static char SccsId[] = "@(#) %filespec: csi_rpcdisp.c,2 %  (%full_filespec: csi_rpcdisp.c,2:csrc:1 %)";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *      csi_rpcdisp()
 *
 * Description:
 *      RPC dispatcher for multiplexing input between an arbitrary number of
 *      RPC procedures.  Receives notice of RPC input, determines the nature
 *      of the input based on the RPC procedure number, and calls necessary
 *      functions to input and translate the packet and route it to its
 *      intended destination.
 *
 *      This process can be "ping'd" using rpcinfo.  Will hit the NULLPROC.
 *      and return an "I-am-here" flavor message to the "pinger".
 *
 *      For the ACSLM procedure number, the following description is true:
 *
 *      o zeros out the following buffers
 *              - application socket
 *              - global network buffer data area
 *
 *      o sets a pointer into starting data position for the network packet. 
 *        Alignment is handled by using the CSI_PAK_NETDATAP macro in creating
 *        "net_datap" pointer to data in the buffer.  The macro measures network
 *        data starting byte offset from the start of the data buffer. 
 *
 *      o determines the procedure number that rpc input is directed at.
 *              - If procedure number invalid notifies the sender, returns(1).
 *
 *      # if conditional compile:  If compiling an SSI
 *              - Sets the "request" translation function to be used.
 *      # if conditional compile:  If compiling a  CSI
 *              - Sets the "response" translation function to be used.
 *      # end conditional compile
 *
 *      o calls input function for rpc input to get the packet buffer.
 *              - If 0 == return code (an error), returns (1) to the caller.
 *                No further processing of the current packet takes place.
 *      o Sets:
 *              - socket name
 *              - direction-of-processing string used in packet trace display
 *              - message options
 *
 *      o If packet tracing is set to TRACE_CSI_PACKETS:
 *              - converts network address and port number to strings
 *              - calls tracing function to log the packet contents.
 *
 *      # if conditional compile:  If compiling a CSI:
 *              - Puts packet csi_header (SSI's return address) on a queue, and
 *                returned is the id of the node in the connection queue. 
 *                If not STATUS_SUCCESS (error), returns (1) to the caller.
 *              - Places id into ipc_header to be sent to destination process.
 *      # else conditional compile:  If compiling an SSI:
 *              - Gets packet csi_header (SSI's return address) from a queue,
 *                using ssi_identifier from csi_header as key.
 *                If not STATUS_SUCCESS (error), returns (1) to the caller.
 *              - Saves ipc_header (destination ipc address) from queue 
 *              - Copies name of destination application socket to ipc_header
 *      # end conditional compile
 *
 *      o sets ipc_header module_type to TYPE_CSI 
 *
 *      o Sets pointer to starting data position for an acslm packet. Alignment
 *        is handled by using the CSI_PAK_LMDATAP macro in creating "ipc_datap"
 *        data pointer.  The macro measures acslm starting byte offset from the
 *        start of the data buffer.
 *
 *      o Converts the network format packet to an acslm format packet by
 *        overwriting the top of the packet with the ipc_header.  Alignment
 *        was handled above when the "ipc_datap" data pointer was created with
 *        a macro that handled measuring packet offset.
 *
 *      o Writes the ipc format packet to its intended destination.
 *              - If not STATUS_SUCCESS returns (1) to the caller.
 *              
 *      # if conditional compile:  If compiling an SSI
 *              - if a final response, deletes return address from the queue
 *      # end conditional compile
 *
 *      o Returns (0) as required by RPC.
 *
 *
 * Return Values:
 *
 *      (0)             - Processing succeeded.  RPC requires either a 0 or 1 
 *                        return from dispatchers.
 *      (1)             - Processing failed.  RPC requires either a 0 or 1 
 *                        return from dispatchers.
 *
 * Implicit Inputs:
 *
 *      csi_netbufp     - (global) network packet buffer structure
 *
 * Implicit Outputs:
 *
 *      csi_netbufp     - (global) packet copied to the acslm packet buffer 
 *
 * Considerations:
 *
 *      - trace_module (global common library) reset for packet trace in DEBUG
 *        by getenv() on every call: allows packet tracing to be toggled.
 *
 *      - New procedure numbers must set and manage the following variables
 *        within each procedure number case of the switch on procedure number:
 *
 *              o appl_socket
 *              o directp
 *              o msg_opt
 *              o xlate_func
 *
 *      - Race Condition:
 *
 *      In the case where the SSI time'd out sending to the CSI, but the CSI
 *      still got the message, if the CSI and ACSLM are fast enough and are
 *      pounding the SSI with input, which causes the SSI to have alot of
 *      network requests queue'd up, the CSI may actually get the acknowledge
 *      response and final response back to the SSI while the SSI still has the
 *      original request on its network send queue waiting to be retried.  So
 *      if the request completes here or if an acknowledge response is received
 *      (remember, there is only a final response for some requests under error
 *      conditions), then we must search the network output queue for that
 *      request and delete it so that the request isn't sent again.  Since once
 *      it returns a final, the CSI drops its knowledge of the request from its
 *      queues, it will treat the SSI's retried request as if it were brand new,
 *      ...it would not see it as a duplicate.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       12-Jan-1989.    Created.
 *      D. F. Reed          24-May-1989.    Added setting n_fds and fd_list
 *                          prior to cl_ipc_write call to avoid blocking NI
 *                          input when IPC output is blocked.
 *      J. A. Wishner       30-May-1989.    Changes since csi_netbufp now a ptr.
 *      R. P. Cushman       20-Apr-1990.    Added contionally compiled code
 *                                          for routing PDAEMON packets.
 *      J. S. Alexander     29-May-1990.    Added #ifdef  STATS.
 *      J. W. Montgomery    29-Sep-1990.    Modified for OSLAN.  
 *      R. T. Pierce        04-Apr-1991     changed test_hook call parameter
 *
 *      D. A. Beidle        02-Dec-1991.    Removed rebuild of fd_list since
 *          it's now done globally by csi_chk_input() which is a direct
 *          replacement for the cl_chk_input() common library routine.  This
 *          resolves the errno 9's seen on busy systems.  Some lint cleanup.
 *      E. A. Alongi        29-Jul-1992     Add checks to see if ACSPD allowed.
 *                                          Added csi_trace_flag.
 *      E. A. Alongi        22-Sep-1992     Modifications to support access
 *                                          control in rpc csi.
 *      E. A. Alongi        30-Oct-1992     Replaced bzero and bcopy with
 *                                          memset and memcpy respectively.
 *      Alec Sharp          24-Nov-1992     Copy name returned from
 *          cl_ac_internet to net_datap instead of the local copy in
 *	    cs_req_hdr. Also changed strcpy to strncpy for this copy.
 *      E. A. Alongi        03-Dec-1992     Changed function name cl_ac_internet
 *          to csi_get_accessid and added new second parameter. Also, modifica-
 *          tions resulting from running flexelint.
 *	Emanuel A. Alongi   08-Jun-1993     Prohibit access control call if 
 *	    VERSION0 packet encountered.  Eliminated #ifdef ADI preprocessor
 *	    directives - since csi_rpcdisp() is exclusively an RPC function.
 *	    Added comments to preprocessor statements.  Modifications to use
 *	    dynamic variables.
 *	Emanuel A. Alongi   20-Aug-1993     Added #ifndef ADI to keep adi make
 *	    				    happy - especially on the Bull.
 *      Alec Sharp   	    07-Sep-1993     Set the host_id field in the
 *                                          ipc_header.
 *	Emanuel A. Alongi   26-Oct-1993	    Eliminated all ACSPD support code.
 *	Emanuel A. Alongi   19-Nov-1993	    Cleaned up all flint detected 
 *					    anomalies.
 *	Emanuel A. Alongi   20-Dec-1993	    Added code to handle multi-homed
 *					    clients.
 *	Emanuel A. Alongi   05-Jan-1994	    changed csi_rpcdisp from returning
 *					    int to void - see svc_register()
 *					    prototype declaration.
 *	Emanuel A. Alongi   09-Feb-1994.    Corrected parameter to inet_ntoa()
 *					    and added its prototype. Eliminated
 *					    #ifndef ADI preprocessor directive
 *					    from around multi-homed host code.
 *      Ken Stickney        23-Dec-1994     Changes for Solaris port.
 *      Ken Stickney        26-Jan-1994     To fix improper platform tag.
 *                                          Was sunos5. Changed to none.
 *      Mitch Black         07-Dec-2004     Type cast several variables to
 *                                          xdrproc_t to prevent Linux errors.
 */

/*      Header Files: */
#include <stdio.h>
#include <string.h>
#include "cl_pub.h"
#include "cl_ipc_pub.h"
#include "csi.h"
#include "dv_pub.h"
#include "ml_pub.h"
#include "system.h"

#ifdef NOT_CSC /* production code - not intended for CSC developers. */
#include "cl_ac_pub.h"
#endif

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_module = "csi_rpcdisp()";

/*	Global and Static Prototype Declarations: */
#ifndef SOLARIS
bool_t svc_sendreply(SVCXPRT *, xdrproc_t, char *);
void svcerr_noproc(SVCXPRT *);
#endif

#ifndef SSI
char *inet_ntoa(struct in_addr in);
#endif

/*      Procedure Type Declarations: */
#ifdef SSI
static STATUS st_netq_delete(CSI_REQUEST_HEADER *cs_req_hdrp);
#endif

void 
csi_rpcdisp (
    struct svc_req *reqp,          /* pointer to rpc request handle */
    SVCXPRT *xprtp         /* pointer to rpc transport handle */
)
{
    IPC_HEADER     ipc_header;          /* ptr to ipc header to send to acslm */
    char          *ipc_datap;   /* start of lm data in packet buffer */
    char          *net_datap;           /* start of lm data in packet buffer */
    char           appl_socket[SOCKET_NAME_SIZE+1]; /* output socket */
    xdrproc_t      xlate_func;          /* csi xdr function to call */          
    char           cvt[256];            /* string conversion buffer */
    unsigned long  netaddr;             /* buffer for net address */
    char           netaddr_str[CSI_NAME_SIZE+1];/* string net address */
    char           port_str[CSI_NAME_SIZE+1];   /* string port number */
    STATUS         status;              /* return status */
    char          *directp;             /* string representing direction */
    CSI_REQUEST_HEADER cs_req_hdr;      /* csi request header recv'd network */

#ifndef SSI /* csi only declarations: */
    QM_MID         member_id;           /* member id generally used with Q's */
    BOOLEAN	   multi_homed_client;  /* multi-homed client flag */

#else /* ssi only declarations: */
    unsigned char  msg_opt;             /* message options */
    IPC_HEADER    *ipc_header_qp;       /* ptr to ipc header to send to acslm */

#endif /* !SSI */

#ifdef NOT_CSC          /* production code - not intended for CSC developers. */
    char          *client_name;         /* if not null, pointer to client name
                                         * returned by cl_ac_get_accessid(). */
#endif


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) reqp,          /* argument list */
                 (unsigned long) xprtp);        /* argument list */
#endif /* DEBUG */


    /*
     * setup
     */

    /* memset buffers */
    memset(appl_socket, '\0', SOCKET_NAME_SIZE);
    memset((char *)csi_netbufp->data, '\0', csi_netbufp->maxsize);

    /* set up pointers */
    net_datap  = CSI_PAK_NETDATAP(csi_netbufp);


    /*
     * rpc messages dispatched by procedure number
     */

    switch(reqp->rq_proc) {

    case NULLPROC:

        if (!svc_sendreply(xprtp, (xdrproc_t)xdr_void, 0)) {
            MLOGCSI((STATUS_RPC_FAILURE,  st_module, "svc_sendreply()", 
	      MMSG(953, "Invalid procedure number")));
        }
        return;

    case CSI_ACSLM_PROC:

#ifdef SSI /* ssi code */
/* goto */
acslm_alternate:                                /* allow ssi alternate proc# */

        /* translation function */
        /* SSI's receives storage server responses via rpc */
        xlate_func = (xdrproc_t)(*csi_xlm_response);
        directp = "To Client";

#else /* SSI - the csi code */

        /* CSI's receives storage server requests via rpc */
        xlate_func = (xdrproc_t)(*csi_xlm_request);
        directp = "To ACSLM";

#endif /* SSI */

        /* decode the rpc input and put it into the CSI_MSGBUF *csi_netbufp */
        if (!csi_rpcinput(xprtp,xlate_func,csi_netbufp,(xdrproc_t)xdr_void,NULL,NULL))
            return;

        strncpy(appl_socket, ACSLM, SOCKET_NAME_SIZE);
        CSI_DEBUG_LOG_NETQ_PACK((CSI_REQUEST_HEADER*)net_datap, "Received",
                                                            STATUS_SUCCESS);
        break;

    default:

#ifdef SSI /* ssi code */
        /* support ssi use of alternate return/callback procedure number */
        if (csi_ssi_alt_procno_lm == reqp->rq_proc)
            goto acslm_alternate;
/* goto */
#endif /* SSI */

        svcerr_noproc(xprtp);
        MLOGCSI((STATUS_INVALID_COMM_SERVICE,  st_module,  CSI_NO_CALLEE, 
	  MMSG(953, "Invalid procedure number")));
        return;

    }; /* end switch on procedure number */

    /* extract addressing information from the network data */
    cs_req_hdr = *(CSI_REQUEST_HEADER *)net_datap;

    /* format network addresses and ports */
    memcpy((char *)&netaddr,
                (char *)&cs_req_hdr.csi_header.csi_handle.raddr.sin_addr,
                                                        sizeof(unsigned long));
    CSI_INTERNET_ADDR_TO_STRING(netaddr_str, netaddr);
    sprintf(port_str, "%u", cs_req_hdr.csi_header.csi_handle.raddr.sin_port);

    /* packet trace if requested */
    if(csi_trace_flag)
        csi_ptrace(csi_netbufp, cs_req_hdr.csi_header.ssi_identifier,
                                        netaddr_str, port_str, directp);

#ifdef TEST_HOOK
    {
        extern csi_recv_test();
        csi_recv_test(&cs_req_hdr.csi_header);
    }
#endif /* TEST_HOOK */


    /*
     * message queueing/dequeueing
     */

#ifndef SSI /* the csi code */

    /* The following code segment addresses the problem where a client host
     * has two IP addresses; the host not necessarily a gateway.  The ssi
     * records its return address in the csi header.  The ssi uses 
     * gethostname() to return its hostname and gethostbyname() to return
     * the hostname inet address from the network host data base, /etc/hosts.
     * This may or may not access the correct inet address of a multi-homed
     * host.  This depends, of course, on whether or not the "correct" inet 
     * address is in the /etc/hosts data base.
     * 
     * An alternate approach is to use the information stored in the RPC
     * request handle.
     *
     */

    /* NOTE: for the CDK csi, we will never have a multi-homed client, so
     *       we will not use the following code:
     * if (dv_get_boolean(DV_TAG_CSI_MULTI_HOMED_CL, &multi_homed_client)
     *     != STATUS_SUCCESS) {
     *     multi_homed_client = FALSE;
     * }

     * We will just set multi_homed_client to TRUE.
     */

    multi_homed_client = FALSE;

#ifdef MULTI_HOMED
    if (multi_homed_client) {

	/* log the following for debug only: */
	MLOGDEBUG(0, (MMSG(1370, "The client supplied IP address from the "
			"csi_header.csi_handle of\nof the request: %s\n"),
	   inet_ntoa(cs_req_hdr.csi_header.csi_handle.raddr.sin_addr)));

        memcpy((char *)&cs_req_hdr.csi_header.csi_handle.raddr.sin_addr.s_addr,
              		(char *) reqp->rq_xprt->xp_raddr.sin_addr.s_addr,
	      		sizeof(reqp->rq_xprt->xp_raddr.sin_addr.s_addr));

	MLOGDEBUG(0, (MMSG(1371, "The IP address from the "
		"csi_header.csi_handle of the request after being\n"
		"overwritten with the SVCXPRT transport handle: %s\n"),
	   inet_ntoa(cs_req_hdr.csi_header.csi_handle.raddr.sin_addr)));
    }
#endif /* MULTI_HOMED */

    /* save the return address of the caller */
    if ((status = csi_qput(csi_lm_qid, (char *) &cs_req_hdr.csi_header,
			  sizeof(CSI_HEADER), &member_id)) != STATUS_SUCCESS) {
       MLOGCSI((status,  st_module, "csi_qput()", 
	 MMSG(958, "Message for unknown client discarded")));
       return;
    }

    /* save acslm node on the queue so he can return it in response */
    ipc_header.ipc_identifier = member_id;

#ifdef NOT_CSC /* production code - not intended for CSC developers. */

    if (strcmp(directp, "To ACSLM") == 0) {  /* only if destination is acslm */

	/* Fill in the host ID */
	cl_set_hostid (&ipc_header.host_id, netaddr_str);
	
	/* for access control - rpc only: the csi sends the string equivalent
	 * of the network address to cl_ac_get_accessid(). If the corresponding
	 * client name is returned, stuff if into the user_label field destined
	 * for the acslm.  Don't do this for VERSION0 packets because they
	 * don't have an access_id field.
	 */
	if (((CSI_REQUEST_HEADER *)net_datap)->message_header.message_options &
	    EXTENDED) {      /* VERSION0 packets do not have the EXTENDED bit */
	    
	    if ((client_name = cl_ac_get_accessid(netaddr_str, AC_GET_RPC))
                                                                      != NULL) {
		strncpy(((CSI_REQUEST_HEADER *)net_datap)->
			message_header.access_id.user_id.user_label,
			client_name, EXTERNAL_USERID_SIZE);
	    }
        }
    }    
#endif /* NOT_CSC */

#else /* the ssi code */

    /* if an ssi, get the return ipc address of the application */
    if ((status = csi_qget(csi_lm_qid, cs_req_hdr.csi_header.ssi_identifier,
                                (void **)&ipc_header_qp)) != STATUS_SUCCESS) {
       MLOGCSI((status,  st_module, "csi_qget()", 
	 MMSG(958, "Message for unknown client discarded")));
       return;
    }
    ipc_header = *ipc_header_qp;

    /* set the name of the socket to write to */
    strncpy(appl_socket, ipc_header.return_socket,SOCKET_NAME_SIZE);

#endif

    /*
     * ipc transmission
     */

    /* set ipc return address and sender module type */
    strcpy(ipc_header.return_socket, my_sock_name);
    ipc_header.module_type = my_module_type;

    /* set up a pointer to the ipc data in the buffer */
    ipc_datap = CSI_PAK_IPCDATAP(csi_netbufp);

    /* convert the net format packet to ipc format */
    * (IPC_HEADER *)ipc_datap = ipc_header;

    /* packet size changes by taking off csi_header & adding ipc_header */
    csi_netbufp->size += sizeof(IPC_HEADER) - sizeof(CSI_HEADER);

    /* send packet via ipc */
    if (cl_ipc_write(appl_socket, ipc_datap, csi_netbufp->size)
                                                    != STATUS_SUCCESS) {
        MLOGCSI((STATUS_IPC_FAILURE,  st_module, CSI_NO_CALLEE, 
	  MMSG(959, "Cannot send message %s:discarded"), directp));
        return;
    }

#ifdef STATS
    pe_collect(0,"csi_rpcdisp: write_lm         ");
#endif /* STATS */

#ifdef DEBUG
    sprintf(cvt,"Wrote %d bytes to ACSLM or application", csi_netbufp->size);
    CSI_DEBUG_LOG_NETQ_PACK(&cs_req_hdr, cvt, STATUS_SUCCESS);
#endif /* DEBUG */

#ifdef SSI

    /*
     * message removal from queue
     */

    msg_opt = ((CSI_REQUEST_HEADER*)
                                net_datap)->message_header.message_options;

    /* ack-final, clear from net outputQ see "Considerations: Race Condition" */
    if ((msg_opt & ACKNOWLEDGE) || (TRUE == CSI_ISFINALRESPONSE(msg_opt))) {
        status = st_netq_delete(&cs_req_hdr);
        if (STATUS_SUCCESS != status && STATUS_NONE != status)
            return;
    }
    if (TRUE == CSI_ISFINALRESPONSE(msg_opt)) {
        if (csi_freeqmem(csi_lm_qid, cs_req_hdr.csi_header.ssi_identifier,
                                    CSI_NO_LOGFUNCTION) != STATUS_SUCCESS) {
            return;
        }
    }
#endif /* SSI */

    return;
}


#ifdef SSI
/*
 * Name:
 *
 *      st_netq_delete()
 *
 * Description:
 *
 *      Looks for an ssi identifier on the network output queue which matches
 *      the member id of the request on the request queue.  If it finds it,
 *      it deletes it since we have already received a response for this
 *      request and the only reason that it is on the network output queue
 *      is due to the "Considerations:  Race Condition" listed in the top
 *
 * Return Values:
 *
 *      STATUS_SUCCESS                  Found entry on network queue.
 *      STATUS_NONE                     Did not find entry on network queue.
 *      STATUS_QUEUE_FAILURE            Error in queueing routines.
 *
 * Implicit Inputs:
 *
 *      csi_ni_out_qid (global)         Queue identifer for output queue.
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      See "Considerations:  Race Condition" in top-of-file prologue.
 *
 */
static STATUS 
st_netq_delete (
    CSI_REQUEST_HEADER *cs_req_hdrp
)
{
    QM_MID          mid;                        /* current member id if any */
    QM_MID          next_mid;           /* next member id if any */
    QM_MID          connectq_mid;       /* ssi identifier to search for */
    CSI_HEADER     *cs_hdrp;            /* csi header in queue'd packet */
    CSI_MSGBUF     *netbufp;            /* nework buffering structure */
    CSI_VOIDFUNC    log_fmt_func;       /* ptr to function to use for logging */
    char            cvt[256];           /* gen string buffer */

    connectq_mid = cs_req_hdrp->csi_header.ssi_identifier;

    /* search net output queue to see if this request is about to be retried */
    for (mid = cl_qm_mlocate(csi_ni_out_qid, QM_POS_FIRST, (QM_MID) 0); 
                                                 0 != mid; mid = next_mid) {

        /* locate the next member on the queue */
        next_mid = cl_qm_mlocate(csi_ni_out_qid, QM_POS_NEXT, (QM_MID) 0);

        /* get net output that was buffered */
        if (csi_qget(csi_ni_out_qid, mid, (void **)&netbufp) != STATUS_SUCCESS)
            return(STATUS_QUEUE_FAILURE);
                                                     
        /* set pointers to access the ssi transaction identifier */
        cs_hdrp  = (CSI_HEADER *) CSI_PAK_NETDATAP(netbufp);

        /* if found net outputQ member corresponding to this request, drop it */
        if (connectq_mid == cs_hdrp->ssi_identifier) {

#ifndef DEBUG
            /* only log the dropping of packet if not under debug */
            log_fmt_func = CSI_NO_LOGFUNCTION;
#else
            log_fmt_func = (CSI_VOIDFUNC)csi_fmtniq_log;
            sprintf(cvt, 
            "Race condition between connect and network queues: synchronized.");
            CSI_DEBUG_LOG_NETQ_PACK(cs_req_hdrp, cvt, STATUS_SUCCESS);
#endif

            /* delete it so ssi won't retry the request later */
            if (csi_freeqmem(csi_ni_out_qid,mid,log_fmt_func) != STATUS_SUCCESS)
                return(STATUS_QUEUE_FAILURE);

            return(STATUS_SUCCESS);
        }

    } /* end for loop */

    return(STATUS_NONE);
}

#endif /*SSI*/
