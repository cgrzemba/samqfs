static char SccsId[] = "@(#)csi_xcsi_hdr.c	5.8 1/5/94 (c) 1993 StorageTek";
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      csi_xcsi_hdr()
 *
 * Description:
 *      Routine serializes/deserializes a CSI_HEADER structure.
 *      If -DADI has been conditionally compiled, use IPA (OSLAN) data structs.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 *      DO NOT CHECK PROCEDURE NUMBER, VERSION NUMBER, OR PROGRAM NUMBER.
 *      These are client-dependent.
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       01-Feb-1989.    Created.
 *      J. A. Wishner       11-Apr-1989.    Added translation for CSI_XID struct
 *      J. W. Montgomery    20-Sep-1990.    Added OSLAN stuff.
 *      J. W. Montgomery    20-Nov-1990.    Added field validation. 
 *      J. A. Wishner       20-Oct-1991.    Fix pre-EVT bug#84, ICL.
 *                          xadi_handle() missing the return(1) statement for
 *                          success case.
 *      J. A. Wishner       05-Nov-1991.    Fix incident numbers:
 *                                          F014512 F014357 F014252.  The were
 *                                          caused by StorageTek checking and
 *                                          validating the version number which
 *                                          is a client-dependent value.
 *	D. B. Farmer	    16-Aug-1993	    changes to handle differences in
 *					    sockaddr_in between sun and RS6000
 *	E. A. Alongi	    26-Oct-1993.    Eliminated reference to ACSPD.
 *	E. A. Alongi	    02-Dec-1993.    Added ifdefs between ADI and RPC
 *			    		    code.
 *	E. A. Alongi	    05-Jan-1994.    eliminated ifdef Sun section since
 *					    S_un.S_addr is defined as s_addr
 *					    on the Sun in netinet/in.h.
 *	S. L. Siao	    06-Mar-2002.    Added type cast to arguments of xdr
 *                                          functions.
 *	Mitch Black	07-Dec-2004	Type cast the 5th argument to xdr_vector 
 *					to prevent "incompatible pointer" 
 *					messages under Linux.
 *      Mike Williams   14-May-2010     For 64-bit compile Changed the xdr
 *                                      routine for the saddrp->sin_addr.s_addr
 *                                      field to be xdr_u_int rather than
 *                                      xdr_u_long.
 */


/*      Header Files: */
#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi_xdr_pri.h"
 
/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_module = "csi_xcsi_hdr()";
 
/*	static prototype declarations */

#ifndef ADI /* the rpc declarations */
static bool_t xrpc_handle(XDR *xdrsp, CSI_HANDLE_RPC *handle);
static bool_t xsockaddr_in(XDR *xdrsp, struct sockaddr_in *saddrp);

#else /* the adi declaration */
static bool_t xadi_handle (XDR *xdrsp, CSI_HANDLE_ADI *handle);

#endif /* ~ADI */



/*      Procedure Type Declarations: */

bool_t 
csi_xcsi_hdr (
    XDR *xdrsp,                         /* xdr handle structure */
    CSI_HEADER *csi_hdr                       /* csi header structure */
)
{

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp, 
            (unsigned long) csi_hdr);
#endif /* DEBUG */

