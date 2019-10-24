static char SccsId[] = "@(#)csi_rpcinput.c	5.4 10/12/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_rpc_input()
 *
 * Description:
 *
 *      Routine controls the transaction processing for an RPC request received
 *      across the NI.  Reads rpc input from the NI and sends the required 
 *      output to the ACSLM.
 *
 *      Calls svc_getargs() with the routine csi_xdrrequest() as a parameter as
 *      well as the general network request translation buffer.   
 *      Csi_xdrrequest() performs deserialization from NI XDR format and 
 *      places the decoded result in the network request buffer.
 *
 *              If 0 == svc_getargs(), calls svcerr_decode() with with the 
 *              transport handle as a parameter, notifying the caller of the 
 *              problem (Decoding of the message failed and error information 
 *              was recorded in the event log.) 
 *              Returns (0) to the caller.
 *
 *      Calls svc_sendreply() to ACK successful receipt of the request message.
 *
 *              If 0 == svc_sendreply(), calls csi_logevent() with 
 *              STATUS_RPC_FAILURE and message MSG_RPC_CANT_REPLY.
 *              Returns (0) to the caller.
 *
 *      Calls svc_freeargs() to free any memory allocated by RPC in
 *      the receiving/deserialization process.
 *
 *              If 0 == svc_freeargs(), calls csi_logevent() with
 *              STATUS_RPC_FAILURE.  
 *
 *      If inbufp->packet_status == CSI_PAKSTAT_DUPLICATE_PACKET, a duplicate 
 *      packet was detected.  This routine returns (0) (failure) so that the
 *      top layer will not continue to process this packet.
 *
 *      Returns (1) indicating successful completion.
 *
 * Return Values:
 *
 *      (1)             - Processing succeeded.  RPC requires either a 0 or 1 
 *                        return from dispatchers.
 *      (0)             - Processing failed.  RPC requires either a 0 or 1 
 *                        return from dispatchers.
 *
 * Implicit Inputs:
 *
 *      svc_fds         - (global) rpc file descriptors
 *
 * Implicit Outputs:
 *
 *      inbufp->data - receives the request packet
 *      inbufp->size - MUST be initialized to packet size by csi_xdrrequest
 *
 * Considerations:
 *
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       11-Jan-1989.    Created.
 *      J. A. Wishner       22-Sep-1990.    Installed performance #ifdef STATS.
 *	D. A. Myers	    12-Oct-1994	    Porting changes
 *      Mitch Black         07-Dec-2004     Type cast several variables to
 *                                          xdrproc_t to prevent Linux errors.
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
static  char *st_module = "csi_rpcinput()";
static  char *st_src = __FILE__;

/*
 *      Procedure Type Declarations:
 */


int 
csi_rpcinput (
    SVCXPRT *xprtp,      /* transport handle */
    xdrproc_t inproc,     /* input processing xdr procedure */
    CSI_MSGBUF *inbufp,     /* input buffer */
    xdrproc_t outproc,    /* output procedure for svc_sendreply */
    CSI_MSGBUF *outbufp,    /* output buffer */
    xdrproc_t free_rtn   /* routine frees memory allocated by deserialization */
)
{


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 6,                             /* parameter count */
                 (unsigned long) xprtp,         /* argument list */
                 (unsigned long) inproc,        /* argument list */
                 (unsigned long) inbufp,        /* argument list */
                 (unsigned long) outproc,       /* argument list */
                 (unsigned long) outbufp,       /* argument list */
                 (unsigned long) free_rtn);     /* argument list */
#endif /* DEBUG */


    /* get the request packet */
    if (!svc_getargs(xprtp, inproc, (char*)inbufp)) {

        /* error, cannot decode message */
        svcerr_decode(xprtp);

        /* log the error (may not be csi's problem) */
        MLOGCSI((STATUS_RPC_FAILURE, st_module,  "svc_getargs()",
	  MMSG(941, "Undefined message detected:discarded")));
        return(0);
    } 
            
    /* ack to the client the successful receipt of the storage server packet */
    if (!svc_sendreply(xprtp, (xdrproc_t)xdr_void, (char*)NULL)) {
        MLOGCSI((STATUS_RPC_FAILURE, st_module,  "svc_sendreply()", 
	  MMSG(960, "Cannot reply to RPC message")));
        return(0);
    }

    /* free any memory allocated by rpc/xdr due to translation */
    if (free_rtn != (xdrproc_t) NULL) {
        if (!svc_freeargs(xprtp, free_rtn, (caddr_t)inbufp)) {
            MLOGCSI((STATUS_RPC_FAILURE, st_module,  "svc_freeargs()", 
	      MMSG(961, "Cannot decode to free memory allocated by XDR")));
            return(0);
        }
    }

    /* do not continue to process packet if duplicate */
    if (CSI_PAKSTAT_DUPLICATE_PACKET == inbufp->packet_status)
        return(0);

#ifdef STATS
        pe_collect(0,"csi_rpcdisp: read_net         ");
#endif

    return(1);
}
