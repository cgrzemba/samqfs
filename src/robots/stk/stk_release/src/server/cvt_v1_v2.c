#ifndef lint
static char SccsId[] = "@(#) %full_name:	server/csrc/cvt_v1_v2/2.0A %";
#endif

/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      lm_cvt_v1_v2
 *
 * Description:
 *
 *      This routine will convert a VERSION1 packet to a VERSION2 packet.
 *      It is one of the first things done to a packet before being
 *      processed by the ACSLM.  
 *
 *      The ACSLM will only deal with the must current version of the requests.
 *
 *      o The steps taken to change a VERSION1 packet to a VERSION2 packet are
 *        as follows:
 *
 *          * check the sanity of the incoming packet.
 *          * convert the packet
 *          * All data (IPC headers, message headers, response_status,
 *            variable portion, etc) from the VERSION1 packet gets 
 *            copied into the VERSION2 packet.
 *          * If the request has a CAP identifier in the variable 
 *            portion (Enter, eject, vary, etc) the CAPid is 
 *            modified to reflect the the new extended CAP 
 *            structure.  The following rules are taken:
 *              + the CAP number in the new CAPid is set to MIN_CAP.
 *              + if it is a set_cap pkt, the new CAPid is 
 *                setup as above, and the cap_mode is set to MODE_SAME.
 *              + The version number in the packet does NOT change.
 *
 *      o Note: The fixed portion as well as the variable portion of the 
 *        pkt needs to be updated.  This may take a noticably long time.
 *
 *      o return the new packet and it's length to the input process.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_INVALID_COMMAND
 *      STATUS_PROCESS_FAILURE
 *
 * Implicit Inputs:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      None
 *
 * Considerations:
 *
 *      Even though vary has a type_cap in the VERSION2 command, it
 *      will never be a part of the VERSION1 command.  Therefore, vary
 *      doesn't change between VERSION1 and VERSION2 in this 
 *      direction
 *
 * Module Test Plan:
 *
 *      None
 *
 * Revision History:
 *
 *      H. I. Grapek            22-Mar-1993.    Original.
 *
 *      H. I. Grapek            02-Oct-1993.    Cleanup, Code review changes.
 *
 *      H. I. Grapek            18-Oct-1993     Fixed lint issues.
 *
 *      D. A. Beidle            13-Nov-1993.    Fixed the following bugs:
 *              BR#367 - Fixed to check for VIRTUAL option in extended_options
 *              field of message_header.
 *              BR#486 - Fixed to check for RANGE option in extended_options
 *              field of message_header.
 *      Alec Sharp              26-Jun-1993     Changed the version 2
 *              structures to have a V2_ prefix, and included v2_structs.h.
 *              This means that we no longer have to worry about changes
 *              to the current structures when converting.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>
#include <string.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "v2_structs.h"
#include "acslm.h"
#include "v1_structs.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char             module_string[] = "lm_cvt_v1_v2";
static ALIGNED_BYTES    result_bfr[MAX_MESSAGE_BLOCK];     /* results */

/*
 * Procedure declarations
 */
static void st_audit(V1_REQUEST_TYPE *v1_req_ptr,
		     V2_REQUEST_TYPE *req_ptr, int *byte_count);
static void st_eject(V1_REQUEST_TYPE *v1_req_ptr,
		     V2_REQUEST_TYPE *req_ptr, int *byte_count);
static void st_enter(V1_REQUEST_TYPE *v1_req_ptr,
		     V2_REQUEST_TYPE *req_ptr, int *byte_count);
static void st_query(V1_REQUEST_TYPE *v1_req_ptr,
		     V2_REQUEST_TYPE *req_ptr, int *byte_count);
static void st_set_cap(V1_REQUEST_TYPE *v1_req_ptr,
		       V2_REQUEST_TYPE *req_ptr, int *byte_count);


