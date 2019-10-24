#ifndef lint
static char SccsId[] = "@(#)csi_xlm_resp.c	5.7 2/9/94 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xlm_response()
 *
 * Description:
 *
 *      CSI high level xdr based routine for serializing and deserializing
 *      storage server response packets.  The routines in this source and those
 *      called from this source support a bi-directional protocol for either
 *      encoding or decoding storage server response packets based on the value
 *      of the XDR handle's "xdrsp->x_op" directional variable.
 *
 *      The data buffer passed, "bufferp->data", is encoded during serialization
 *      for up to "bufferp->size" bytes.  During deserialization, the XDR stream
 *      is translated out of the xdr handle "xdrsp" into "bufferp->data"
 *      beginning  at  the  offset  specified  by  "bufferp->offset".
 *      The number of bytes of the packet that were successfully translated
 *      is returned in "bufferp->translated_bytes".
 *
 *      This routine will only return an error to the rpc layer (return 0)
 *      if the contents of the request header could not be translated.
 *
 *      If the request header was translated but there was a translation error
 *      lower in the packet, "bufferp->packet_status" equals
 *      CSI_PAKSTAT_XLATE_ERROR otherwise it equals CSI_PAKSTAT_XLATE_COMPLETED.
 *
 *      If a duplicate packet is detected during deserialization, this routine
 *      returns "bufferp->packet_status" equal to CSI_PAKSTAT_DUPLICATE_PACKET.
 *      In this case, upon return, only the CSI request header portion of the
 *      packet will have been translated.  Note the conditional compilation
 *      differences between a CSI and an SSI in checking for a duplicate packet.
 *      The SSI can use a static comparison test of the previous packet against
 *      a new packet since it always gets responses in order from a particular
 *      CSI, hence the use of routine csi_xidcmp().  A CSI, however, gets
 *      responses from mulitple ssi's, and therefore must search its connection
 *      queue to see if it has already received a particular message, hence the
 *      use of routine csi_qcmp().
 *
 *      SERIALIZATION:
 *      For serialization of a response packet, xdrsp->x_op equals XDR_ENCODE.
 *      Upon entry to this routine, the data buffer description structure of
 *      type CSI_MSGBUF must be initialized as follows:
 *
 *      Upon Entry:
 *      o  bufferp->data            - ptr to memory containing a response packet
 *      o  bufferp->offset          - position where data starts in buffer
 *      o  bufferp->size            - size of entire response packet in buffer
 *      o  bufferp->translated_size - 0 or don't care
 *      o  bufferp->packet_status   - don't care or CSI_PAKSTAT_INITIAL
 *      o  bufferp->maxsize         - allocated size of bufferp->data
 *
 *      Upon Exit:
 *      o  bufferp->data            - unaltered
 *      o  bufferp->offset          - unaltered
 *      o  bufferp->size            - unaltered
 *      o  bufferp->translated_size - bytes of response that xdr could translate
 *      o  bufferp->packet_status   - CSI_PAKSTAT_XLATE_ERROR or
 *                                    CSI_PAKSTAT_XLATE_COMPLETED
 *      o  bufferp->maxsize         - unaltered
 *
 *      DE-SERIALIZATION:
 *      For deserialization of a response packet, xdrsp->x_op equals XDR_DECODE.
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
 *      o  bufferp->data            - memory where a response packet will be put
 *                               OR - NULL for xdr to do allocation
 *      o  bufferp->offset          - starting position to put data in buffer
 *      o  bufferp->size            - 0 or don't care
 *      o  bufferp->translated_size - 0 or don't care
 *      o  bufferp->packet_status   - don't care or CSI_PAKSTAT_INITIAL
 *      o  bufferp->maxsize         - allocated size of bufferp->data
 *
 *      Upon Exit:
 *      o  bufferp->data            - contains the translated data
 *      o  bufferp->offset          - unaltered
 *      o  bufferp->size            - bytes of response that xdr could translate
 *      o  bufferp->translated_size - bytes of response that xdr could translate
 *      o  bufferp->packet_status   - CSI_PAKSTAT_XLATE_ERROR or
 *                                    CSI_PAKSTAT_XLATE_COMPLETED or
 *                                    CSI_PAKSTAT_DUPLICATE_PACKET
 *      o  bufferp->maxsize         - unaltered bufferp->data not null on entry
 *                               OR - if xdr allocated, bufferp->translated_size
 *
 *      ERROR CONDITIONS AND HANDLING DURING TRANSLATION:
 *      During translation, portions of a packet might not be translated
 *      for numerous reasons following:
 *
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
 *      bool_t          - 1 successful xdr conversion of at least request header
 *      bool_t          - 0 xdr conversion failed
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
 *      The value of packet size returned is undefined when 0 return code
 *      (error) is returned.
 *
 *      xdr_allocated:  Is useed by RETURN_PARTIAL_PACKET macro in
 *      csi_xdr_xlate.h and used here.  Is TRUE if bufferp->data == null
 *      which forces xdr to do the memory allocation during decoding
 *
 *
 * Module Test Plan:
 *      See CSI Unit Test Plan
 *
 * Revision History
 *      J. A. Wishner       10-Jan-1989.    Created.
 *      J. A. Wishner       10-Aug-1989.    Portability.  Changed size
 *                                          calculations for data structures
 *                                          so portable to other machines, such
 *                                          as SPARC.
 *      J. A. Wishner       08-Sep-1989.    Portability.  Change size calulation
 *                                          for CSI_ACKNOWLEDGE_RESPONSE.
 *      R. P. Cushman       24-Apr-1990.    Added compilable changes for pdaemon
 *      J. W. Montgomery    15-Jun-1990.    Version 2.
 *      J. A. Wishner       08-Sep-1989.    Created, split out pdaemon xdr code
 *                                          from csi_xdrresponse() and removed
 *                                          csi_xdrrespo.c.
 *      J. W. Montgomery    29-Sep-1990.    Modified for OSLAN.
 *      H. I. Grapek        28-Aug-1991     Changes needed for RELEASE 3.
 *      J. A. Wishner       03-Oct-1991.    Complete mods release 3 (version 2).
 *                                          Delete command; unused.
 *      E. A. Alongi        29-Jul-1992     Modified for Version 0 packet 
 *                                          pseudo-elimination, minimum version
 *                                          allowed and Version 3 packet support
 *      E. A. Alongi        30-Oct-1992     Replaced bzero and bcopy with
 *                                          memset and memcpy respectively.
 *      E. A. Alongi        04-Nov-1992     When passing error message info,
 *                                          convert ulong Internet address to
 *                                          dotted-decimal equiv using inet_ntoa
 *	Emanuel A. Alongi   27-Jul-1993	    Modifications to encode/decode R5,
 *					    VERSION4 request packets. Deleted
 *					    global csi_minimum_version. Now will
 *					    access minimum version dynamically.
 *	Emanuel A. Alongi   09-Feb-1994.    Added prototype for inet_ntoa().
 *					    Flint cleanup.
 */


