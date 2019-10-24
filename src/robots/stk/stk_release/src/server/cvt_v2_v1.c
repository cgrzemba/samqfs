#ifndef lint
static char SccsId[] = "@(#) %full_name:	server/csrc/cvt_v2_v1/2.0A %";
#endif

/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      lm_cvt_v2_v1
 *
 * Description:
 *
 *      This routine will convert a VERSION2 packet back to a VERSION1 packet.
 *      It is the last thing done to a V1 packet before being returned to the
 *      requestor.  
 *
 *      o All information will be preserved in the packet during the conversion
 *        with the exception of the extended capid information, which has the
 *        CAP number truncated from the identifier.  In addition, new VERSION2
 *        message and identifier statuses are converted as follows:
 *
 *          STATUS_INCORRECT_LOCKID     ->   STATUS_VARY_DISALLOWED
 *          STATUS_CAP_NOT_IN_LIBRARY   ->   Not converted (CAP is always 0)
 *          STATUS_CAP_OFFLINE          ->   STATUS_CAP_IN_USE
 *          STATUS_INVALID_CAP          ->   Not converted (CAP is always 0)
 *          STATUS_INCORRECT_CAP_MODE   ->   STATUS_CAP_IN_USE
 *          STATUS_INCORRECT_STATE      ->   STATUS_VARY_DISALLOWED
 *          STATUS_VARY_IN_PROGRESS     ->   STATUS_VARY_DISALLOWED
 *
 *      o strip cap_mode from set_cap response (as appropriate).
 *
 *	o For the query mount and query mount scratch responses, only copy
 *	  V1_MAX_ACS_DRIVES and not V2_QU_MAX_DRV_STATUS. This insures 
 *	  correct conversion to V1 since V2 allows more drives.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_PROCESS_FAILURE
 *
 * Implicit Inputs:
 *
 *      V2 request structure
 *
 * Implicit Outputs:
 *
 *      None
 *
 * Considerations:
 *
 *      o if an error occurs during the conversion, an appropriate error
 *        message will be created and sent to the event logger.  The packet 
 *        will not get discarded, but the ramifications of such an error 
 *        cannot be predicted.  A client process may, for instance,  
 *        never terminate due to a lost final response.
 *
 * Module Test Plan:
 *
 *      None
 *
 * Revision History:
 *
 *      Howard I. Grapek        22-Mar-1993     Original.
 *
 *      Howard I. Grapek        20-Nov-1993     BR#280 fixed.
 *
 *      J.S. Alexander          14-Apr-1993     BR#124 - Fixed code to filter
 *              out CAP's 1 and 2 for VERSON0 and VERSION1 packets.
 *
 *      J.S. Alexander          14-Apr-1993     BR#465 - Added conversion of
 *              STATUS_INCORRECT_LOCKID to STATUS_VARY_DISALLOWED.
 *
 *      D. A. Beidle            15-Apr-1993.    Fixed several problems:
 *              BR#404 - Convert STATUS_INCORRECT_CAP_MODE to STATUS_CAP_IN_USE.
 *              BR#465 - Corrected V1 identifier count for QUERY CAP responses.
 *              BR#490 - Convert STATUS_CAP_OFFLINE to STATUS_CAP_IN_USE and
 *                       corrected to convert message_status in all responses.
 *      Alec Sharp              26-Jun-1993     Changed the version 2
 *              structures to have a V2_ prefix, and included v2_structs.h.
 *              This means that we no longer have to worry about changes
 *              to the current structures when converting. Also added code
 *              to handle intermediate responses to an audit request.
 *	A. W. Steere		13-Nov-1993	R4.0 BR# 72, 102, 146, 300
 *		change from bcopy to memcopy didn't change order of arguments
 *      A. W. Steere            14-Dec-1993     R4.0 BR#325 convert 
 *              STATUS_ACS_ONLINE to STATUS_VARY_DISALLOWED
 *	J. Borzuchowski		17-Sep-1993	R5.0 Mixed Media-- Added a
 *		check to only copy V1_MAX_ACS_DRIVES of drive statuses
 *		returned for the query mount and query mount scrach responses.
 *		This is necessary to insure correct conversion to v1 since
 *		v2 packets allow more.
 *	J. Borzuchowski		08-Mar-1994	R5.0 BR#143, BR#151, BR#152--
 *		Copy v2 count to v1 count.  
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
static char             module_string[] = "lm_cvt_v2_v1";
static ALIGNED_BYTES    result_bfr[MAX_MESSAGE_BLOCK];    /* results */

