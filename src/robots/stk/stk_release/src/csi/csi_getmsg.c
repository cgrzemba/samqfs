static char SccsId[] = "@(#)csi_getmsg.c	5.6 11/12/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_getmsg()
 *
 * Description:
 *      Functions returns a pointer to a csi message, accessed via  defined
 *      value (defined in csi_msg.h).
 *
 * Return Values:
 *      (char *)        - pointer to a static message string
 *
 * Implicit Inputs:
 *      st_msgtab       - static message table
 *
 * Implicit Outputs:
 *
 * Considerations:
 *      Messages must be kept in sync with the enumerated type CSI_MSGNO
 *      in csi_msg.h.  Also, table must be fully populated and messages must
 *      at least be defined as an empty string (at a minumum).
 *      
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       25-Jan-1989.    Created.
 *      J. A. Wishner       26-Jul-1989.    Added MSG_INVALID_CONNECTQ_AGETIME.
 *      J. A. Wishner       27-Jul-1989.    Added MSG_INVALID_LOCATION_TYPE.
 *      J. A. Wishner       22-Sep-1990.    Changed MSG_ACSLM_SEND_FAILURE to
 *                                              MSG_IPC_SEND_FAILURE.  
 *                                          Deleted MSG_ACSPD_SEND_FAILURE.
 *                                          Added MSG_INVALID_VERSION_NUMBER.
 *      J. A. Wishner       22-Nov-1991.    Install MSG_RPC_INVALID_VERSION.
 *      E. A. Alongi        05-Aug-1992     Modified MSG_QUEUE_STATUS_FAILURE,
 *                                          MSG_QUEUE_ENTRY_DROP and 
 *                                          MSG_QUEUE_ADI_ENTRY_DROP
 *      E. A. Alongi        04-Nov-1992     Changed all messages that printed
 *                                          ulong Internet addresses to print
 *                                          the dotted-decimal string equiv.
 *	David Farmer	    17-Aug-1993	    Changes for Bull port
 *      E. A. Alongi        12-Nov-1993.    Made flint error free.
 */


