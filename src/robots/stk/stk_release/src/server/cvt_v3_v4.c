#ifndef lint
static char SccsId[] = "@(#) %full_name:	server/csrc/cvt_v3_v4/2.0A %";
#endif

/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      lm_cvt_v3_v4
 *
 * Description:
 *
 *      This routine will convert a VERSION3 packet to a VERSION4 packet.
 *      It is one of the first things done to a packet before being
 *      processed by the ACSLM.  
 *
 *      The ACSLM will only deal with the most current version of the requests.
 *
 *      o The steps taken to change a VERSION3 packet to a VERSION4 packet are
 *        as follows:
 *
 *          * check the sanity of the incoming packet.
 *          * convert the packet
 *          * All data (IPC headers, message headers, response_status,
 *            variable portion, etc) from the VERSION3 packet gets 
 *            copied into the VERSION4 packet.
 *          * If the request is a mount scratch request, add ALL_MEDIA_TYPE
 *	      to the media_type field.
 *	    * If the request is a query request, copy the request into the new
 *	      query request structures.  If the type is TYPE_MOUNT_SCRATCH,
 *	      set the media type count to zero.
 *
 *      o return the new packet and it's length to the input process.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_INVALID_COMMAND
 *	Others as returned by routines st_query and st_mount_scr.
 *
 * Parameters:
 *	*v3_req_ptr	pointer to ACSLM input buffer area
 *	*byte_count     Used as both input and output parameter.
 *			Input Parameter:	Number of bytes in V3 request.
 *			Output Paramater:	Number of bytes in V4 request.
 *
 * Implicit Inputs:	NONE
 *
 * Implicit Outputs:	NONE
 *
 * Considerations:	This conversion routine is called before the request
 *	is verified as correct by routine lm_req_valid.  In the course of the
 *	conversion, if the query command is incorrect, or the query type is 
 *	incorrect, or the mount scratch request does not contain one drive
 *	identifier, the appropiate error status codes are returned.
 *
 * Module Test Plan:	NONE
 *
 * Revision History:
 *
 *      Janet Borzuchowski	30-Aug-1993.    Original. R5.0 Mixed Media.
 *	Janet Borzuchowski	12-Nov-1993	R5.0 Mixed Media-- Change to
 *				    only one media type in request; convert
 *				    downlevel packet media type to
 *				    ALL_MEDIA_TYPE.
 *	Janet Borzuchowski	14-Jan-1994	R5.0 BR#20 and BR#50-- Compute
 *				    new size of the query server request for 
 *				    V4.  Even thought the V4 query server 
 *				    request does not have a variable portion, 
 *				    the count was removed from the fixed 
 *				    portion, and thus the size is different.
 *	Andy Steere		16-Mar-1994	R5.0 BR#201 - change default 
 *				    from ANY_MEDIA_TYPE to ALL_MEDIA_TYPE.
 *
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
#include "v3_structs.h"
#include "acslm.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char             module_string[] = "lm_cvt_v3_v4";
static ALIGNED_BYTES    result_bfr[MAX_MESSAGE_BLOCK];     /* results */

/*
 * Procedure declarations
 */
static STATUS st_query(REQUEST_TYPE 		*req_ptr, 
		       V3_REQUEST_TYPE 		*v3_req_ptr, 
		       int 			*byte_count);

static STATUS st_mount_scr(REQUEST_TYPE 	*req_ptr, 
			   V3_REQUEST_TYPE 	*v3_req_ptr, 
			   int 			*byte_count);