STATUS 
lm_cvt_v1_v2 (
    char *request_ptr,                   /* acslm input buffer area */
    int *byte_count                    /* number of bytes in request. */
)
{
    V2_REQUEST_TYPE     *req_ptr;       /* pointer to request type msg's */
    V1_REQUEST_TYPE     *v1_req_ptr;    /* V1 pointer to request type msg's */
    char                *result_ptr;    /* ptr to result buffer area */

#ifdef DEBUG
    if TRACE (0)
        cl_trace(module_string, 2, (unsigned long) request_ptr,
                (unsigned long) byte_count);
#endif

    /* Check the sanity of incomming request */
    CL_ASSERT(module_string, ((request_ptr) && (*byte_count))); 

    MLOGDEBUG(0,(MMSG(869, "%s: here... byte_cnt: %d"), module_string, *byte_count));

    /* zero the result buffer out */
    memset((char *) result_bfr, 0, sizeof(result_bfr));

    /* set up some generic pointers */
    v1_req_ptr = (V1_REQUEST_TYPE *) request_ptr;       /* incomming packet */
    result_ptr = (char *) result_bfr;                   /* result's data area */
    req_ptr = (V2_REQUEST_TYPE *) result_ptr;           /* result pointer */

    /* copy ipc header (same between versions) */
    req_ptr->generic_request.ipc_header = 
        v1_req_ptr->generic_request.ipc_header;

    /* copy message header (same between versions) */
    req_ptr->generic_request.message_header = 
        v1_req_ptr->generic_request.message_header;

    /*
     * copy the remainder of the request packet
     * Calculate the number of bytes (for this command) from the beginning
     * to the fixed portion of the VERSION1 packet.
     */
    switch (req_ptr->generic_request.message_header.command) {

        case COMMAND_AUDIT:
            st_audit(v1_req_ptr, req_ptr, byte_count);
            break;

        case COMMAND_EJECT:
            st_eject(v1_req_ptr, req_ptr, byte_count);
            break;

        case COMMAND_ENTER:     
            st_enter(v1_req_ptr, req_ptr, byte_count);
            break;

        case COMMAND_QUERY:
            st_query(v1_req_ptr, req_ptr, byte_count);
            break;

        case COMMAND_SET_CAP:
            st_set_cap(v1_req_ptr, req_ptr, byte_count);
            break;

        case COMMAND_CANCEL:
        case COMMAND_CLEAR_LOCK:
        case COMMAND_DEFINE_POOL:
        case COMMAND_DELETE_POOL:
        case COMMAND_DISMOUNT:
        case COMMAND_IDLE:
        case COMMAND_LOCK:
        case COMMAND_MOUNT:
        case COMMAND_MOUNT_SCRATCH:
        case COMMAND_QUERY_LOCK:
        case COMMAND_SET_CLEAN:
        case COMMAND_SET_SCRATCH:
        case COMMAND_START:
        case COMMAND_UNLOCK:
        case COMMAND_VARY:
            /* commands don't change From VERSION1 to VERSION2 */
            return (STATUS_SUCCESS);

        default:
            return(STATUS_INVALID_COMMAND);
    } 

    /* stuff calculated byte count into the resulting request pointer */
    req_ptr->generic_request.ipc_header.byte_count = *byte_count;

    /* return the resulting VERSION2 pkt */
    memcpy(request_ptr, result_ptr, *byte_count);
    return (STATUS_SUCCESS);
}

/*
 * Name:
 *
 *      st_audit
 *
 * Description:
 *
 *      This module converts an audit request from VERSION1 to VERSION2
 *
 * Return Values:     None.
 *
 * Implicit Outputs:  None.
 *
 */
static void 
st_audit (
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    V2_REQUEST_TYPE *req_ptr,       /* pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V1_AUDIT_REQUEST    *v1_aup;        /* VERSION1 audit request */
    V2_AUDIT_REQUEST    *aup;           /* VERSION2 audit request */
    char                *from_ident;    /* VERSION1 identifier pointer */
    char                *to_ident;      /* VERSION2 identifier pointer */
    int                 copy_size;      /* number of bytes to copy */

    aup = (V2_AUDIT_REQUEST *) req_ptr;
    v1_aup = (V1_AUDIT_REQUEST *) v1_req_ptr;

    /* copy fixed portion */
    aup->cap_id.lsm_id  = v1_aup->cap_id;
    aup->cap_id.cap     = MIN_CAP;
    aup->type           = v1_aup->type;
    aup->count          = v1_aup->count;

    /* copy variable portion */
    from_ident = (char *) &v1_aup->identifier;
    to_ident   = (char *) &aup->identifier;
    copy_size  = (*byte_count - ((char *)&v1_aup->identifier - (char *)v1_aup));
    memcpy(to_ident, from_ident, copy_size);

    /* calculate byte count of new request pointer */
    *byte_count = (copy_size + ((char *) &aup->identifier - (char *) aup));

    return;
}