/*
 * Procedure declarations
 */
static void st_rs(RESPONSE_STATUS *v1_rs);
static void st_set_cap(V2_RESPONSE_TYPE *v2_res_ptr,
		       V1_RESPONSE_TYPE *v1_res_ptr, int *byte_count);
static void st_enter(V2_RESPONSE_TYPE *v2_res_ptr,
		     V1_RESPONSE_TYPE *v1_res_ptr, int *byte_count);
static void st_eject(V2_RESPONSE_TYPE *v2_res_ptr,
		     V1_RESPONSE_TYPE *v1_res_ptr, int *byte_count);
static void st_audit(V2_RESPONSE_TYPE *v2_res_ptr,
		     V1_RESPONSE_TYPE *v1_res_ptr, int *byte_count);
static void st_query(V2_RESPONSE_TYPE *v2_res_ptr,
		     V1_RESPONSE_TYPE *v1_res_ptr, int *byte_count);
static STATUS st_vary(V2_RESPONSE_TYPE *v2_res_ptr,
		      V1_RESPONSE_TYPE *v1_res_ptr);

STATUS 
lm_cvt_v2_v1 (
    char *response_ptr,                       /* acslm input buffer area */
    int *byte_count                         /* MODIFIED byte count */
)
{
    V2_RESPONSE_TYPE    *res_ptr;          /* pointer to response type msg's */
    V1_RESPONSE_TYPE    *v1_res_ptr;       /* pointer to v1 response msg's */
    STATUS              rval;              /* return value */
    char                *result_ptr;       /* converted packet. */

#ifdef DEBUG
    if TRACE (0)
        cl_trace(module_string, 2, (unsigned long) response_ptr,
                (unsigned long) byte_count);
#endif

    /* Check the sanity of incomming request */
    CL_ASSERT(module_string, ((response_ptr) && (byte_count))); 

    MLOGDEBUG(0,(MMSG(871, "%s: here... byte_count = %d"), module_string, *byte_count));

    /* zero the result buffer out */
    memset((char *) result_bfr, '\0', sizeof(result_bfr));

    /* set up some generic pointers */
    result_ptr = (char *) result_bfr;                   /* result's data area */
    v1_res_ptr = (V1_RESPONSE_TYPE *) result_ptr;       /* result pointer */
    res_ptr = (V2_RESPONSE_TYPE *) response_ptr;        /* incomming packet */

    /* copy ipc header (same between versions) */ 
    v1_res_ptr->generic_response.request_header.ipc_header =
        res_ptr->generic_response.request_header.ipc_header;

    /* copy message header (same between versions) */ 
    v1_res_ptr->generic_response.request_header.message_header =
        res_ptr->generic_response.request_header.message_header;

    /* copy message status */
    v1_res_ptr->generic_response.response_status = 
        res_ptr->generic_response.response_status;

    /* convert message status */
    st_rs(&v1_res_ptr->generic_response.response_status);

    /*
     * Copy the rest of the packet. 
     */
    
    switch (res_ptr->generic_response.request_header.message_header.command) {
      case COMMAND_AUDIT:
	st_audit(res_ptr, v1_res_ptr, byte_count);
	break;
	
      case COMMAND_EJECT:
	st_eject(res_ptr, v1_res_ptr, byte_count);
	break;
	
      case COMMAND_ENTER:
	st_enter(res_ptr, v1_res_ptr, byte_count);
	break;
	
      case COMMAND_QUERY:
	st_query(res_ptr, v1_res_ptr, byte_count);
	break;
	
      case COMMAND_SET_CAP:
	st_set_cap(res_ptr, v1_res_ptr, byte_count);
	break;
	
      case COMMAND_VARY:
	rval = st_vary(res_ptr, v1_res_ptr);
	if (rval != STATUS_SUCCESS)
	    return (rval);
	break;
	
      default:
	
	/* Other commands don't change, so return the packet unchanged.*/
	return(STATUS_SUCCESS);
    }

    MLOGDEBUG(0,(MMSG(872, "%s: returning byte_count(%d)\n"), module_string, *byte_count));

    /* return the resulting VERSION1 request */ 
    memcpy(response_ptr, result_ptr, *byte_count);


    return (STATUS_SUCCESS);
}


