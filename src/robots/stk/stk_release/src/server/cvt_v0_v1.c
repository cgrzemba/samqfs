#ifndef lint
static char SccsId[] = "@(#) %full_name:	server/csrc/cvt_v0_v1/2.0A %";
#endif
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      lm_cvt_v0_v1
 *
 * Description:
 *
 *   General:
 *
 *      This module will convert a VERSION0 request packet to a
 *      VERSION1 request packet and then return it to the caller.
 *
 *   Specific:
 *
 *     * Copy the data from the VERSION0 packet's IPC_HEADER into the
 *       VERSION1 packet's IPC_HEADER.
 *     * Create a VERSION1 MESSAGE_HEADER by:
 *              a) copying all VERSION0 message header information from
 *                 the request packet to the VERSION1 packet.
 *              b) clear the EXTENDED bit in the VERSION1 packet and
 *                 set version field to VERSION0.
 *              c) set all extended fields in the VERSION1 packet to be
 *                 the default values (ie: lock_id = NO_LOCK_ID,
 *                 version = VERSION0).
 *      * Copy the remainder of the request packet to the resulting packet.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_PROCESS_FAILURE
 *      STATUS_INCVALID_COMMAND
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
 *      This module assumes that VERSION0 packets are arriving
 *      and does not validate this fact.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      H. I. Grapek    16-Jun-1993     Original.
 *      H. I. Grapek    06-Sep-1993     Fixed pkt size problem
 *                                      ignore responses from rp's
 *                                      fixed byte-count.
 *      H. I. Grapek    03-Oct-1993     Rewrote for RELEASE3.
 *      H. I. Grapek    25-Oct-1993     Added start and idle functionality.
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
#include "acslm.h"
#include "v0_structs.h"
#include "v1_structs.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char             module_string[] = "lm_cvt_v0_v1";
static ALIGNED_BYTES    result_bfr[MAX_MESSAGE_BLOCK];     /* results */

/*
 * Procedure declarations
 */
static void st_audit(V0_REQUEST_TYPE *v0_req_ptr,
		     V1_REQUEST_TYPE *v1_req_ptr, int *byte_count);
static void st_cancel(V0_REQUEST_TYPE *v0_req_ptr,
		      V1_REQUEST_TYPE *v1_req_ptr, int *byte_count);
static void st_dismount(V0_REQUEST_TYPE *v0_req_ptr,
			V1_REQUEST_TYPE *v1_req_ptr, int *byte_count);
static void st_eject(V0_REQUEST_TYPE *v0_req_ptr,
		     V1_REQUEST_TYPE *v1_req_ptr, int *byte_count);
static void st_enter(V0_REQUEST_TYPE *v0_req_ptr,
		     V1_REQUEST_TYPE *v1_req_ptr, int *byte_count);
static void st_idle(int *byte_count);
static void st_mount(V0_REQUEST_TYPE *v0_req_ptr,
		     V1_REQUEST_TYPE *v1_req_ptr, int *byte_count);
static void st_query(V0_REQUEST_TYPE *v0_req_ptr,
		     V1_REQUEST_TYPE *v1_req_ptr, int *byte_count);
static void st_start(int *byte_count);
static void st_vary(V0_REQUEST_TYPE *v0_req_ptr,
		    V1_REQUEST_TYPE *v1_req_ptr, int *byte_count);



