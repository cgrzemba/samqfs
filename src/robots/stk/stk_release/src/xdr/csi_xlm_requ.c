static char SccsId[] = "@(#)csi_xlm_requ.c	5.7 10/12/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xlm_request()
 *
 * Description:
 *      Serialize and deserialize ACSLS request packets:
 *      o Determine buffer size.
 *      o Translate request header portion of the request.
 *      o Translate csi     header portion of the request.
 *      o Translate message header portion of the request.
 *      o Determine if the request is a duplicate.  If it is, discard.
 *      o Switch on the command portion of the request.  Call appropriate
 *        xdr routines to do encoding/decoding for the remainder of the request.
 *
 *      SERIALIZATION:
 *      For serialization of a request packet, xdrsp->x_op equals XDR_ENCODE.
 *      Upon entry to this routine, the data buffer description structure of
 *      type CSI_MSGBUF must be initialized as follows:
 *
 *      Upon Entry:
 *      o  bufferp->data            - ptr to memory containing a request packet
 *      o  bufferp->offset          - postion where data starts in buffer
 *      o  bufferp->size            - size of entire request packet in buffer
 *      o  bufferp->translated_size - 0 or don't care
 *      o  bufferp->packet_status   - don't care or CSI_PAKSTAT_INITIAL
 *      o  bufferp->maxsize         - allocated size of bufferp->data
 *      o  bufferp->requestor_type  - module requesting xdr services
 *
 *      Upon Exit:
 *      o  bufferp->data            - unaltered
 *      o  bufferp->offset          - unaltered
 *      o  bufferp->size            - unaltered
 *      o  bufferp->translated_size - bytes of request that xdr could translate
 *      o  bufferp->packet_status   - CSI_PAKSTAT_XLATE_ERROR or
 *                                    CSI_PAKSTAT_XLATE_COMPLETED
 *      o  bufferp->maxsize         - unaltered
 *      o  bufferp->requestor_type  - unaltered
 *
 *      DE-SERIALIZATION:
 *      For deserialization of a request packet, xdrsp->x_op equals XDR_DECODE.
 *      During deserialization, if "bufferp->data" is NULL, xdr will allocate
 *      memory for the packet read in off of the wire.  In this case it is
 *      the responsibility of the caller to free that memory.
 *
 *      After decoding the request header, determines if the packet is a
 *      duplicate by examining its transaction id (type CSI_XID) in the
 *      csi_header. If the packet is a duplicate returns 0 (failure) to caller.
 *
 *      Upon entry to this routine, the data buffer description structure of
 *      type CSI_MSGBUF must be initialized as follows:
 *
 *      Upon Entry:
 *      o  bufferp->data            - memory where request packet will be put
 *                              OR  - OR NULL for xdr to do allocation
 *      o  bufferp->offset          - starting position to put data in buffer
 *      o  bufferp->size            - 0 or don't care
 *      o  bufferp->translated_size - 0 or don't care
 *      o  bufferp->packet_status   - don't care or CSI_PAKSTAT_INITIAL
 *      o  bufferp->maxsize         - allocated size of bufferp->data
 *      o  bufferp->requestor_type  - 0 or don't care
 *
 *      Upon Exit:
 *      o  bufferp->data            - contains the translated data
 *      o  bufferp->offset          - unaltered
 *      o  bufferp->size            - bytes of request that xdr could translate
 *      o  bufferp->translated_size - bytes of request that xdr could translate
 *      o  bufferp->packet_status   - CSI_PAKSTAT_XLATE_ERROR or
 *                                    CSI_PAKSTAT_XLATE_COMPLETED or
 *                                    CSI_PAKSTAT_DUPLICATE_PACKET
 *      o  bufferp->maxsize         - unaltered bufferp->data not null on entry
 *                              OR  - if xdr allocated, bufferp->translated_size
 *      o  bufferp->requestor_type  - module xdr services being directed at
 *
 *      ERROR CONDITIONS AND HANDLING DURING TRANSLATION:
 *      During translation, portions of a packet might not be translated
 *      for numerous reasons following:
 *      o  xdr error,
 *      o  client packet format error,
 *      o  invalid command,
 *      o  invalid identifier type,
 *      o  invalid count,
 *      o  invalid packet size for designated storage server command
 *      o  invalid packet transaction id
 *      o  duplicate packet
 *
 *      If at least the request_header (type CSI_REQUEST_HEADER-csi_structs.h)
 *      can be processed, then a partial packet will be translated. The receiver
 *      of the partial packet can determine what the attempted operation was by
 *      by analyzing the request header and the downstream portions of the
 *      packet that were translated.  The amount of the packet that was
 *      actually translated and presumeably sent on the wire (barring a higher
 *      level Network layer error) is returned in "bufferp->translated_size".
 *
 * Return Values:
 *      (bool_t)        1 - successful xdr conversion of at least request header
 *      (bool_t)        0 - xdr conversion failed.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *      bufferp->data            - data is placed here during deserialization.
 *      bufferp->size            - size of data put here during deserialization.
 *      bufferp->packet_status   - describes various translation errors
 *      bufferp->translated_size - bytes of data the could be translated
 *
 * Considerations:
 *      Portability Issues--
 *      Note that sizing of the translated data must be done using address
 *      arithmetic rather than pure sizeof(data-structure) since sizeof
 *      is not a portable construct, especially for having this routine
 *      translate across different machine architectures and different
 *      versions of an operating system.  Different compilers will product
 *      different alignment of data structures and their contents.
 *      One can only do a sizeof() on the last element in the packet.
 *
 *      The CSI_PAK_NETDATAP() macro is used to extract the location of the
 *      data in the data buffer since the data may not start at the first byte,
 *      if the IPC_HEADER ever becomes larger than the CSI_HEADER.  The start
 *      of the data is always at bufferp->offset.
 *
 *      This routine may return a partial packet.  Return code will be (1)
 *      if at least a request header can be serialized/deserialized.
 *
 *      Variable "csi_xexp_size" (global int) must always be set near the top of
 *      this function since it is also used by static routines in this file.
 *
 *      Variable xdr_allocated is TRUE if buffer->data is NULL.  This action
 *      forces xdr to do the memory allocation for xdr.  This is required,
 *      although not visible in the file, since it is used by the macro
 *      called RETURN_PARTIAL_PACKET, currently defined in csi_xdr_xlate.h..
 *
 * Module Test Plan:
 *      See CSI Unit Test Plan
 *
 * Revision History:
 *      J. A. Wishner       10-Jan-1989.    Created.
 *      J. A. Wishner       10-Aug-1989.    Portabilit.  Changed size
 *                                          calculations for data structures
 *                                          so portable to other machines, such
 *                                          as SPARC.
 *      R. P. Cushman       24-Apr-1990.    Added compilable changes for pdaemon
 *      J. W. Montgomery    15-Jun-1990.    Version 2.
 *      J. A. Wishner       23-Sep-1990.    Strip out pdaemon changes.  Pdaemon
 *                                          code moved to another source file.
 *      J. W. Montgomery    29-Sep-1990.    Modified for OSLAN.
 *      H. I. Grapek        28-Aug-1991     Mods needed for release 3.
 *      J. A. Wishner       03-Oct-1991.    Complete mods release 3 (version 2).
 *      E. A. Alongi        29-Jul-1992     Modified for Version 0 packet 
 *                                          pseudo-elimination, minimum version
 *                                          allowed and Version 3 packet support
 *      E. A. Alongi        04-Nov-1992     When passing error message info,
 *                                          convert ulong Internet address to
 *                                          dotted-decimal equiv using inet_ntoa
 *	Emanuel A. Alongi   20-Jul-1993	    Modifications to encode/decode R5,
 *					    VERSION4 request packets. Deleted
 *					    global csi_minimum_version. Now will
 *					    access minimum version dynamically.
 *	Emanuel A. Alongi   09-Feb-1994.    Added prototype for inet_ntoa().
 *					    Flint cleanup.
 *	D. A. Myers	    12-Oct-1994	    Porting changes
 */