STATUS 
lm_cvt_v3_v4(
    V3_REQUEST_TYPE *v3_req_ptr,       /* acslm input buffer area */
    int *byte_count                    /* number of bytes in request. */
)
{
    REQUEST_TYPE     	*req_ptr;      /* V4 pointer to request type msg's */
    char                *result_ptr;   /* ptr to result buffer area        */
    STATUS		status;	

#ifdef DEBUG
    if TRACE (0)
        cl_trace(module_string, 2, (unsigned long) v3_req_ptr,
                (unsigned long) byte_count);
#endif

    /* Check the sanity of incoming request */
    CL_ASSERT(module_string, v3_req_ptr && *byte_count); 

    MLOGDEBUG(0,(MMSG(869, "%s: here... byte_cnt: %d"), module_string, *byte_count));

    /* zero the result buffer out */
    memset((char *) result_bfr, '\0', sizeof(result_bfr));

    /* set up some generic pointers */
    result_ptr = (char *) result_bfr;                   /* result's data area */
    req_ptr = (REQUEST_TYPE *) result_ptr;              /* result pointer */

    /* copy ipc header (same between versions) */
    req_ptr->generic_request.ipc_header = 
        v3_req_ptr->generic_request.ipc_header;

    /* copy message header (same between versions) */
    req_ptr->generic_request.message_header = 
        v3_req_ptr->generic_request.message_header;

    /*
     * copy the remainder of the request packet
     * Calculate the number of bytes (for this command) from the beginning
     * to the fixed portion of the VERSION3 packet.
     */
    switch (req_ptr->generic_request.message_header.command) {

        case COMMAND_MOUNT_SCRATCH:
            if ((status = st_mount_scr(req_ptr, v3_req_ptr, byte_count)) !=
		 STATUS_SUCCESS){
                MLOGU((module_string, "st_mount_scr", status,
                     MNOMSG));
                return(status);
            }
	    break;

        case COMMAND_QUERY:
            if ((status = st_query(req_ptr, v3_req_ptr, byte_count)) != 
		 STATUS_SUCCESS){
                MLOGU((module_string, "st_query", status,
                     MNOMSG));
                return(status);
	    }
	    break;

        case COMMAND_AUDIT:
        case COMMAND_CANCEL:
        case COMMAND_CLEAR_LOCK:
        case COMMAND_DEFINE_POOL:
        case COMMAND_DELETE_POOL:
        case COMMAND_DISMOUNT:
        case COMMAND_EJECT:
        case COMMAND_ENTER:     
        case COMMAND_IDLE:
        case COMMAND_LOCK:
        case COMMAND_MOUNT:
        case COMMAND_QUERY_LOCK:
        case COMMAND_SET_CAP:
        case COMMAND_SET_CLEAN:
        case COMMAND_SET_OWNER:
        case COMMAND_SET_SCRATCH:
        case COMMAND_START:
        case COMMAND_UNLOCK:
        case COMMAND_VARY:
            /* commands did't change from VERSION3 to VERSION4 */
            return (STATUS_SUCCESS);

        default:
            return(STATUS_INVALID_COMMAND);
    } 

    /* stuff calculated byte count into the resulting request pointer */
    req_ptr->generic_request.ipc_header.byte_count = *byte_count;

    /* return the resulting VERSION4 pkt */
    memcpy((char *) v3_req_ptr, result_ptr, *byte_count);
    return (STATUS_SUCCESS);
}

/*
 * Name:
 *
 *      st_query
 *
 * Description:
 *
 *      This routine converts a query request from VERSION3 to VERSION4
 *
 * Return Values:     
 *	STATUS_SUCCESS
 *	STATUS_INVALID_TYPE
 *
 * Parameters:
 *	*req_ptr	V4 pointer to converted query request
 *	*v3_req_ptr	V3 pointer to query request from input buffer.
 *	*byte_count	Used as input and output parameter.
 *			Input Parameter:	Number of bytes in V3 request.
 *						This becomes number of bytes in
 *						the V4 request if unchanged.
 *			Output Parameter:	Number of bytes in converted
 *						V4 request.
 *
 * Implicit Inputs:  	NONE
 *
 * Implicit Outputs:  	result_bfr
 *
 * Considerations:	NONE
 */
