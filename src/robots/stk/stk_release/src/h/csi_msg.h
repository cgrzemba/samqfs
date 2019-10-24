/* SccsId @(#)csi_msg.h	1.2 1/11/94  */
#ifndef _CSI_MSG_
#define _CSI_MSG_		/* where MODULE == header module name */
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *      functional description of objects defined in header file.
 *
 * Modified by:
 *
 *      J. A. Wishner   18-Jan-1988.    Created.
 *	J. A. Wishner   05/01/89.	TIME STAMP-POST CUSTOMER INITIAL RELEASE
 *      R. P. Cushman   16-May-1990.    Added PDAEMON changes.
 *      J. A. Wishner   22-Sep-1990.    Changed MSG_ACSLM_SEND_FAILURE to
 *                                          MSG_IPC_SEND_FAILURE.  
 *					Deleted MSG_ACSPD_SEND_FAILURE.
 *					Added MSG_INVALID_VERSION_NUMBER.
 *      J. W. Montgomery 28-Sep-1990.   Added ADI (OSLAN) message types.
 *      J. A. Wishner    22-Nov-1991.   Install MSG_RPC_INVALID_VERSION.
 *      E. A. Alongi     30-Jul-1992.   Added MSG_INVALID_ENVIRONMENT_VAR
 *                                      (VAR as in variable).
 *      E. A. Alongi     03-Dec-1992.   Changes from running flexelint.
 */

/*      Header Files: */

/*
 *	Defines, Typedefs and Structure Definitions:
 *		Enumerated type for csi messages.
 *
 *	Considerations:
 *		Must be kept in sync with the message declarations in
 *		csi_getmsg.c.
 */

typedef enum {
    MSG_FIRST = 0,			/* invalid */
    MSG_UNMAPPED_RPCSERVICE,
    MSG_RPCTCP_SVCCREATE_FAILED,
    MSG_RPCTCP_SVCREGISTER_FAILED,
    MSG_RPCUDP_SVCCREATE_FAILED,
    MSG_RPCUDP_SVCREGISTER_FAILED,
    MSG_INITIATION_STARTED,
    MSG_INITIATION_COMPLETED,
    MSG_INITIATION_FAILURE,
    MSG_CREATE_CONNECTQ_FAILURE,
    MSG_CREATE_NI_OUTQ_FAILURE,
    MSG_LOCATE_QMEMBER_FAILURE,
    MSG_DELETE_QMEMBER_FAILURE,
    MSG_SYSTEM_ERROR,
    MSG_UNEXPECTED_SIGNAL,
    MSG_RPC_INVALID_PROCEDURE,
    MSG_RPC_INVALID_PROGRAM,
    MSG_RPC_CANT_REPLY,
    MSG_RPCTCP_CLNTCREATE,
    MSG_RPCUDP_CLNTCREATE,
    MSG_INVALID_PROTO,
    MSG_QUEUE_CREATE_FAILURE,
    MSG_QUEUE_STATUS_FAILURE,
    MSG_QUEUE_MEMBADD_FAILURE,
    MSG_QUEUE_CLEANING,
    MSG_UNDEF_MSG,
    MSG_UNDEF_MSG_TRUNC,
    MSG_UNDEF_MODULE_TYPE,
    MSG_UNDEF_CLIENT,
    MSG_MESSAGE_SIZE,
    MSG_MESSAGE_SIZE_TRUNC,
    MSG_IPC_SEND_FAILURE,
    MSG_READ_FAILURE,
    MSG_SEND_NI_FAILURE,
    MSG_SEND_ACSSA_FAILURE,
    MSG_INVALID_COMM_SERVICE,
    MSG_XDR_XLATE_FAILURE,
    MSG_RPC_CANT_FREEARGS,
    MSG_QUEUE_ENTRY_DROP,
    MSG_UNDEF_HOST,
    MSG_INVALID_HOST,
    MSG_TERMINATION_STARTED,
    MSG_TERMINATION_COMPLETED,
    MSG_DUPLICATE_ACSLM_PACKET,
    MSG_INVALID_NI_TIMEOUT,
    MSG_DUPLICATE_NI_PACKET,
    MSG_NI_TIMEDOUT,
    MSG_UNEXPECTED_FAILURE,
    MSG_INVALID_COMMAND,
    MSG_INVALID_TYPE,
    MSG_INVALID_CONNECTQ_AGETIME,
    MSG_INVALID_LOCATION_TYPE,
    MSG_NONE_SPECIFIED,
    MSG_INVALID_VERSION_NUMBER,
    MSG_ADIOPEN_FAILURE,
    MSG_ADIREAD_FAILURE,
    MSG_ADIWRITE_FAILURE,
    MSG_ADI_SIGN_ON_FAILURE,
    MSG_ADI_SIGN_OFF_FAILURE,
    MSG_ADICLOSE_FAILURE,
    MSG_SEND_ADI_NI_FAILURE,
    MSG_QUEUE_ADI_ENTRY_DROP,
    MSG_DUPLICATE_ADI_NI_PACKET,
    MSG_PROC_WARNING,
    MSG_SYNTAX_WARNING,
    MSG_PROTOCOL_WARNING,
    MSG_VERSION_WARNING,
    MSG_HANDLE_PROC_WARNING,
    MSG_RPC_INVALID_VERSION,
    MSG_INVALID_ENVIRONMENT_VAR,
    MSG_LAST 				/* invalid */
} CSI_MSGNO;

/*      Procedure Type Declarations: */

#endif /* _CSI_MSG_ */