STATUS 
lm_cvt_v0_v1 (
    char *request_ptr,           /* acslm input buffer area */
    int *byte_count            /* number of bytes in request. */
)
{
    V0_MESSAGE_HEADER   *v0_msg_hdr_ptr;  /* ptr to V0_MESSAGE_HEADER fields */
    V0_REQUEST_TYPE     *v0_req_ptr;      /* v0 pointer to request type msg's */
    V1_REQUEST_TYPE     *v1_req_ptr;      /* pointer to request type msg's */
    MESSAGE_HEADER      *v1_msg_hdr_ptr;  /* ptr to V1 MESSAGE_HEADER fields */
    char                *result_ptr;      /* ptr to result buffer area */

#ifdef DEBUG
    if TRACE (0)
        cl_trace(module_string, 2, (unsigned long) request_ptr,
                (unsigned long) byte_count);
#endif

    /* Check the sanity of incomming request */
    CL_ASSERT(module_string, ((request_ptr) && (byte_count))); 

    MLOGDEBUG(0,(MMSG(863, "%s: here..."), module_string));

    /* zero the result buffer out */
    memset((char *) result_bfr, 0, sizeof(result_bfr));

    /* set up some generic pointers */
    v0_req_ptr = (V0_REQUEST_TYPE *) request_ptr;       /* incoming packet */
    result_ptr = (char *) result_bfr;                   /* result's data area */
    v1_req_ptr = (V1_REQUEST_TYPE *) result_ptr;        /* result pointer */

    /* copy ipc header (same between versions) */
    v1_req_ptr->generic_request.ipc_header =
        v0_req_ptr->generic_request.ipc_header;

    /* copy message header */
    v1_msg_hdr_ptr = &(v1_req_ptr->generic_request.message_header);
    v0_msg_hdr_ptr = &(v0_req_ptr->generic_request.message_header);

    v1_msg_hdr_ptr->packet_id       = v0_msg_hdr_ptr->packet_id;
    v1_msg_hdr_ptr->command         = v0_msg_hdr_ptr->command;
    v1_msg_hdr_ptr->message_options = v0_msg_hdr_ptr->message_options;
    v1_msg_hdr_ptr->version         = VERSION0;
    v1_msg_hdr_ptr->lock_id         = NO_LOCK_ID;

    /* 
     * Calculate the number of bytes (for this command) from the
     * beginning to the fixed portion of the release 1 packet.
     */

    switch(v1_msg_hdr_ptr->command)
    {
        case COMMAND_AUDIT:
            st_audit(v0_req_ptr, v1_req_ptr, byte_count);
            break;

        case COMMAND_CANCEL:
            st_cancel(v0_req_ptr, v1_req_ptr, byte_count);
            break;

        case COMMAND_DISMOUNT:
            st_dismount(v0_req_ptr, v1_req_ptr, byte_count);
            break;

        case COMMAND_EJECT:
            st_eject(v0_req_ptr, v1_req_ptr, byte_count);
            break;

        case COMMAND_ENTER:
            st_enter(v0_req_ptr, v1_req_ptr, byte_count);
            break;

        case COMMAND_IDLE:
            st_idle(byte_count);
            break;

        case COMMAND_MOUNT:
            st_mount(v0_req_ptr, v1_req_ptr, byte_count);
            break;

        case COMMAND_QUERY:
            st_query(v0_req_ptr, v1_req_ptr, byte_count);
            break;

        case COMMAND_START:
            st_start(byte_count);
            break;

        case COMMAND_VARY:
            st_vary(v0_req_ptr, v1_req_ptr, byte_count);
            break;

        default:
            return (STATUS_INVALID_COMMAND);

    } /* end switch on command */

    /* stuff calculated byte count into the resulting request pointer */
    v1_req_ptr->generic_request.ipc_header.byte_count = *byte_count;

    /* return the resulting VERSION1 packet */
    memcpy(request_ptr, result_ptr, *byte_count);
    return (STATUS_SUCCESS);
}