/*
 * Name:
 *
 *      st_rs
 *
 * Description:
 *
 *      This module will convert a VERSION2 RESPONSE_STATUS structure to
 *      a VERSION1 RESPONSE_STATUS structure.  
 *
 *      The structure definitions are identical, but since there are new
 *      LH_ERR codes in VERSION2 which do not translate directly into LH_ERR
 *      codes in VERSION1, we need to do the translation here.  
 *
 *      There are also new status codes in VERSION2 responses that must be
 *      converted to equivalent VERSION1 status codes.
 *
 * Return Values:
 *
 *      The VERSION1 RESPONSE_STATUS structure with the converted lh_error
 *      (only converts if necessary)
 *
 * Implicit Inputs:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      None.
 *
 * Considerations:
 *
 *      The version 2 status was already copied into the V1 status and
 *      this routine only changes the version specific differences.
 *
 * Module Test Plan:
 *
 *      None
 */
static void 
st_rs (
    RESPONSE_STATUS *v1_rs         /* Version 1 response status */
)
{
    /* convert VERSION2 LH_ERR codes to equivalent VERSION1 LH_ERR codes */
    if (v1_rs->status == STATUS_LIBRARY_FAILURE) {
        switch (v1_rs->identifier.lh_error) {
          case LH_ERR_ALREADY_RESERVED:
            v1_rs->identifier.lh_error = (short)LH_ERR_CAP_BUSY;
            break;

          case LH_ERR_CAP_OPEN:
          case LH_ERR_NOT_RESERVED:
            v1_rs->identifier.lh_error = (short)LH_ERR_CAP_FAILURE;
            break;

          case LH_ERR_LMU_LEVEL_INVALID:
            v1_rs->identifier.lh_error = (short)LH_ERR_LMU_FAILURE;
            break;

          case LH_ERR_NO_ERROR:
            v1_rs->identifier.lh_error = (short)LH_ERR_LH_FAILURE;
            break;

          default:                      /* no differences */
            break;
        }
    }

    /* convert VERSION2 status codes to equivalent VERSION1 status codes */
    switch (v1_rs->status) {
      case STATUS_CAP_OFFLINE:
      case STATUS_INCORRECT_CAP_MODE:
        v1_rs->status = STATUS_CAP_IN_USE;
        break;

      case STATUS_ACS_ONLINE:
      case STATUS_INCORRECT_LOCKID:
      case STATUS_INCORRECT_STATE:
      case STATUS_VARY_IN_PROGRESS:
        v1_rs->status = STATUS_VARY_DISALLOWED;
        break;

      default:                          /* no differences */
        break;
    }
}


/*
 * Name:
 *
 *      st_set_cap
 *
 * Description:
 *
 *      This module converts an set_cap response from VERSION2 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      None.
 */