/*
 * Name:
 *
 *      st_eject
 *
 * Description:
 *
 *      This module converts an eject request from VERSION1 to VERSION2
 *
 * Return Values:     None.
 *
 * Implicit Outputs:  None.
 *
 */
static void 
st_eject (
    V1_REQUEST_TYPE *v1_req_ptr,   /* V1 pointer to request type msg's */
    V2_REQUEST_TYPE *req_ptr,      /* pointer to request type msg's */
    int *byte_count   /* number of bytes in request. */
)
{
    V1_EJECT_REQUEST     *v1_ejp;       /* VERSION1 eject request */
    V1_EXT_EJECT_REQUEST *v1_xejp;      /* VERSION1 ext_eject request */
    V2_EJECT_REQUEST     *ejp;          /* VERSION2 eject request */
    V2_EXT_EJECT_REQUEST *xejp;         /* VERSION2 ext_eject request */
    char                 *from_ident;   /* VERSION1 identifier pointer */
    char                 *to_ident;     /* VERSION2 identifier pointer */
    int                  copy_size;     /* number of bytes to copy */

    if (RANGE & req_ptr->generic_request.message_header.extended_options) {

        /* Type B, extended (range) eject request */
        xejp = (V2_EXT_EJECT_REQUEST *) req_ptr;
        v1_xejp = (V1_EXT_EJECT_REQUEST *) v1_req_ptr;

        /* copy fixed portion */
        xejp->cap_id.lsm_id     = v1_xejp->cap_id;
        xejp->cap_id.cap        = MIN_CAP;
        xejp->count             = v1_xejp->count;

        /* copy variable portion */
        from_ident = (char *) &v1_xejp->vol_range[0];
        to_ident = (char *) &xejp->vol_range[0];
        copy_size = (*byte_count - ((char *) &v1_xejp->vol_range[0] - 
                (char *) v1_xejp));
        memcpy(to_ident, from_ident, copy_size);
        *byte_count = (copy_size + ((char *) &xejp->vol_range[0] - 
                (char *) xejp));
    }
    else {

        /* Type A (non-range) eject request */
        ejp = (V2_EJECT_REQUEST *) req_ptr;
        v1_ejp = (V1_EJECT_REQUEST *) v1_req_ptr;

        /* copy fixed portion */
        ejp->cap_id.lsm_id      = v1_ejp->cap_id;
        ejp->cap_id.cap         = MIN_CAP;
        ejp->count              = v1_ejp->count;

        /* copy variable portion */
        from_ident = (char *) &v1_ejp->vol_id[0];
        to_ident = (char *) &ejp->vol_id[0];
        copy_size = (*byte_count - ((char *) &v1_ejp->vol_id[0] - 
                (char *) v1_ejp));
        memcpy(to_ident, from_ident, copy_size);
        *byte_count = (copy_size + ((char *) &ejp->vol_id[0] - 
                (char *) ejp));
    }
    return;
}

/*
 * Name:
 *
 *      st_enter
 *
 * Description:
 *
 *      This module converts an enter request from VERSION1 to VERSION2
 *
 * Return Values:     None.
 *
 * Implicit Outputs:  None.
 *
 */
static void 
st_enter (
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    V2_REQUEST_TYPE *req_ptr,       /* pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V1_ENTER_REQUEST    *v1_enp;        /* version1 enter request */
    V1_VENTER_REQUEST   *v1_venp;       /* version1 Venter request */
    V2_ENTER_REQUEST    *enp;           /* version2 enter request */
    V2_VENTER_REQUEST   *venp;          /* version2 venter request */
    char                *from_ident;    /* VERSION1 identifier pointer */
    char                *to_ident;      /* VERSION2 identifier pointer */
    int                 copy_size;      /* number of bytes to copy */

    if (VIRTUAL & req_ptr->generic_request.message_header.extended_options) {

        /* Type B (Virtual) enter request */
        venp = (V2_VENTER_REQUEST *) req_ptr;
        v1_venp = (V1_VENTER_REQUEST *) v1_req_ptr;

        /* copy fixed portion */
        venp->cap_id.lsm_id     = v1_venp->cap_id;
        venp->cap_id.cap        = MIN_CAP;
        venp->count             = v1_venp->count;

        /* copy variable portion */
        from_ident = (char *) &v1_venp->vol_id[0];
        to_ident = (char *) &venp->vol_id[0];
        copy_size = (*byte_count - ((char *) &v1_venp->vol_id[0] - 
                (char *) v1_venp));
        memcpy(to_ident, from_ident, copy_size);
        *byte_count = (copy_size + ((char *) &venp->vol_id[0] - (char *) venp));
    }
    else {

        /* Type A (non-Virtual) enter request */
        enp = (V2_ENTER_REQUEST *) req_ptr;
        v1_enp = (V1_ENTER_REQUEST *) v1_req_ptr;

        /* copy fixed portion */
        enp->cap_id.lsm_id      = v1_enp->cap_id;
        enp->cap_id.cap         = MIN_CAP;

        /* copy NO variable portion */
        *byte_count = sizeof(V2_ENTER_REQUEST);
    }
    return;
}