/*
 * Header Files
 */

#include "csi.h"
#include <memory.h>
#include <csi_xdr_xlate.h>
#include "dv_pub.h"
#include "cl_pub.h"
#include "ml_pub.h"

/*
 * Defines, Typedefs and Structure Definitions
 */

/*
 * Global and Static Variable Declarations
 */

static char     *st_module = "csi_xlm_request()";
static CSI_XID   st_dup_xid;
static BOOLEAN   st_init_done = FALSE;

/*	Global and Static Prototype Declarations: */
#ifdef DEBUG
#ifndef ADI
char *inet_ntoa(struct in_addr in);
#endif
#endif

/*
 * Procedure Type Declarations:
 */

bool_t 
csi_xlm_request (
    XDR *xdrsp,                 /* XDR handle */
    CSI_MSGBUF *bufferp        /* data buffer description */
)
{
    CSI_REQUEST     *reqp;                      /* latest request packet ptr*/
    CSI_V0_REQUEST  *reqp_v0;                   /* version 0 request */
    CSI_V1_REQUEST  *reqp_v1;                   /* version 1 request */
    CSI_V2_REQUEST  *reqp_v2;                   /* version 2 request */
    int              part_size = 0;             /* holds size one structure */
    BOOLEAN          badsize;                   /* TRUE flags if a size error */
    CSI_HEADER      *csi_hdr;                   /* packet ptr to csi header */
    BOOLEAN          xdr_allocated = FALSE;     /* see "Considerations */
    long	     minimum_version;		/* minimum version supported */

#ifdef DEBUG
#ifndef ADI
    struct in_addr   netaddr;                   /* for conversion of inet addr
                                                 * to dotted-decimal notation */
#endif

    if TRACE
        (CSI_XDR_TRACE_LEVEL)
            cl_trace(st_module, 2, (unsigned long) xdrsp, (unsigned long) bufferp);
#endif /* DEBUG */

    /* get minimum version supported */
    if (dv_get_number(DV_TAG_ACSLS_MIN_VERSION, &minimum_version) !=
		      STATUS_SUCCESS || minimum_version < (long)VERSION0 ||
		       			minimum_version > (long)VERSION_LAST ) {
	minimum_version = (long)VERSION_MINIMUM_SUPPORTED;
    }

    /* clear out the duplicate packet testing structure */
    if (FALSE == st_init_done) {
        memset((char*)&st_dup_xid, 0, sizeof(CSI_XID));
        st_init_done = TRUE;
    }

    /* initialize working pointer into packet of each version type */
    bufferp->packet_status = CSI_PAKSTAT_INITIAL;
    reqp      = (CSI_REQUEST *) CSI_PAK_NETDATAP(bufferp);
    reqp_v2   = (CSI_V2_REQUEST *) CSI_PAK_NETDATAP(bufferp);
    reqp_v1   = (CSI_V1_REQUEST *) CSI_PAK_NETDATAP(bufferp);
    reqp_v0   = (CSI_V0_REQUEST *) CSI_PAK_NETDATAP(bufferp);
    csi_hdr   = (CSI_HEADER *) CSI_PAK_NETDATAP(bufferp);
    csi_xcur_size = 0;

    /*
     * Determine expected buffer size.  Don't know what it is until we decode.
     * Caller may let xdr do the allocation, so set to MAX_MESSAGE_SIZE.
     */

    if (XDR_DECODE == xdrsp->x_op) {
        csi_xexp_size = MAX_MESSAGE_SIZE;
    }
    else {
        csi_xexp_size = bufferp->size;
        bufferp->service_type = TYPE_LM;
        if (csi_xexp_size > MAX_MESSAGE_SIZE) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return (0);
        }
    }

    /*
     * The data pointer must be valid for encoding xdr operations. If
     * decoding and is null, xdr allocates memory for the data
     */

    if (NULL == reqp) {
        if (XDR_ENCODE == xdrsp->x_op) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return (0);
        }
        else
            xdr_allocated = TRUE;                       /* see considerations */
    }

    /*
     * translate request header, must have at least a version0 header
     */

    part_size = sizeof(CSI_V0_REQUEST_HEADER);
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xreq_hdr(xdrsp, &reqp->csi_req_header)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xreq_hdr()", 
	  MMSG(928, "XDR message translation failure")));
        return (0);
    }

    /* 
     * size csi request header by version
     */

    if (reqp->csi_req_header.message_header.message_options & EXTENDED) {

        /* check packet version number against minimum version supported */
        if ((long)(reqp->csi_req_header.message_header.version) <
                                                          minimum_version) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
	      MMSG(924, "Invalid version number %d"),
                        (int) reqp->csi_req_header.message_header.version));
            return (0);
        }

        /* size version1 and later */
        switch (reqp->csi_req_header.message_header.version) {

		/* To figure the current size of a request packet, this routine
		 * decomposes the level of structure of the request.  On the 
		 * first level is the CSI_REQUEST, a union of structures which
		 * the pointer reqp points to.  Next, the union member is
		 * considered a CSI_CANCEL_REQUEST structure which is used to
		 * calculate the actual size of the request.  The
		 * CSI_CANCEL_REQUEST is composed of a CSI_REQUEST_HEADER and a
		 * MESSAGE_ID.  The MESSAGE_ID is filled in later.  We are only
		 * interested in the size of the CSI_REQUEST_HEADER which is
		 * subsequently composed of a CSI_HEADER and a MESSAGE_HEADER.
		 *
		 * Since the size of the CSI_REQUEST_HEADER has remained
		 * constant from R2 on, packet VERSION 1 through VERSION 4 can
		 * use the same size calculation to set the global variable
		 * csi_xcur_size (see below).  That is, for all packet versions
		 * except 0, the Release 2 struct CSI_V1_REQUEST_HEADER
		 * (VERSION1 packets), the Release 3 / Release 4 struct
		 * CSI_V2_REQUEST_HEADER (VERSION2 and VERSION3 packets) and
		 * the current Release 5 struct CSI_REQUEST_HEADER (VERSION4
		 * packets) all contain a CSI_HEADER structure and a 
		 * MESSAGE_HEADER structure which are identical in size and
		 * composition.
		 *
		 * On the other hand, the Release 1, CSI_V0_CANCEL_REQUEST
		 * structure is also composed of a CSI_V0_REQUEST_HEADER struct
		 * and a MESSAGE_ID struct.  The CSI_V0_REQUEST_HEADER struct,
		 * in turn, is again composed of a CSI_HEADER struct and a
		 * V0_MESSAGE_HEADER struct. But the struct V0_MESSAGE_HEADER
		 * is smaller than the struct MESSAGE_HEADER and thus 
		 * distinguishes VERSION 0 packets from VERSIONS 2, 3, 4 and 5.
		 */

	    case VERSION4:
            case VERSION3:
            case VERSION2:
            case VERSION1:

                csi_xcur_size = (char *) &reqp->csi_cancel_req.request
                    - (char *) &reqp->csi_cancel_req;
                break;

            default:

                /* log invalid version number as translation failure */
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
		  "csi_xreq_hdr()", 
		  MMSG(924, "Invalid version number %d"),
                    (int) reqp->csi_req_header.message_header.version));
                return (0);
           
        } /* end switch on version */
    }
    else {

        /* see if Version 0 packets are allowed. */
        if (minimum_version != (long) VERSION0) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
	      MMSG(924, "Invalid version number %d"),
                        (int) reqp->csi_req_header.message_header.version));
            return (0);
        }

        /* size version 0 */
        csi_xcur_size = (char *) &reqp_v0->csi_cancel_req.request
                        - (char *) &reqp_v0->csi_cancel_req;
    }

    /*
     * note that request header sizing must be done within each case below
     * rather than here since compiler padding of overall data structure
     * is dependent on the trailing data structures.  A portability
     * constraint.
     */