static void 
st_set_cap (
    V2_RESPONSE_TYPE *v2_res_ptr,    /* V2 pointer to response type msg's */
    V1_RESPONSE_TYPE *v1_res_ptr,    /* V1 pointer to response type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V1_SET_CAP_RESPONSE *v1_scp;        /* version1 set_cap request */
    V2_SET_CAP_RESPONSE *v2_scp;        /* version2 set_cap request */
    int                 cnt;            /* miscellaneous counter */

    /* has cap_id in the status field of the response */
    v1_scp = &(v1_res_ptr->set_cap_response);
    v2_scp = &(v2_res_ptr->set_cap_response);

    /* copy the fixed portion (no mode in V1) */
    v1_scp->cap_priority = v2_scp->cap_priority;
    v1_scp->count        = v2_scp->count;

    MLOGDEBUG(0,(MMSG(873, "%s: V2 fixed portion: PRIO %d, MODE %d, COUNT %d\n"), 
                  module_string, v2_scp->cap_priority, 
                  v2_scp->cap_mode, v2_scp->count));

    /* copy the variable portion */
    for (cnt = 0; cnt < (int)v2_scp->count; ++cnt) {
        v1_scp->set_cap_status[cnt].cap_id = 
                v2_scp->set_cap_status[cnt].cap_id.lsm_id;

        if (v2_scp->set_cap_status[cnt].status.type == TYPE_CAP) {
            v1_scp->set_cap_status[cnt].status.status = 
                v2_scp->set_cap_status[cnt].status.status;
            v1_scp->set_cap_status[cnt].status.type = 
                v2_scp->set_cap_status[cnt].status.type;
            v1_scp->set_cap_status[cnt].status.identifier.v1_cap_id = 
                v2_scp->set_cap_status[cnt].status.identifier.lsm_id;
        }
        else 
            v1_scp->set_cap_status[cnt].status = 
                v2_scp->set_cap_status[cnt].status;
    }

    *byte_count = (char *)&v1_scp->set_cap_status[v1_scp->count] - 
        (char *)v1_scp;
}

/*
 * Name:
 *
 *      st_enter
 *
 * Description:
 *
 *      This module converts an enter response from VERSION2 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      None.
 */
static void 
st_enter (
    V2_RESPONSE_TYPE *v2_res_ptr,    /* V2 pointer to response type msg's */
    V1_RESPONSE_TYPE *v1_res_ptr,    /* V1 pointer to response type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V1_ENTER_RESPONSE   *v1_enp;        /* version 1 enter RESPONSE */
    V2_ENTER_RESPONSE   *v2_enp;        /* VERSION2 ENTER RESPONSE */
    int                 cnt;            /* miscellaneous counter */


    /* has cap_id in fixed and variable portion of the packet. */
    v1_enp = &(v1_res_ptr->enter_response);
    v2_enp = &(v2_res_ptr->enter_response);

    /* copy the fixed portion */
    v1_enp->cap_id = v2_enp->cap_id.lsm_id;
    v1_enp->count  = v2_enp->count;

    /* copy the variable portion */
    for (cnt = 0; cnt < (int)v2_enp->count; ++cnt) {
        v1_enp->volume_status[cnt].vol_id = v2_enp->volume_status[cnt].vol_id;

        if (v2_enp->volume_status[cnt].status.type == TYPE_CAP) {
            v1_enp->volume_status[cnt].status.status = 
                v2_enp->volume_status[cnt].status.status;
            v1_enp->volume_status[cnt].status.type = 
                v2_enp->volume_status[cnt].status.type;
            v1_enp->volume_status[cnt].status.identifier.v1_cap_id = 
                v2_enp->volume_status[cnt].status.identifier.lsm_id;
        }
        else {
            v1_enp->volume_status[cnt].status = 
                v2_enp->volume_status[cnt].status;
        }
    }

    *byte_count = (char *)&v1_enp->volume_status[v1_enp->count] - 
        (char *)v1_enp;
}

/*
 * Name:
 *
 *      st_eject
 *
 * Description:
 *
 *      This module converts an eject response from VERSION2 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      None.
 */