/*
 * Header Files: 
 */

#include "csi.h"
#include <csi_xdr_xlate.h>
#include <memory.h>
#include "dv_pub.h"
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi_xdr_pri.h"

/*
 * Defines, Typedefs and Structure Definitions: 
 */

/*
 * Global and Static Variable Declarations: 
 */

static char     *st_module = "csi_xlm_response()";
static CSI_XID  st_dup_xid;
static BOOLEAN  st_init_done = FALSE;

/*	Global and Static Prototype Declarations: */
#ifdef DEBUG
#ifndef ADI
char *inet_ntoa(struct in_addr in);
#endif
#endif
/*      Procedure Type Declarations: */

bool_t 
csi_xlm_response (
    XDR *xdrsp,                             /* XDR handle */
    CSI_MSGBUF *bufferp                    /* data description structure */
)
{
    CSI_RESPONSE    *resp;                  /* current VERSION4 packets */
    CSI_V2_RESPONSE *resp_v2;               /* VERSION2 and VERSION3 packets */
    CSI_V1_RESPONSE *resp_v1;               /* VERSION1 packets */
    CSI_V0_RESPONSE *resp_v0;               /* VERSION0 PACKETS */
    register int     part_size = 0;         /* holds size of one structure */
    BOOLEAN          badsize;               /* TRUE if sizing error detected */
    char   	    *net_datap;             /* working pointer into packet */
    CSI_HEADER      *csi_hdr;               /* ptr to packet csi header */
    BOOLEAN          xdr_allocated = FALSE; /* see considerations */
    long	     minimum_version;       /* minimum version supported */

#ifdef DEBUG
    struct in_addr   netaddr;               /* for conversion of inet addr to
                                             * dotted-decimal notation */

    if TRACE
        (CSI_XDR_TRACE_LEVEL)
            cl_trace(st_module,2,
            (unsigned long)xdrsp,
            (unsigned long) bufferp);
#endif /*DEBUG */

    /* get minimum version supported */
    if (dv_get_number(DV_TAG_ACSLS_MIN_VERSION, &minimum_version) !=
		      STATUS_SUCCESS || minimum_version < (long)VERSION0 ||
		       			minimum_version > (long)VERSION_LAST ) {
	minimum_version = (long)VERSION_MINIMUM_SUPPORTED;
    }

    /* clear out the duplicate packet testing structure */
    if (FALSE == st_init_done) {
        memset((char *) &st_dup_xid, '\0', sizeof(CSI_XID));
        st_init_done = TRUE;
    }

    bufferp->packet_status = CSI_PAKSTAT_INITIAL;
    csi_xcur_size = 0;

    /* set pointers to request header for each supported version */
    resp      = (CSI_RESPONSE *) CSI_PAK_NETDATAP(bufferp);
    resp_v2   = (CSI_V2_RESPONSE *) CSI_PAK_NETDATAP(bufferp);
    resp_v1   = (CSI_V1_RESPONSE *) CSI_PAK_NETDATAP(bufferp);
    resp_v0   = (CSI_V0_RESPONSE *) CSI_PAK_NETDATAP(bufferp);
    net_datap = CSI_PAK_NETDATAP(bufferp);
    csi_hdr   = (CSI_HEADER *) net_datap;

    /*
     * Set up "csi_xexp_size" used for validating a packet by its size. If
     * encoding, buffer size is known.  Check to make sure that it is in
     * bounds.  If decoding, don't know what the expected size is until we
     * decode and the caller may even be having xdr do the allocation, so
     * set to MAX_MESSAGE_SIZE.
     */

    if (XDR_DECODE == xdrsp->x_op) 
        csi_xexp_size = MAX_MESSAGE_SIZE;
    else {
        csi_xexp_size = bufferp->size;
        bufferp->service_type = TYPE_LM;
        if (csi_xexp_size > MAX_MESSAGE_SIZE) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return (0);
        }
    }

    /*
     * The data pointer must be valid for encoding xdr operations. If
     * decoding and is null, xdr allocates memory for the data
     */

    if ((CSI_RESPONSE *) NULL == resp) {
        if (XDR_ENCODE == xdrsp->x_op) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return (0);
        }
        else
            xdr_allocated = TRUE;       /* used in macro */
    }

    /* translate request header, must have at least a version0 header */
    part_size = sizeof(CSI_V0_REQUEST_HEADER);
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xreq_hdr(xdrsp, &resp->csi_req_header)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xreq_hdr()", 
	  MMSG(928, "XDR message translation failure")));
        return (0);
    }

    /* size csi response header by version */
    if (resp->csi_req_header.message_header.message_options & EXTENDED) {

        /* check packet version number against minimum version number allowed */
        if ((long)(resp->csi_req_header.message_header.version) <
                                                              minimum_version) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
	      MMSG(924, "Invalid version number %d"),
                        (int) resp->csi_req_header.message_header.version));
            return (0);
        }

        /* size version1 and later */
        switch (resp->csi_req_header.message_header.version) {

            case VERSION4:

                /* use ack packet as dummy */
                csi_xcur_size = (char *) &resp->csi_ack_res.message_status
                                - (char *) &resp->csi_ack_res;
                csi_active_xdr_version_branch = VERSION4;
                break;

            case VERSION3:

                /* use ack packet as dummy */
                csi_xcur_size = (char *) &resp_v2->csi_ack_res.message_status
                                - (char *) &resp_v2->csi_ack_res;
                csi_active_xdr_version_branch = VERSION3;
                break;

            case VERSION2:

                /* use ack packet as dummy */
                csi_xcur_size = (char *) &resp_v2->csi_ack_res.message_status
                    - (char *) &resp_v2->csi_ack_res;
                csi_active_xdr_version_branch = VERSION2;
                break;

            case VERSION1:

                /* use ack packet as dummy */
                csi_xcur_size = (char *) &resp_v1->csi_ack_res.message_status
                    - (char *) &resp_v1->csi_ack_res;
                csi_active_xdr_version_branch = VERSION1;
                break;

            default:

                /* log invalid version number as translation failure */
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xreq_hdr()", 
		  MMSG(924, "Invalid version number %d"),
                    (int) resp->csi_req_header.message_header.version));
                return (0);

        } /* end switch on version */
    }
    else {

        /* see if Version 0 packets are allowed */
        if (minimum_version != (long) VERSION0) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
	      MMSG(924, "Invalid version number %d"), (int) VERSION0));
            return (0);
        }

        /* size version0 using ack packet as dummy */
        csi_xcur_size = (char *) &resp_v0->csi_ack_res.message_status
                        - (char *) &resp_v0->csi_ack_res;
        csi_active_xdr_version_branch = VERSION0;
    }