static STATUS
st_query (
    REQUEST_TYPE 	*req_ptr,     /* V4 pointer to request type msg's */
    V3_REQUEST_TYPE 	*v3_req_ptr,  /* V3 pointer to request type msg's */
    int 		*byte_count   /* number of bytes in request. */
)
{
    QUERY_REQUEST       *qp;          /* VERSION4 query request */
    V3_QUERY_REQUEST    *v3_qp;       /* VERSION3 query request */
    int			i;	      /* loop index */
    int			count;	      /* Number of identifiers copied from v3 
				       * to v4.  MAX_ID is the max number.
				       */
				       

    qp = (QUERY_REQUEST *) req_ptr;
    v3_qp = (V3_QUERY_REQUEST *) v3_req_ptr;

    /* copy fixed portion */
    qp->type = v3_qp->type;

    /* Check if count is too big; if so, set count to MAX_ID for the copy. 
     * The check for this occurs after the structure is converted, at which
     * time, an error message is generated.  In this conversion routine, only
     * MAX_ID identifiers are copied to new v4 criteria structures.
     */
    count = (int) v3_qp->count;
    if (count > MAX_ID) {
	count = MAX_ID;
    }

    /* copy variable portion */
    switch (v3_qp->type) {
      case TYPE_ACS:
	/* Copy count to new ACS criteria structure */
	qp->select_criteria.acs_criteria.acs_count = v3_qp->count;

	/* Copy identifiers to new ACS criteria structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.acs_criteria.acs_id[i] = 
	        v3_qp->identifier.acs_id[i];
	}

        /* Calculate byte count. */
        *byte_count = 
	    ((char *)&qp->select_criteria.acs_criteria.acs_id[v3_qp->count]
	    - (char *)qp);
	break;

      case TYPE_CAP:
	/* Copy count to new cap criteria structure. */
	qp->select_criteria.cap_criteria.cap_count = v3_qp->count;

	/* Copy identifiers to new cap criteria.structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.cap_criteria.cap_id[i] = 
	        v3_qp->identifier.cap_id[i];
	}

        /* Calculate byte count. */
        *byte_count = 
	    ((char *)&qp->select_criteria.cap_criteria.cap_id[v3_qp->count]
	    - (char *)qp);
	break;

      case TYPE_CLEAN:
      case TYPE_MOUNT:
      case TYPE_VOLUME:
	/* These query types all use the new volume criteria structure. 
	 * Copy count to new volume criteria structure. 
	 */
	qp->select_criteria.vol_criteria.volume_count = v3_qp->count;

	/* Copy identifiers to new volume criteria structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.vol_criteria.volume_id[i] = 
	        v3_qp->identifier.vol_id[i];
	}
        /* Calculate byte count. */
        *byte_count = 
	    ((char *)&qp->select_criteria.vol_criteria.volume_id[v3_qp->count]
	    - (char *)qp);
	break;
	
      case TYPE_DRIVE:
	/* Copy count to new drive criteria structure. */
	qp->select_criteria.drive_criteria.drive_count = v3_qp->count;

	/* Copy identifiers to new drive criteria structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.drive_criteria.drive_id[i] = 
	        v3_qp->identifier.drive_id[i];
	}

        /* Calculate byte count. */
        *byte_count = 
	    ((char *)&qp->select_criteria.drive_criteria.drive_id[v3_qp->count]
	    - (char *)qp);
	break;
	
      case TYPE_LSM:
	/* Copy count to new LSM criteria structure. */
	qp->select_criteria.lsm_criteria.lsm_count = v3_qp->count;

	/* Copy identifiers to new LSM criteria structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.lsm_criteria.lsm_id[i] = 
	        v3_qp->identifier.lsm_id[i];
	}

        /* Calculate byte count. */
        *byte_count = 
	    ((char *)&qp->select_criteria.lsm_criteria.lsm_id[v3_qp->count]
	    - (char *)qp);
	break;
	
      case TYPE_MOUNT_SCRATCH:
	/* Copy count to new mount scratch criteria structure. 
	 * Note that this down level count is the number of pool identifiers. 
	 */
	qp->select_criteria.mount_scratch_criteria.pool_count = v3_qp->count;

	/* Copy pool identifiers to new mount scratch criteria structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.mount_scratch_criteria.pool_id[i] = 
	        v3_qp->identifier.pool_id[i];
	}

	/* Set the media type to ALL_MEDIA_TYPE. This causes all drives 
	 * compatible to all the media types in the scratch pool to be returned.
	 */
	qp->select_criteria.mount_scratch_criteria.media_type = ALL_MEDIA_TYPE;

        /* Calculate byte count. */ 
        *byte_count = 
	    ((char *)&qp->select_criteria.mount_scratch_criteria.
	    pool_id[v3_qp->count] - (char *)qp);
	break;
	
      case TYPE_POOL:
      case TYPE_SCRATCH:
	/* These query types use the new pool criteria structure. 
	 * Copy count to the new pool criteria structure. 
	 */
	qp->select_criteria.pool_criteria.pool_count = v3_qp->count;

	/* Copy identifiers to new pool criteria structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.pool_criteria.pool_id[i] = 
	        v3_qp->identifier.pool_id[i];
	}

        /* Calculate byte count. */
        *byte_count = 
	    ((char *)&qp->select_criteria.pool_criteria.pool_id[v3_qp->count]
	    - (char *)qp);
	break;

      case TYPE_PORT:
	/* Copy count to new port criteria structure. */
	qp->select_criteria.port_criteria.port_count = v3_qp->count;

	/* Copy identifiers to new port criteria structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.port_criteria.port_id[i] = 
	        v3_qp->identifier.port_id[i];
	}

        /* Calculate byte count. */
        *byte_count = 
	    ((char *)&qp->select_criteria.port_criteria.port_id[v3_qp->count]
	    - (char *)qp);
	break;
	
      case TYPE_REQUEST:
	/* Copy count to new request criteria structure. */
	qp->select_criteria.request_criteria.request_count = v3_qp->count;

	/* Copy identifiers to new request criteria structure. */
	for (i=0; i < count; i++) {
	    qp->select_criteria.request_criteria.request_id[i] = 
	        v3_qp->identifier.request[i];
	}

        /* Calculate byte count. */
        *byte_count = 
	    ((char *)&qp->select_criteria.request_criteria.request_id[v3_qp->count]
	    - (char *)qp);
	break;
	
      case TYPE_SERVER:
	/* Note that the query server request uses no criteria structures, only
	 * sets type to TYPE_SERVER, therefore, nothing to convert.
	 * Recalculate byte count, since count was removed from fixed portion.
	 */
        *byte_count = 
	    ((char *)&qp->select_criteria - (char *)qp);
	break;
	
      default:
	/* Note that the type is also checked in routine lm_req_valid for
	 * packets already in V4 format.
	 */
	return (STATUS_INVALID_TYPE);
	
    }   /* End switch */

    return(STATUS_SUCCESS);
}