static void 
st_eject (
    V2_RESPONSE_TYPE *v2_res_ptr,    /* V2 pointer to response type msg's */
    V1_RESPONSE_TYPE *v1_res_ptr,    /* V1 pointer to response type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V1_EJECT_RESPONSE   *v1_ejp;        /* version 1 eject RESPONSE */
    V2_EJECT_RESPONSE   *v2_ejp;        /* VERSION2 EJECT RESPONSE */
    int                 cnt;            /* miscellaneous counter */

    /* has cap_id in fixed and variable portion of the packet. */
    v1_ejp = &(v1_res_ptr->eject_response);
    v2_ejp = &(v2_res_ptr->eject_response);

    /* copy the fixed portion */
    v1_ejp->cap_id = v2_ejp->cap_id.lsm_id;
    v1_ejp->count  = v2_ejp->count;

    /* copy the variable portion */
    for (cnt = 0; cnt < (int)v2_ejp->count; ++cnt) {
        v1_ejp->volume_status[cnt].vol_id = v2_ejp->volume_status[cnt].vol_id;

        if (v2_ejp->volume_status[cnt].status.type == TYPE_CAP) {
            v1_ejp->volume_status[cnt].status.status = 
                v2_ejp->volume_status[cnt].status.status;
            v1_ejp->volume_status[cnt].status.type = 
                v2_ejp->volume_status[cnt].status.type;
            v1_ejp->volume_status[cnt].status.identifier.v1_cap_id = 
                v2_ejp->volume_status[cnt].status.identifier.lsm_id;
        }
        else {
            v1_ejp->volume_status[cnt].status = 
                v2_ejp->volume_status[cnt].status;
        }
    }

    *byte_count = (char *)&v1_ejp->volume_status[v1_ejp->count] - 
        (char *)v1_ejp;
}

/*
 * Name:
 *
 *      st_audit
 *
 * Description:
 *
 *      This module converts an audit response from VERSION2 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *      None
 */
static void 
st_audit (
    V2_RESPONSE_TYPE *v2_res_ptr,    /* V2 pointer to response type msg's */
    V1_RESPONSE_TYPE *v1_res_ptr,    /* V1 pointer to response type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V1_AUDIT_RESPONSE   *v1_aup;        /* version 1 final audit RESPONSE */
    V2_AUDIT_RESPONSE   *v2_aup;        /* VERSION2 final AUDIT RESPONSE */
    V1_EJECT_ENTER      *v1_irp;        /* version 1 intermed audit RESPONSE */
    V2_EJECT_ENTER      *v2_irp;        /* VERSION2 intermed. AUDIT RESPONSE */
    char                *from_ident;    /* pointer to source identifier */
    char                *to_ident;      /* pointer to dest identifier */
    int                 copy_size;      /* number of bytes to copy */
    MESSAGE_HEADER      *mhp;           /* Pointer to message header - used to
				         * make code clearer                */

    /*
     * There are two different types of response - an intermediate and
     * a final response.  We handle the two separately.
     */

    mhp = &v2_res_ptr->generic_response.request_header.message_header;

    if (mhp->message_options & INTERMEDIATE) {

	/* Intermediate response */

	v1_irp = &v1_res_ptr->eject_enter;
	v2_irp = &v2_res_ptr->eject_enter;
	
	/* copy fixed portion */
	v1_irp->cap_id = v2_irp->cap_id.lsm_id;
	v1_irp->count  = v2_irp->count;
	
	/* copy variable portion */
	from_ident = (char *) v2_irp->volume_status;
	to_ident   = (char *) v1_irp->volume_status;
	copy_size  = *byte_count -
	    ((char *) v2_irp->volume_status - (char *) v2_irp);

	if (copy_size < 0)
	    copy_size = 0;
	memcpy(to_ident, from_ident, copy_size);
	
	*byte_count = copy_size +
	    ((char *)v1_irp->volume_status - (char *) v1_irp);

    }
    else {

	/* Final response */

	v1_aup = &v1_res_ptr->audit_response;
	v2_aup = &v2_res_ptr->audit_response;
	
	/* copy fixed portion */
	v1_aup->cap_id = v2_aup->cap_id.lsm_id;
	v1_aup->type   = v2_aup->type;
	v1_aup->count  = v2_aup->count;
	
	/* copy variable portion */
	from_ident = (char *) &v2_aup->identifier_status;
	to_ident   = (char *) &v1_aup->identifier_status;
	copy_size  = *byte_count -
	    ((char *) &v2_aup->identifier_status - (char *) v2_aup);
	if (copy_size < 0)
	    copy_size = 0;
	memcpy(to_ident, from_ident, copy_size);
	
	*byte_count = copy_size +
	    ((char *)&v1_aup->identifier_status - (char *) v1_aup);

    }
}