#ifndef ADI
    /* translate the transaction id host address */
    if (!xdr_vector(xdrsp, (char *)csi_hdr->xid.addr, sizeof(csi_hdr->xid.addr),
        (int)1, (xdrproc_t)xdr_u_char)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_vector()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
#else
    /* translate the xid client_name */
    if (!xdr_vector(xdrsp, (char *)csi_hdr->xid.client_name, 
        sizeof(csi_hdr->xid.client_name), (int)1, (xdrproc_t)xdr_u_char)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_vector()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    /* translate the destination procedure number */
    if (!xdr_u_long(xdrsp, &csi_hdr->xid.proc)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    /* validate the proc */
    switch(csi_hdr->xid.proc) {
     case CSI_ACSLM_PROC:
        break;
     default:
        MLOGCSI((STATUS_NONE,              st_module,  st_module, 
	  MMSG(972, "Validation Warning: csi_header.xid.proc")));
    }
#endif /* ADI */

    /* translate the transaction id process id */
    if (!xdr_u_int(xdrsp, &csi_hdr->xid.pid)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_int()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the transaction id sequence number */
    if (!xdr_u_long(xdrsp, &csi_hdr->xid.seq_num)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the ssi_identifier */
    if (!xdr_u_long(xdrsp, &csi_hdr->ssi_identifier)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the csi_syntax */
    if (!xdr_enum(xdrsp, (enum_t *)&csi_hdr->csi_syntax)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    switch(csi_hdr->csi_syntax) {
     case CSI_SYNTAX_XDR:
        break;
     case CSI_SYNTAX_NONE:
     default:
        MLOGCSI((STATUS_NONE,             st_module,  st_module, 
	  MMSG(973, "Validation Warning: csi_header.csi_syntax")));
    }

    /* translate the csi_proto (protocol) */
    if (!xdr_enum(xdrsp, (enum_t *) &csi_hdr->csi_proto)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    switch(csi_hdr->csi_proto) {
     case CSI_PROTOCOL_TCP:
     case CSI_PROTOCOL_UDP:
     case CSI_PROTOCOL_ADI:
        break;
     default:
        MLOGCSI((STATUS_NONE,             st_module,  st_module, 
	  MMSG(974, "Validation Warning: csi_header.csi_proto")));
    }

    /* translate the csi_ctype (connection type) */
    if (!xdr_enum(xdrsp, (enum_t *) &csi_hdr->csi_ctype)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

#ifndef ADI
    /* translate the csi client_handle (RPC) */
    if (!xrpc_handle(xdrsp, &csi_hdr->csi_handle)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xrpc_handle()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
#else
    /* translate the csi client_handle (ADI) */
    if (!xadi_handle(xdrsp, &csi_hdr->csi_handle)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xadi_handle()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
#endif /* ADI */

    return(1);

}


#ifndef ADI /* the rpc version */

/*
 * Name:
 *
 *      xrpc_handle()
 *
 * Description:
 *
 *      Routine serializes/deserializes an CSI_HANDLE_RPC structure.
 *      Note:  procedure number, version number, program number SHOULD NOT BE
 *      VALIDATED.  These are client-dependent return address values and are
 *      not controlled by StorageTek.
 *
 * Return Values:
 *
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
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
 *      DO NOT CHECK PROCEDURE NUMBER, VERSION NUMBER, OR PROGRAM NUMBER.
 *      These are client-dependent.
 *
 */
static bool_t 
xrpc_handle (
    XDR *xdrsp,                 /* xdr handle structure */
    CSI_HANDLE_RPC *handle                /* csi client handle structure */
)
{
#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp,
             (unsigned long) handle);
#endif /* DEBUG */

    /* translate program number */
    if (!xdr_u_long(xdrsp, &handle->program)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate version number */
    if (!xdr_u_long(xdrsp, &handle->version)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /*
     * DO NOT VALIDATE THE VERSION, THIS IS CLIENT-DEPENDENT
     */

    /* translate proc (procedure) number */
    if (!xdr_u_long(xdrsp, &handle->proc)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /*
     * DO NOT VALIDATE THE PROCEDURE, THIS IS CLIENT-DEPENDENT
     */

    /* translate the sockaddr_in (socket addressing) structure */
    if (!xsockaddr_in(xdrsp, &handle->raddr)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xsockaddr_in()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    return(1);
}


/*
 * Name:
 *      xsockaddr_in()
 *
 * Description:
 *      Serializes/deserializes a "sockaddr_in" structure (netinet/in.h).
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 */
static bool_t 
xsockaddr_in (
    XDR *xdrsp,                      /* xdr handle structure */
    struct sockaddr_in *saddrp                     /* sockaddr_in structure */
)
{

#ifndef sun
    unsigned short temp_family;	/* network short */
#endif

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) saddrp);       /* argument list */
#endif /* DEBUG */

    /* translate sin_family */
#ifdef sun
    if (!xdr_u_short(xdrsp, &saddrp->sin_family)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
#else
    if(xdrsp->x_op == XDR_ENCODE)
	temp_family = (unsigned short)saddrp->sin_family;
	 
    if (!xdr_u_short(xdrsp, &temp_family)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    if(xdrsp->x_op == XDR_DECODE)
	saddrp->sin_family = temp_family;
#endif

    /* translate sin_port */
    if (!xdr_u_short(xdrsp, &saddrp->sin_port)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate sin_addr.s_addr */
    if (!xdr_u_int(xdrsp, (u_int *) &saddrp->sin_addr.s_addr)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_int()",
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate sin_zero */
    if (!xdr_char(xdrsp, (char *) saddrp->sin_zero)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_char()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    return(1);
}
#else /* the adi routine */
/*
 * Name:
 *      xadi_handle()
 *
 * Description:
 *      Routine serializes/deserializes an CSI_HANDLE_ADI structure.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 */
static bool_t 
xadi_handle (
    XDR *xdrsp,                 /* xdr handle structure */
    CSI_HANDLE_ADI *handle                /* csi client handle structure */
)
{

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp,
             (unsigned long) handle);
#endif /* DEBUG */

    /* translate client name */
    if (!xdr_vector(xdrsp, (char *)handle->client_name,
	sizeof(handle->client_name), (int)1, (xdrproc_t)xdr_u_char)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_vector()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate proc (procedure) number */
    if (!xdr_u_long(xdrsp, &handle->proc)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_u_long()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    return(1);
}

#endif /* ~ADI */