#ifndef SSI /* the csi code: */

    /* compare this xid to ones on csi connection queue, drop duplicates */
    if (XDR_DECODE == xdrsp->x_op) {

        if (0 == csi_qcmp(csi_lm_qid, &csi_hdr->xid, sizeof(CSI_XID))) {

#ifdef DEBUG
#ifndef ADI /* the debug csi RPC code: */
            memcpy((char *) &netaddr.s_addr, (char *) csi_hdr->xid.addr,
                                                          sizeof(unsigned long));

            /* convert ulong inet addr to dotted-decimal notation for log msg */
            MLOGCSI((STATUS_INVALID_MESSAGE, st_module, CSI_NO_CALLEE, 
	      MMSG(925, "Duplicate packet from Network detected:discarded\n"
	  "Remote Internet address: %s, process-id: %d, sequence number: %lu"),
		inet_ntoa(netaddr), csi_hdr->xid.pid, csi_hdr->xid.seq_num));


#else /*ADI*/ /* the debug csi OSLAN code: */
            MLOGCSI((STATUS_INVALID_MESSAGE, st_module, CSI_NO_CALLEE, 
	      MMSG(926, "Duplicate packet from Network detected:discarded\n"
	      "Adman name:%s, process-id:%d, sequence number:%lu"),
                    csi_hdr->xid.client_name, csi_hdr->xid.pid,
                    csi_hdr->xid.seq_num));

#endif /* ADI */
#endif /*DEBUG*/

            bufferp->packet_status = CSI_PAKSTAT_DUPLICATE_PACKET;
            RETURN_PARTIAL_PACKET(xdrsp, bufferp);
        }
    }