/*
 * Name:
 *
 *      st_mount_scr
 *
 * Description:
 *
 *      This routine converts a mount scratch request from VERSION3 to VERSION4
 *
 * Return Values:
 *	STATUS_SUCCESS
 *	STATUS_COUNT_TOO_LARGE
 *	STATUS_COUNT_TOO_SMALL
 *
 * Parameters:
 *	*req_ptr	V4 pointer to converted query request
 *	*v3_req_ptr	V3 pointer to query request from input buffer.
 *	*byte_count	Used as input and output parameter.
 *			Input Parameter:	Number of bytes in V3 request.
 *						This becomes number of bytes in
 *						the V4 request if unchanged.
 *			Output Parameter:	Number of bytes in converted
 *						V4 request.
 *
 * Implicit Inputs:  	NONE
 *
 * Implicit Outputs:  	NONE
 *
 * Considerations:	NONE
 *
 */
STATUS
st_mount_scr (
    REQUEST_TYPE    *req_ptr,          /* V4 pointer to request type msg's */
    V3_REQUEST_TYPE *v3_req_ptr,       /* V3 pointer to request type msg's */
    int             *byte_count        /* number of bytes in request. */
)
{

    MOUNT_SCRATCH_REQUEST    *mscp;    /* VERSION4 mount scratch request */
    V3_MOUNT_SCRATCH_REQUEST *v3_mscp; /* VERSION3 mount scratch request */

    mscp = (MOUNT_SCRATCH_REQUEST *) req_ptr;
    v3_mscp = (V3_MOUNT_SCRATCH_REQUEST *) v3_req_ptr;

    /* Copy fixed portion */
    mscp->pool_id = v3_mscp->pool_id;
    mscp->count = v3_mscp->count;

    /* Set media type to ALL_MEDIA_TYPE.  This selects a compatible cartridge
     * for down level clients.
     */
    mscp->media_type = ALL_MEDIA_TYPE;

    /* Copy variable portion. 
     * Note that for V4, count may only be set to 1. 
     */

    if (v3_mscp->count < 1){
	return(STATUS_COUNT_TOO_SMALL);
    }
    else if (v3_mscp->count > 1){
	return(STATUS_COUNT_TOO_LARGE);
    }
    else {
        mscp->drive_id[0] = v3_mscp->drive_id[0];
    }

    /* Calculate byte count. */
    *byte_count = ((char *)&mscp->drive_id[mscp->count] - (char *)mscp);

    return(STATUS_SUCCESS);
}