#ifndef SSI /* the csi code: */

    /* compare this xid to ones on csi connection queue, drop duplicates */
    if (XDR_DECODE == xdrsp->x_op) {
        if (0 == csi_qcmp(csi_lm_qid, &csi_hdr->xid, sizeof(CSI_XID))) {

#ifdef DEBUG
#ifndef ADI /* the debug csi RPC code: */
            memcpy((char*) &netaddr.s_addr, (char *) csi_hdr->xid.addr,
                                                        sizeof(unsigned long));

            /* convert ulong inet addr to dotted-decimal notation for log msg */
            MLOGCSI((STATUS_INVALID_MESSAGE, st_module,  CSI_NO_CALLEE, 
	      MMSG(925, "Duplicate packet from Network detected:discarded\n"
	  "Remote Internet address: %s, process-id: %d, sequence number: %lu"),
              inet_ntoa(netaddr), csi_hdr->xid.pid, csi_hdr->xid.seq_num));


#else /* ADI */ /* the debug csi OSLAN code: */
            MLOGCSI((STATUS_INVALID_MESSAGE, st_module,  CSI_NO_CALLEE, 
	      MMSG(926, "Duplicate packet from Network detected:discarded\n"
	      "Adman name:%s, process-id:%d, sequence number:%lu"),
                    csi_hdr->xid.client_name, csi_hdr->xid.pid,
                    csi_hdr->xid.seq_num));

#endif /* ADI */
#endif /* DEBUG */

            bufferp->packet_status = CSI_PAKSTAT_DUPLICATE_PACKET;
            RETURN_PARTIAL_PACKET(xdrsp, bufferp);
        }
    }