/*
 * Name:
 *
 *      st_audit()
 *
 * Description:
 *
 *      This module converts an audit request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_audit (
    V0_REQUEST_TYPE *v0_req_ptr,    /* V0 pointer to request type msg's */
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V0_AUDIT_REQUEST    *v0_aup;        /* VERSION0 audit request */
    V1_AUDIT_REQUEST    *aup;           /* VERSION1 audit request */
    char                *from_ident;    /* VERSION0 identifier  pointer */
    char                *to_ident;      /* VERSION1 identifier  pointer */
    int                 copy_size;      /* number of bytes to copy */

    aup = (V1_AUDIT_REQUEST *)v1_req_ptr;
    v0_aup = (V0_AUDIT_REQUEST *)v0_req_ptr;

    /* copy fixed portion */
    aup->cap_id = v0_aup->cap_id;
    aup->type   = v0_aup->type;
    aup->count  = v0_aup->count;

    /* copy variable portion */
    from_ident = (char *) &v0_aup->identifier;
    to_ident   = (char *) &aup->identifier;
    copy_size  = (*byte_count - ((char *)&v0_aup->identifier - (char*)v0_aup));
    memcpy(to_ident, from_ident, copy_size);

    /* calculate byte count from new request pointer */
    *byte_count = (copy_size + ( (char*)&aup->identifier - (char*)aup ));

    return;
}
/*
 * Name:
 *
 *      st_cancel()
 *
 * Description:
 *
 *      This module converts a cancel request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_cancel (
    V0_REQUEST_TYPE *v0_req_ptr,    /* V0 pointer to request type msg's */
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V0_CANCEL_REQUEST   *v0_cp;         /* VERSION0 cancel request */
    V1_CANCEL_REQUEST   *cp;            /* VERSION1 cancel request */

    cp    = (V1_CANCEL_REQUEST *)v1_req_ptr;
    v0_cp = (V0_CANCEL_REQUEST *)v0_req_ptr;

    /* copy fixed portion */
    cp->request = v0_cp->request;

    /* copy NO variable portion */
    *byte_count = sizeof(V1_CANCEL_REQUEST);

    return;
}
/*
 * Name:
 *
 *      st_dismount()
 *
 * Description:
 *
 *      This module converts a dismount request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_dismount (
    V0_REQUEST_TYPE *v0_req_ptr,    /* V0 pointer to request type msg's */
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V0_DISMOUNT_REQUEST *v0_dp;         /* VERSION0 dismount request */
    V1_DISMOUNT_REQUEST *dp;            /* VERSION1 dismount request */
    char                *from_ident;    /* VERSION0 identifier  pointer */
    char                *to_ident;      /* VERSION1 identifier  pointer */
    int                 copy_size;      /* number of bytes to copy */

    dp    = (V1_DISMOUNT_REQUEST *)v1_req_ptr;
    v0_dp = (V0_DISMOUNT_REQUEST *)v0_req_ptr;

    /* copy fixed portion */
    dp->vol_id = v0_dp->vol_id;

    /* copy variable portion */
    from_ident = (char *) &v0_dp->drive_id;
    to_ident   = (char *) &dp->drive_id;
    copy_size  = (*byte_count - ((char *) &v0_dp->drive_id - (char *)v0_dp));
    memcpy(to_ident, from_ident, copy_size);

    /* calculate byte count from new request pointer */
    *byte_count = (copy_size + ((char *) &dp->drive_id - (char *)dp));

    return;
}
/*
 * Name:
 *
 *      st_eject()
 *
 * Description:
 *
 *      This module converts an eject request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_eject (
    V0_REQUEST_TYPE *v0_req_ptr,    /* V0 pointer to request type msg's */
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V0_EJECT_REQUEST    *v0_ejp;        /* VERSION0 eject request */
    V1_EJECT_REQUEST    *ejp;           /* VERSION1 eject request */
    char                *from_ident;    /* VERSION0 identifier  pointer */
    char                *to_ident;      /* VERSION1 identifier  pointer */
    int                 copy_size;      /* number of bytes to copy */

    ejp    = (V1_EJECT_REQUEST *)v1_req_ptr;
    v0_ejp = (V0_EJECT_REQUEST *)v0_req_ptr;

    /* copy fixed portion */
    ejp->cap_id = v0_ejp->cap_id;
    ejp->count  = v0_ejp->count;

    /* copy variable portion */
    from_ident = (char *) &v0_ejp->vol_id[0];
    to_ident   = (char *) &ejp->vol_id[0];
    copy_size  = (*byte_count - ((char *)&v0_ejp->vol_id[0] - (char *)v0_ejp));
    memcpy(to_ident, from_ident, copy_size);

    /* calculate byte count from new request pointer */
    *byte_count = (copy_size + ((char *)&ejp->vol_id[0] - (char *)ejp));

    return;
}
/*
 * Name:
 *
 *      st_enter()
 *
 * Description:
 *
 *      This module converts an enter request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_enter (
    V0_REQUEST_TYPE *v0_req_ptr,    /* V0 pointer to request type msg's */
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V0_ENTER_REQUEST    *v0_enp;        /* VERSION0 enter request */
    V1_ENTER_REQUEST    *enp;           /* VERSION1 enter request */

    enp    = (V1_ENTER_REQUEST *)v1_req_ptr;
    v0_enp = (V0_ENTER_REQUEST *)v0_req_ptr;
    
    /* copy fixed portion */
    enp->cap_id = v0_enp->cap_id;

    /* copy NO variable portion */
    *byte_count = sizeof(ENTER_REQUEST);
    
    return;
}
/*
 * Name:
 *
 *      st_idle()
 *
 * Description:
 *
 *      This module converts an idle request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_idle (
    int *byte_count    /* number of bytes in request. */
)
{


    /* copy NO fixed or variable portion */
    *byte_count = sizeof(V1_IDLE_REQUEST);

    return;
}
/*
 * Name:
 *
 *      st_mount()
 *
 * Description:
 *
 *      This module converts a mount request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_mount (
    V0_REQUEST_TYPE *v0_req_ptr,    /* V0 pointer to request type msg's */
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V0_MOUNT_REQUEST    *v0_mp;         /* VERSION0 mount request */
    V1_MOUNT_REQUEST    *mp;            /* VERSION1 mount request */
    char                *from_ident;    /* VERSION0 identifier  pointer */
    char                *to_ident;      /* VERSION1 identifier  pointer */
    int                 copy_size;      /* number of bytes to copy */

    mp    = (V1_MOUNT_REQUEST *)v1_req_ptr;
    v0_mp = (V0_MOUNT_REQUEST *)v0_req_ptr;

    /* copy fixed portion */
    mp->vol_id = v0_mp->vol_id;
    mp->count  = v0_mp->count;

    /* copy variable portion */
    from_ident = (char *) &v0_mp->drive_id[0];
    to_ident   = (char *) &mp->drive_id[0];
    copy_size = (*byte_count - ((char *)&v0_mp->drive_id[0] - (char *)v0_mp));
    memcpy(to_ident, from_ident, copy_size);

    /* calculate byte count from new request pointer */
    *byte_count = (copy_size + ((char *)&mp->drive_id[0] - (char*)mp));

    return;
}
/*
 * Name:
 *
 *      st_query()
 *
 * Description:
 *
 *      This module converts a query request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_query (
    V0_REQUEST_TYPE *v0_req_ptr,    /* V0 pointer to request type msg's */
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V0_QUERY_REQUEST    *v0_qp;         /* VERSION0 query request */
    V1_QUERY_REQUEST    *qp;            /* VERSION1 query request */
    char                *from_ident;    /* VERSION0 identifier  pointer */
    char                *to_ident;      /* VERSION1 identifier  pointer */
    int                 copy_size;      /* number of bytes to copy */

    qp    = (V1_QUERY_REQUEST *)v1_req_ptr;
    v0_qp = (V0_QUERY_REQUEST *)v0_req_ptr;
    
    /* copy fixed portion */
    qp->type  = v0_qp->type;
    qp->count = v0_qp->count;

    /* copy variable portion */
    from_ident = (char *)&v0_qp->identifier;
    to_ident   = (char *) &qp->identifier;
    copy_size  = (*byte_count - ((char *)&v0_qp->identifier - (char *)v0_qp));
    memcpy(to_ident, from_ident, copy_size);

    /* calculate byte count from new request pointer */
    *byte_count = (copy_size + ( (char*)&qp->identifier - (char*)qp ));

    return;
}
/*
 * Name:
 *
 *      st_start()
 *
 * Description:
 *
 *      This module converts a start request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_start (
    int *byte_count    /* number of bytes in request. */
)
{

    /* copy NO fixed or variable portion */
    *byte_count = sizeof(V1_START_REQUEST);

    return;
}
/*
 * Name:
 *
 *      st_vary()
 *
 * Description:
 *
 *      This module converts a vary request from VERSION0 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      v1_req_ptr        
 *      byte_count      
 */
