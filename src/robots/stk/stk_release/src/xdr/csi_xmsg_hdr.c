static char SccsId[] = "@(#)csi_xmsg_hdr.c	5.5 10/12/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xmsg_hdr()
 *
 * Description:
 *      Serializes/deserialized a message header structure.      
 *
 * Return Values:
 *      bool_t          - 1, message header decoded
 *      bool_t          - 0, error, message header not decoded
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. W. Montgomery    15-Aug-1990         Added Release 2 fields.
 *	D. A. Myers	    12-Oct-1994	    Porting changes
 */


/*      Header Files: */
#include "csi.h"
#include "ml_pub.h"
 
/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_src = __FILE__;
static char     *st_module = "csi_xmsg_hdr()";
 
/*      Procedure Type Declarations: */

bool_t 
csi_xmsg_hdr (
    XDR *xdrsp,                 /* xdr handle structure pointer */
    MESSAGE_HEADER *msghp                 /* message header structure */
)
{
#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp, (unsigned long) msghp);
#endif /* DEBUG */

    /* translate the packet id */
    if (!xdr_u_short(xdrsp, &msghp->packet_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the command */
    if (!xdr_enum(xdrsp, (enum_t *)&msghp->command)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the message options */
    if (!xdr_u_char(xdrsp, (u_char *)&msghp->message_options)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_char()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    if (!(msghp->message_options & EXTENDED)) {
        return (1);      /* Version 0 -  return here */
    }

    /* Version 1 (or greater) - translate remaining message header fields */

    /* translate version */
    if (!csi_xversion(xdrsp, &msghp->version)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xversion()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate extended_options */
    if (!xdr_u_long(xdrsp, &msghp->extended_options)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate lock_id */
    if (!csi_xlockid(xdrsp, &msghp->lock_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xlock_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate access_id */
    if (!csi_xaccess_id(xdrsp, &msghp->access_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xversion()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    return (1);
}