/*      Header Files: */
#include <stdio.h>
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_module = "csi_getmsg()";
static struct st_msg {
    char        *msg;
} st_msgtab [] = {
/* MSG_FIRST */                      "Invalid Message MSG_FIRST",
/* MSG_UNMAPPED_RPCSERVICE */        "Unmapped previously registered RPC service.",
/* MSG_RPCTCP_SVCCREATE_FAILED */    "Create of RPC TCP service failed",
/* MSG_RPCTCP_SVCREGISTER_FAILED */  "Can't register RPC TCP service",
/* MSG_RPCUDP_SVCCREATE_FAILED */    "Create of RPC UDP service failed",
/* MSG_RPCUDP_SVCREGISTER_FAILED */  "Can't register RPC UDP service",
/* MSG_INITIATION_STARTED */         "Initiation Started",
/* MSG_INITIATION_COMPLETED */       "Initiation Completed",
/* MSG_INITIATION_FAILURE */         "Initiation of CSI Failed",
/* MSG_CREATE_CONNECTQ_FAILURE */    "Creation of connect queue failed",
/* MSG_CREATE_NI_OUTQ_FAILURE */     "Creation of network output queue failed",
/* MSG_LOCATE_QMEMBER_FAILURE */     "Can't locate queue Q-id:%d, Member:%d",
/* MSG_DELETE_QMEMBER_FAILURE */     "Can't delete Q-id:%d, Member:%d",
/* MSG_SYSTEM_ERROR */               "Operating system error %d",
/* MSG_UNEXPECTED_SIGNAL */          "Unexpected signal caught, value:%d",
/* MSG_RPC_INVALID_PROCEDURE */      "Invalid procedure number",
/* MSG_RPC_INVALID_PROGRAM */        "Invalid RPC program number",
/* MSG_RPC_CANT_REPLY */             "Cannot reply to RPC message",
/* MSG_RPCTCP_CLNTCREATE */          "RPC TCP client connection failed, %s\nErrno = %d (%s)\nRemote Internet address: %s, Port: %d",
/* MSG_RPCUDP_CLNTCREATE */          "RPC UDP client connection failed, %s\nRemote Internet address: %s, Port: %d",
/* MSG_INVALID_PROTO */              "Invalid network protocol",
/* MSG_QUEUE_CREATE_FAILURE */       "Queue creation failure", 
/* MSG_QUEUE_STATUS_FAILURE */       "Can't get queue status Errno:%d, Q-id:%d, Member:%d",
/* MSG_QUEUE_MEMBADD_FAILURE */      "Can't add member to queue Q-id:%d",
/* MSG_QUEUE_CLEANING */             "Queue cleanup Q-id:%d.   Member:%d removed.",
/* MSG_UNDEF_MSG */                  "Undefined message detected: discarded",
/* MSG_UNDEF_MSG_TRUNC */            "Invalid message contents from NI: truncated",
/* MSG_UNDEF_MODULE_TYPE */          "Unsupported module type %d detected: discarded",
/* MSG_UNDEF_CLIENT */               "Message for unknown client discarded",
/* MSG_MESSAGE_SIZE */               "Invalid message size, %d, from NI: discarded",
/* MSG_MESSAGE_SIZE_TRUNC */         "Invalid message size, %d, from NI: truncated",
/* MSG_IPC_SEND_FAILURE */           "Cannot send message %s: discarded",
/* MSG_READ_FAILURE */               "Cannot read message from ACSLM : discarded",
/* MSG_SEND_NI_FAILURE */            "Cannot send message to NI: discarded, %s\nErrno = %d (%s)\nRemote Internet address: %s, Port: %d",
/* MSG_SEND_ACSSA_FAILURE */         "Cannot send message to ACSSA: discarded",
/* MSG_INVALID_COMM_SERVICE */       "Invalid communications service",
/* MSG_XDR_XLATE_FAILURE */          "XDR message translation failure",
/* MSG_RPC_CANT_FREEARGS */          "Cannot decode to free memory allocated by XDR",
/* MSG_QUEUE_ENTRY_DROP */  	     "Dropping from Queue: Remote Internet address: %s, Port: %d\n, ssi_identifier: %ld, Protocol: %d, Connect type: %d",
/* MSG_UNDEF_HOST */                 "Undefined hostname",
/* MSG_INVALID_HOST */               "Invalid hostname:%s",
/* MSG_TERMINATION_STARTED */        "Termination Started",
/* MSG_TERMINATION_COMPLETED */      "Termination Completed",
/* MSG_DUPLICATE_ACSLM_PACKET */     "Duplicate packet from ACSLM detected: discarded",
/* MSG_INVALID_NI_TIMEOUT */         "Invalid network timeout value",
/* MSG_DUPLICATE_NI_PACKET */        "Duplicate packet from Network detected: discarded\nRemote Internet address: %s, process-id: %d, sequence number: %lu",
/* MSG_NI_TIMEDOUT */                "Network timeout",
/* MSG_UNEXPECTED_FAILURE */         "Unexpected failure detected: errno=%d",
/* MSG_INVALID_COMMAND */            "Invalid command",
/* MSG_INVALID_TYPE */               "Invalid type",
/* MSG_INVALID_CONNECTQ_AGETIME */   "Invalid connection queue aging time:%s, default:%ld seconds substituted",
/* MSG_INVALID_LOCATION_TYPE */      "Invalid location type",
/* MSG_NONE_SPECIFIED */             "None specified",
/* MSG_INVALID_VERSION_NUMBER */     "Invalid version number %d",
/* MSG_ADIOPEN_FAILURE */            "Adiopen() failed:%d.  Adman name:%s",
/* MSG_ADIREAD_FAILURE */            "Adiread() failed:%d",
/* MSG_ADIWRITE_FAILURE */           "Adiwrite() failed:%d",
/* MSG_ADI_SIGN_ON_FAILURE */        "Adiwrite() of sign_on message failed:%d",
/* MSG_ADI_SIGN_OFF_FAILURE */       "Adiwrite() of sign_off message failed:%d",
/* MSG_ADICLOSE_FAILURE */           "Adiclose() failed:%d",
/* MSG_SEND_ADI_NI_FAILURE */        "Cannot send message to NI: discarded, %s\nAdman name:%s",
/* MSG_QUEUE_ADI_ENTRY_DROP */       "Dropping from Queue: Adman name:%s, ssi_identifier:%ld, Protocol:%d, Connect type:%d",
/* MSG_DUPLICATE_ADI_NI_PACKET */    "Duplicate packet from Network detected: discarded\nAdman name:%s, process-id:%d, sequence number:%lu",
/* MSG_PROC_WARNING */      	     "Validation Warning: csi_header.xid.proc",
/* MSG_SYNTAX_WARNING */     	     "Validation Warning: csi_header.csi_syntax",
/* MSG_PROTOCOL_WARNING */   	     "Validation Warning: csi_header.csi_proto",
/* MSG_VERSION_WARNING */    	     "Validation Warning: csi_header.csi_handle.version",
/* MSG_HANDLE_PROC_WARNING */	     "Validation Warning: csi_header.csi_handle.proc",
/* MSG_RPC_INVALID_VERSION */	     "Invalid RPC version number",
/* MSG_LAST */                       "Invalid Message MSG_LAST"
}; /* end of table declaration */


/*      Procedure Type Declarations: */

char *
csi_getmsg (
    CSI_MSGNO msgno                          /* message number */
)
{
    int i;

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, 1, (unsigned long) msgno);
#endif /* DEBUG */

    i = (int) msgno;

    if (i <= (int) MSG_FIRST || i >= (int) MSG_LAST) {
        MLOGCSI((STATUS_MESSAGE_NOT_FOUND,  st_module,  "", 
	  MMSG(934,"%s: %s: Invalid message."),CSI_LOG_NAME,st_module));
        return("error: unknown message");
    }
    return(st_msgtab[i].msg);
}