/*
 * Name:
 *
 *      st_query
 *
 * Description:
 *
 *      This module converts an query response from VERSION2 to VERSION1
 *
 * Return Values:
 *
 *      None.
 *
 * Implicit Outputs:
 *
 *	None.
 */
static void 
st_query (
    V2_RESPONSE_TYPE *v2_res_ptr,    /* V2 pointer to response type msg's */
    V1_RESPONSE_TYPE *v1_res_ptr,    /* V1 pointer to response type msg's */
    int *byte_count    /* number of bytes in request. */
)
{
    V1_QUERY_RESPONSE   *v1_qp;         /* version 1 query RESPONSE */
    V2_QUERY_RESPONSE   *v2_qp;         /* VERSION2 QUERY RESPONSE */
    char                *from_ident;    /* pointer to source identifier */
    char                *to_ident;      /* pointer to dest identifier */
    int                 copy_size;      /* number of bytes to copy */
    int                 variable_size;  /* number of bytes in variabe portion */
    int                 cnt;            /* miscellaneous counter */
    int			drive_count=0;	/* number of v2 drive status structs */
    int			i;		/* loop index for copy */

    MLOGDEBUG(0,(MMSG(874, "%s: st_query: here... byte_count = %d"),
	module_string, *byte_count));

    /* has cap_id in variable portion (status) of the packet. */
    v1_qp = &(v1_res_ptr->query_response);
    v2_qp = &(v2_res_ptr->query_response);

    /* copy the fixed portion */
    v1_qp->type  = v2_qp->type;

    /* copy variable portion */
    switch (v2_qp->type) {
	case TYPE_CAP:
            /* convert CAP statuses to VERSION1 format */
	    v1_qp->count = 0;
            for (cnt = 0; cnt < (int)v2_qp->count; ++cnt) {

                /* filter out statuses for non-zero CAP numbers */
                if (v2_qp->status_response.cap_status[cnt].cap_id.cap == 0) {
    
                    /* convert status */
                    v1_qp->status_response.cap_status[v1_qp->count].cap_id = 
                        v2_qp->status_response.cap_status[cnt].cap_id.lsm_id;
                    v1_qp->status_response.cap_status[v1_qp->count].status = 
                        v2_qp->status_response.cap_status[cnt].status;
                    v1_qp->status_response.cap_status[v1_qp->count].cap_priority = 
                        v2_qp->status_response.cap_status[cnt].cap_priority;
                    v1_qp->status_response.cap_status[v1_qp->count].cap_size = 
                        v2_qp->status_response.cap_status[cnt].cap_size;
                    v1_qp->count++;
                }
            }
            /* Determine size of the variable portion. */
            variable_size = (*byte_count - 
                         ((char *) &v2_qp->status_response - (char *) v2_qp));
	    break;

        case TYPE_MOUNT:
	    v1_qp->count = v2_qp->count;
            for (cnt = 0; cnt < (int)v2_qp->count; ++cnt) {
		v1_qp->status_response.mount_status[cnt].vol_id =
		  v2_qp->status_response.mount_status[cnt].vol_id;
		v1_qp->status_response.mount_status[cnt].status =
		  v2_qp->status_response.mount_status[cnt].status;

		drive_count = 
		  v2_qp->status_response.mount_status[cnt].drive_count;

		if (drive_count > V1_MAX_ACS_DRIVES) {
		    drive_count = V1_MAX_ACS_DRIVES;
    	            MLOGDEBUG(0,(MMSG(1013, 
	              "%s: st_query: V2 drive_count = %d, exceeds max. Number"
		      "of drive status structures returned for volid %s "
		      "truncated to V1_MAX_ACS_DRIVES.\n"),
    	              module_string, drive_count, 
		      v1_qp->status_response.mount_status[cnt].vol_id.
		      external_label));
                }
		v1_qp->status_response.mount_status[cnt].drive_count = 
		  drive_count;

                /* Copy drive status structure. */
		for(i = 0; i < drive_count; i++){
		  v1_qp->status_response.mount_status[cnt].drive_status[i].
		    drive_id =
		    v2_qp->status_response.mount_status[cnt].drive_status[i].
		    drive_id;
		  v1_qp->status_response.mount_status[cnt].drive_status[i].
		    state =
		    v2_qp->status_response.mount_status[cnt].drive_status[i].
		    state;
		  v1_qp->status_response.mount_status[cnt].drive_status[i].
		    vol_id =
		    v2_qp->status_response.mount_status[cnt].drive_status[i].
		    vol_id;
		  v1_qp->status_response.mount_status[cnt].drive_status[i].
		    status =
		    v2_qp->status_response.mount_status[cnt].drive_status[i].
		    status;
                } /* end copy drive status */

            } /* end copy mount status */

            /* Determine size of the V1 variable portion. */
            variable_size = (char *)
	      &v1_qp->status_response.mount_status[cnt].drive_status[drive_count]
	      - (char *) v1_qp;
	    break;

	case TYPE_MOUNT_SCRATCH:
            v1_qp->count = v2_qp->count;
            for (cnt = 0; cnt < (int)v2_qp->count; ++cnt) {
		v1_qp->status_response.mount_scratch_status[cnt].pool_id =
		  v2_qp->status_response.mount_scratch_status[cnt].pool_id;
		v1_qp->status_response.mount_scratch_status[cnt].status =
		  v2_qp->status_response.mount_scratch_status[cnt].status;

		drive_count = 
		  v2_qp->status_response.mount_scratch_status[cnt].drive_count;

		if (drive_count > V1_MAX_ACS_DRIVES) {
		    drive_count = V1_MAX_ACS_DRIVES;
    	            MLOGDEBUG(0,(MMSG(1014, 
	              "%s: st_query: V2 drive_count = %d, exceeds max. Number "
		      "of drive status structures returned for poolid %ld "
		      "truncated to V1_MAX_ACS_DRIVES.\n"),
    	              module_string, drive_count, 
		      v1_qp->status_response.mount_scratch_status[cnt].
		      pool_id.pool));
                }
		v1_qp->status_response.mount_scratch_status[cnt].drive_count = 
		  drive_count;

                /* Copy drive status structure. */
		for(i = 0; i < drive_count; i++){
		  v1_qp->status_response.mount_scratch_status[cnt].
		    drive_list[i].drive_id =
		    v2_qp->status_response.mount_scratch_status[cnt].
		    drive_list[i].drive_id;
		  v1_qp->status_response.mount_scratch_status[cnt].
		    drive_list[i].state =
		    v2_qp->status_response.mount_scratch_status[cnt].
		    drive_list[i].state;
		  v1_qp->status_response.mount_scratch_status[cnt].
		    drive_list[i].vol_id =
		    v2_qp->status_response.mount_scratch_status[cnt].
		    drive_list[i].vol_id;
		  v1_qp->status_response.mount_scratch_status[cnt].
		    drive_list[i].status =
		    v2_qp->status_response.mount_scratch_status[cnt].
		    drive_list[i].status;
                }  /* end copy drive status */

	    }  /* end copy mount scratch status */

            /* Determine size of the V1 variable portion. */
            variable_size = (char *)
	      &v1_qp->status_response.mount_scratch_status[cnt].drive_list[drive_count] - 
	      (char *) v1_qp;
	    break;

	default:
	    /* The other types did not change, copy the variable portions. */
            v1_qp->count = v2_qp->count;
    
    	    MLOGDEBUG(0,(MMSG(1015, 
	      "%s: st_query: not type cap,mount,or mount scratch, fixed "
	      "portion: count = %d"),
    	      module_string, v1_qp->count));
    
            from_ident = (char *) &v2_qp->status_response;
            to_ident   = (char *) &v1_qp->status_response;

	    /* Determine size of V2 variable portion. */
            copy_size  = (*byte_count - 
                          ((char *) &v2_qp->status_response - (char *) v2_qp));
    
    	    MLOGDEBUG(0,(MMSG(876, "%s: st_query: just before copy, copy_size = %d"),
    	      module_string, copy_size));
    
    	    if (copy_size < 0) {
    	        copy_size = 0;
            }
            memcpy(to_ident, from_ident, copy_size);

	    /* Variable size of V1 response is the same as was copied from V2
	     * response.
	     */
	    variable_size = copy_size;
            break;
    }

    /* Determine total size  of V1 response by adding variable and 
     * fixed portions. 
     */
    *byte_count = (variable_size + 
                   ((char *) &v1_qp->status_response - (char *) v1_qp));

    MLOGDEBUG(0,(MMSG(877, "%s: st_query: got past variable portion"), module_string));
}