#else /* SSI */ /* the ssi code: */

    /* throw away duplicate message */
    if (XDR_DECODE == xdrsp->x_op) {
        if (0 == csi_xidcmp(&csi_hdr->xid, &st_dup_xid)) {

#ifdef DEBUG
#ifndef ADI /* the debug ssi RPC code: */
            memcpy((char*) &netaddr.s_addr, (char *) csi_hdr->xid.addr,
                                                        sizeof(unsigned long)); 

            /* convert ulong inet addr to dotted-decimal notation for log msg */
            MLOGCSI((STATUS_INVALID_MESSAGE, st_module,  CSI_NO_CALLEE, 
	      MMSG(925, "Duplicate packet from Network detected:discarded\n"
	  "Remote Internet address: %s, process-id: %d, sequence number: %lu"),
	      inet_ntoa(netaddr), csi_hdr->xid.pid, csi_hdr->xid.seq_num));


#else /* ADI */ /* the debug ssi OSLAN code: */
            MLOGCSI((STATUS_INVALID_MESSAGE, st_module,  CSI_NO_CALLEE, 
	      MMSG(926, "Duplicate packet from Network detected:discarded\n"
	      "Adman name:%s, process-id:%d, sequence number:%lu"),
                    csi_hdr->xid.client_name, csi_hdr->xid.pid,
                    csi_hdr->xid.seq_num));

#endif /* ADI */
#endif /* DEBUG */

            /* size to end of request header */
            bufferp->packet_status = CSI_PAKSTAT_DUPLICATE_PACKET;
            RETURN_PARTIAL_PACKET(xdrsp, bufferp);
        }
        else {
            /*
             * packet header accepted, test for next packet will be this
             * xid
             */
            st_dup_xid = csi_hdr->xid;
        }
    }

#endif /* SSI */

    if (reqp->csi_req_header.message_header.message_options & EXTENDED) {
        /* size version1 and later */
        switch (reqp->csi_req_header.message_header.version) {
	    case VERSION4:
                if (!csi_xv4_req(xdrsp, reqp)) 
                    RETURN_PARTIAL_PACKET(xdrsp, bufferp);
                break;
            case VERSION3:
            case VERSION2:
                if (!csi_xv2_req(xdrsp, reqp_v2)) 
                    RETURN_PARTIAL_PACKET(xdrsp, bufferp);
                break;
            case VERSION1:
                if (!csi_xv1_req(xdrsp, reqp_v1)) 
                    RETURN_PARTIAL_PACKET(xdrsp, bufferp);
                break;
        }
    }
    else {
        /* Version 0 */
        if (!csi_xv0_req(xdrsp, reqp_v0)) 
            RETURN_PARTIAL_PACKET(xdrsp, bufferp);
    }

    RETURN_COMPLETE_PACKET(xdrsp, bufferp);

}