/*
 * Name:
 *
 *      st_query
 *
 * Description:
 *
 *      This module converts an query request from VERSION1 to VERSION2
 *
 * Return Values:     None.
 *
 * Implicit Outputs:  None.
 *
 */
static void 
st_query (
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    V2_REQUEST_TYPE *req_ptr,       /* pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V2_QUERY_REQUEST    *qp;            /* VERSION2 query request */
    V1_QUERY_REQUEST    *v1_qp;         /* VERSION1 query request */
    char                *from_ident;    /* VERSION1 identifier pointer */
    char                *to_ident;      /* VERSION2 identifier pointer */
    int                 copy_size;      /* number of bytes to copy */
    int                 cntr;           /* miscellaneous counter */

    qp = (V2_QUERY_REQUEST *) req_ptr;
    v1_qp = (V1_QUERY_REQUEST *) v1_req_ptr;

    /* copy fixed portion */
    qp->type    = v1_qp->type;
    qp->count   = v1_qp->count;

    /* copy variable portion */

    if (qp->type == TYPE_CAP) {
        /* loop to copy the new cap id */
        for (cntr = 0; cntr < (int)qp->count; ++cntr) {
            qp->identifier.cap_id[cntr].lsm_id = v1_qp->identifier.cap_id[cntr];
            qp->identifier.cap_id[cntr].cap    = MIN_CAP;
        }

        *byte_count = ((char *)&qp->identifier.cap_id[qp->count] - (char *)qp);
    }
    else {
        /* non cap query, copy the data as is. */
        from_ident = (char *) &v1_qp->identifier;
        to_ident = (char *) &qp->identifier;
        copy_size = (*byte_count - ((char *) &v1_qp->identifier - 
                (char *) v1_qp));
        memcpy(to_ident, from_ident, copy_size);
        *byte_count = (copy_size + ((char *) &qp->identifier - (char *) qp));
    }
    return;
}

/*
 * Name:
 *
 *      st_set_cap
 *
 * Description:
 *
 *      This module converts an set_cap request from VERSION1 to VERSION2
 *
 * Return Values:     None.
 *
 * Implicit Outputs:  None.
 *
 */
static void 
st_set_cap (
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    V2_REQUEST_TYPE *req_ptr,       /* pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V2_SET_CAP_REQUEST  *scp;           /* version2 set_cap request */
    V1_SET_CAP_REQUEST  *v1_scp;        /* version1 set_cap request */
    int                 cntr;           /* miscellaneous counter */
    IDENTIFIER		id;

    scp = (V2_SET_CAP_REQUEST *) req_ptr;
    v1_scp = (V1_SET_CAP_REQUEST *) v1_req_ptr;

    /* copy fixed portion */
    scp->cap_priority = v1_scp->cap_priority;
    scp->cap_mode = CAP_MODE_SAME;
    scp->count = v1_scp->count;

    /* copy variable portion */

    /* loop to copy the new cap id */
    for (cntr = 0; cntr < (int)scp->count; ++cntr) {
        scp->cap_id[cntr].lsm_id.acs = v1_scp->cap_id[cntr].acs;
        scp->cap_id[cntr].lsm_id.lsm = v1_scp->cap_id[cntr].lsm;
        scp->cap_id[cntr].cap        = MIN_CAP;
    }

    *byte_count = ((char *)&scp->cap_id[scp->count] - (char *)scp);

    id.cap_id = scp->cap_id[0];
    MLOGDEBUG(0,(MMSG(870, "%s:st_set_cap: 0th cap_id(%s), byte_count(%d)\n"), 
	    module_string, cl_identifier(TYPE_CAP, &id), *byte_count));

    return;
}