#else /*SSI*/ /* the ssi code: */

    /* throw away duplicate message */
    if (XDR_DECODE == xdrsp->x_op) {
        if (0 == csi_xidcmp(&csi_hdr->xid, &st_dup_xid)) {

#ifdef DEBUG
#ifndef ADI /* the debug ssi RPC code: */
            memcpy((char *) &netaddr.s_addr, (char *) csi_hdr->xid.addr,
                                                          sizeof(unsigned long));

            /* convert ulong inet addr to dotted-decimal notation for log msg */
            MLOGCSI((STATUS_INVALID_MESSAGE, st_module, CSI_NO_CALLEE, 
	      MMSG(925, "Duplicate packet from Network detected:discarded\n"
	  "Remote Internet address: %s, process-id: %d, sequence number: %lu"),
		 inet_ntoa(netaddr), csi_hdr->xid.pid, csi_hdr->xid.seq_num));


#else /* ADI */ /* the debug ssi OSLAN code: */
            MLOGCSI((STATUS_INVALID_MESSAGE, st_module, CSI_NO_CALLEE, 
	      MMSG(926, "Duplicate packet from Network detected:discarded\n"
	      "Adman name:%s, process-id:%d, sequence number:%lu"),
                    csi_hdr->xid.client_name, csi_hdr->xid.pid,
                    csi_hdr->xid.seq_num));

#endif /* ADI */
#endif /*DEBUG*/

            bufferp->packet_status = CSI_PAKSTAT_DUPLICATE_PACKET;
            RETURN_PARTIAL_PACKET(xdrsp, bufferp);
        }
        else 
            /* packet header accepted, test for next packet will be this xid */
            st_dup_xid = csi_hdr->xid;
    }