static void 
st_vary (
    V0_REQUEST_TYPE *v0_req_ptr,    /* V0 pointer to request type msg's */
    V1_REQUEST_TYPE *v1_req_ptr,    /* V1 pointer to request type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V0_VARY_REQUEST     *v0_vp;         /* VERSION0 vary request */
    V1_VARY_REQUEST     *vp;            /* VERSION1 vary request */
    char                *from_ident;    /* VERSION0 identifier  pointer */
    char                *to_ident;      /* VERSION1 identifier  pointer */
    int                 copy_size;      /* number of bytes to copy */

    vp    = (V1_VARY_REQUEST *)v1_req_ptr;
    v0_vp = (V0_VARY_REQUEST *)v0_req_ptr;

    /* copy fixed portion */
    vp->state = v0_vp->state;
    vp->type  = v0_vp->type;
    vp->count = v0_vp->count;

    /* copy variable portion */
    from_ident = (char *) &v0_vp->identifier;
    to_ident   = (char *) &vp->identifier;
    copy_size  = (*byte_count - ((char *)&v0_vp->identifier - (char *)v0_vp));
    memcpy(to_ident, from_ident, copy_size);

    /* calculate byte count from new request pointer */
    *byte_count = (copy_size + ((char*)&vp->identifier - (char *)vp));

    return;
}