/*
 * Name:
 *
 *      st_vary
 *
 * Description:
 *
 *      This module converts a vary response from VERSION2 to VERSION1.
 * 	The byte count stays the same, so it is not passed as a parameter.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *	STATUS_PROCESS_FAILURE
 *
 * Implicit Outputs:
 *
 *      None.
 */
static STATUS 
st_vary (
    V2_RESPONSE_TYPE *v2_res_ptr,    /* V2 pointer to response type msg's */
    V1_RESPONSE_TYPE *v1_res_ptr     /* V1 pointer to response type msg's */
)
{
    V1_VARY_RESPONSE    *v1_vp;         /* version 1 vary RESPONSE */
    V2_VARY_RESPONSE    *v2_vp;         /* VERSION2 VARY RESPONSE */
    int                 cnt;            /* miscellaneous counter */


    /* vary has lh_identifiers to be converted. */
    v1_vp = &(v1_res_ptr->vary_response);
    v2_vp = &(v2_res_ptr->vary_response);

    /* copy the fixed portion */
    v1_vp->state = v2_vp->state;
    v1_vp->type  = v2_vp->type;
    v1_vp->count = v2_vp->count;

    /* copy the variable portion */
    for (cnt = 0; cnt < (int)v2_vp->count; ++cnt) {
        switch(v2_vp->type) {
            /*
             * copy response status (same between versions) with 
             * the exception of LH_ERRORS.
             */
	  case TYPE_ACS:
	    v1_vp->device_status.acs_status[cnt] = 
                v2_vp->device_status.acs_status[cnt];
	    
	    v1_vp->device_status.acs_status[cnt].status =
                v2_vp->device_status.acs_status[cnt].status;
	    
	    st_rs(&v1_vp->device_status.acs_status[cnt].status);
	    break;          
	    
	  case TYPE_LSM:
	    v1_vp->device_status.lsm_status[cnt] = 
                v2_vp->device_status.lsm_status[cnt];
	    
	    v1_vp->device_status.lsm_status[cnt].status =
                v2_vp->device_status.lsm_status[cnt].status;
	    
	    st_rs(&v1_vp->device_status.lsm_status[cnt].status);
	    break;          
	    
	  case TYPE_DRIVE:
	    v1_vp->device_status.drive_status[cnt] = 
                v2_vp->device_status.drive_status[cnt];
	    
	    v1_vp->device_status.drive_status[cnt].status =
                v2_vp->device_status.drive_status[cnt].status;
	    
	    st_rs(&v1_vp->device_status.drive_status[cnt].status);
	    break;          
	    
	  case TYPE_PORT:
	    v1_vp->device_status.port_status[cnt] = 
                v2_vp->device_status.port_status[cnt];
	    
	    v1_vp->device_status.port_status[cnt].status = 
                v2_vp->device_status.port_status[cnt].status;
	    
	    st_rs(&v1_vp->device_status.port_status[cnt].status);
	    break;          
	    
	  case TYPE_CAP:
	  default:
	    /* 
	     * unexpected type: , Version 1 vary has no TYPE_CAP. 
	     * This should never happen... it should be caught in
	     * the acslm earlier on.
	     */
	    MLOGU((module_string, "Invalid Type", 
			      STATUS_PROCESS_FAILURE,
	         MNOMSG));
	    return (STATUS_PROCESS_FAILURE);
        }
    }

    return(STATUS_SUCCESS);
}