#endif /*SSI*/

    /*
     * complete translation depending on version number
     * 
     * Note:  response status verified here, but the sizing for it will be
     * corrected in the next level of routines to ensure portability so
     * compiler padding can be counted.  Use ack response as dummy packet.
     */

    if (resp_v0->csi_req_header.message_header.message_options & EXTENDED) {

        switch (resp->csi_req_header.message_header.version) {

	case VERSION4:

            /* translate version 4 response status */
            part_size = sizeof(RESPONSE_STATUS);
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xres_status(xdrsp,&resp->csi_ack_res.message_status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                RETURN_PARTIAL_PACKET(xdrsp, bufferp);
            }
            csi_xcur_size = (char *) &resp->csi_ack_res.message_id
                - (char *) &resp->csi_ack_res;

            /* translate version 4 acknowledge reponse if appropriate */
            if (resp->csi_req_header.message_header.message_options &
                                                                ACKNOWLEDGE) {
                part_size = sizeof(resp->csi_ack_res.message_id);
                badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || 
                    !xdr_u_short(xdrsp, &resp->csi_ack_res.message_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_short()", 
		      MMSG(928, "XDR message translation failure")));
                    RETURN_PARTIAL_PACKET(xdrsp, bufferp);
                }

                /* correct size overall to account for compiler padding */
                csi_xcur_size = sizeof(CSI_ACKNOWLEDGE_RESPONSE);
                RETURN_COMPLETE_PACKET(xdrsp, bufferp);
            }

            /* translate rest of version 4 packet */
            if (!csi_xv4_res(xdrsp, resp))
                RETURN_PARTIAL_PACKET(xdrsp, bufferp);
            break;

        case VERSION3:
        case VERSION2:

            /* translate version 2/3 response status */
            part_size = sizeof(RESPONSE_STATUS);
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xres_status(xdrsp,&resp_v2->csi_ack_res.message_status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                RETURN_PARTIAL_PACKET(xdrsp, bufferp);
            }
            csi_xcur_size = (char *) &resp_v2->csi_ack_res.message_id
                - (char *) &resp_v2->csi_ack_res;

            /* translate version 2/3 acknowledge reponse if appropriate */
            if (resp_v2->csi_req_header.message_header.message_options &
                                                                ACKNOWLEDGE) {
                part_size = sizeof(resp_v2->csi_ack_res.message_id);
                badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || 
                    !xdr_u_short(xdrsp, &resp_v2->csi_ack_res.message_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_short()", 
		      MMSG(928, "XDR message translation failure")));
                    RETURN_PARTIAL_PACKET(xdrsp, bufferp);
                }

                /* correct size overall to account for compiler padding */
                csi_xcur_size = sizeof(CSI_V2_ACKNOWLEDGE_RESPONSE);
                RETURN_COMPLETE_PACKET(xdrsp, bufferp);
            }

            /* translate rest of version 2/3 packet */
            if (!csi_xv2_res(xdrsp, resp_v2))
                RETURN_PARTIAL_PACKET(xdrsp, bufferp);
            break;

        case VERSION1:

            /* translate version 1 response status */
            part_size = sizeof(RESPONSE_STATUS);
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xres_status(xdrsp,&resp_v1->csi_ack_res.message_status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                RETURN_PARTIAL_PACKET(xdrsp, bufferp);
            }
            csi_xcur_size = (char *) &resp_v1->csi_ack_res.message_id
                - (char *) &resp_v1->csi_ack_res;

            /* trnaslate version 1 acknowledge response if appropriate */
            if (resp_v1->csi_req_header.message_header.message_options &
                                                                ACKNOWLEDGE) {
                part_size = sizeof(resp_v1->csi_ack_res.message_id);
                badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || 
                    !xdr_u_short(xdrsp, &resp_v1->csi_ack_res.message_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_short()", 
		      MMSG(928, "XDR message translation failure")));
                    RETURN_PARTIAL_PACKET(xdrsp, bufferp);
                }

                /* correct size overall to account for compiler padding */
                csi_xcur_size = sizeof(CSI_V1_ACKNOWLEDGE_RESPONSE);
                RETURN_COMPLETE_PACKET(xdrsp, bufferp);
            }

            /* translate rest of version 1 packet */
            if (!csi_xv1_res(xdrsp, resp_v1))
                RETURN_PARTIAL_PACKET(xdrsp, bufferp);
            break;

        default:

            /* should never get here, caught above, log invalid version */
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xreq_hdr()", 
	      MMSG(924, "Invalid version number %d"),
                        (int) resp->csi_req_header.message_header.version));
            return (0);

        } /* end switch on version */
    }
    else {

        /* VERSION0 */

        /* translate version 0 response status */
        part_size = sizeof(RESPONSE_STATUS);
        badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xres_status(xdrsp, &resp_v0->csi_ack_res.message_status)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xres_status()",
	      MMSG(928, "XDR message translation failure")));
            RETURN_PARTIAL_PACKET(xdrsp, bufferp);
        }
        csi_xcur_size = (char *) &resp_v0->csi_ack_res.message_id
            - (char *) &resp_v0->csi_ack_res;

        /* translate version 0 acknowledge repsonse if appropriate */
        if (resp_v0->csi_req_header.message_header.message_options &
                                                                ACKNOWLEDGE) {
            part_size = sizeof(resp_v0->csi_ack_res.message_id);
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &resp_v0->csi_ack_res.message_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                RETURN_PARTIAL_PACKET(xdrsp, bufferp);
            }

            /* correct size overall to account for compiler padding */
            csi_xcur_size = sizeof(CSI_V0_ACKNOWLEDGE_RESPONSE);
            RETURN_COMPLETE_PACKET(xdrsp, bufferp);
        }

        /* Version 0 */
        if (!csi_xv0_res(xdrsp, resp_v0))
            RETURN_PARTIAL_PACKET(xdrsp, bufferp);
    }

    RETURN_COMPLETE_PACKET(xdrsp, bufferp);
}
