#ifndef lint
static char SccsId[] = "@(#) %full_name:	server/csrc/cvt_v3_v2/2.0A %";
#endif

/*=
 * Copyright (1992, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      lm_cvt_v3_v2
 *
 * Description:
 *
 *      This routine will convert a VERSION3 packet to a VERSION2 packet.
 *      It is one of the last things done to a packet before the response
 *      is sent out.
 *
 *      Convert the packet if necessary. The size and structure of the
 *      packet does not change - only some of the values. In particular,
 *      Version 3 has new values for:
 *         ANY_ACS
 *         ANY_LSM
 *         ANY_CAP
 *         ALL_CAP
 *         SAME_PRIORITY
 *         SAME_POOL
 *      Thus, whenever we find the new values, we convert them to the old
 *      values. The old values are in #defines with the prefix V2_ and
 *      these are found in v2_structs.h.  For example V2_ANY_ACS.
 *
 *      We do this conversion for any specifically mentioned variables.
 *      We also take a generic approach to RESPONSE_STATUS blocks, and pass
 *      all such blocks through a general conversion routine. These blocks
 *      have an identifier field, and instead of trying to specifically
 *      identify which types of response could have which types of
 *      variable, we generalize and cover all cases.
 *
 *      Return the new packet and its length to the input process. The
 *      length is unchanged, but we retain the variable for consistency
 *      with lm_cvt_v2_v1 and lm_cvt_v1_v0.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_INVALID_COMMAND    Input buffer has invalid command
 *      STATUS_PROCESS_FAILURE    Software failure
 *
 * Parameters:
 *
 *   V2_RESPONSE_TYPE *resp_ptr   Pointer to input buffer.  The buffer is
 *                                written to directly for output. Note that
 *                                the RESPONSE_TYPE structure is identical
 *                                for VERSION2 and VERSION3 packets.
 *
 *   int              *byte_count Pointer to number of bytes in the
 *                                response packet. This value is used on
 *                                input and written on output.
 *
 * Implicit Inputs:   None.
 *
 * Implicit Outputs:  None
 *
 * Considerations:
 *
 *      The transformations are simply changing the values of variables.
 *      There are no structure changes.  Thus, to save time we simply
 *      write directly onto the input buffer.
 *
 * Revision History:
 *
 *      Alec Sharp          15-Jun-1993  Original.
 *      Alec Sharp          02-Sep-1993  Return immediately if converting
 *                  an acknowledgement.
 *      Alec Sharp          10-Sep-1993  If the TYPE is invalid, don't
 *                  return an error. We may be returning a packet that 
 *                  caused an error return because of an invalid TYPE field.
 *      Alec Sharp          01-Oct-1993  Convert STATUS_COMMAND_ACCESS_DENIED
 *                   to STATUS_INVALID_COMMAND, and STATUS_VOLUME_ACCESS_DENIED
 *                   to STATUS_VOLUME_NOT_IN_LIBRARY.
 *      Alec Sharp          1993ov-1993  Fixed bug where statuses were not
 *                  getting converted.
 *      Andy Steere         10-Dec-1993  R4.0 BR#142 V2 requests for LSMs 16-23
 *                  change STATUS_LSM_NOT_IN_LIBRARY to STATUS_INVALID_LSM
 *	Alec Sharp	    20-Jan-1993	 Major changes to convert the
 *		    identifiers in the message status and the invidual
 *		    response statuses.
 *	Alec Sharp	    21-Jan-1993	 R4.0 BR#391, BR#342. Fixed bug
 *		    where static functions were being called with different
 *                  parameters. Fixed bug where identifiers in
 *                  RESPONSE_STATUS blocks were not being translated.
 *                  Generalized the routine.
 *=
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "v2_structs.h"
#include "acslm.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char *self = "lm_cvt_v3_v2";

/*
 * Procedure declarations
 */

static void st_convert_acs(ACS *p_acs);
static void st_convert_cap(CAPID *p_capid, STATUS *p_status);
static void st_convert_cap_priority(CAP_PRIORITY *p_cap_priority);
static void st_convert_drv(DRIVEID *p_driveid, STATUS *p_status);
static void st_convert_lsm(LSMID *p_lsmid, STATUS *p_status);
static void st_convert_pool(POOLID *p_poolid);
static void st_convert_response_status(RESPONSE_STATUS *p_rstatus);
static void st_convert_status(STATUS *p_status);
static void st_response_audit(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_clear_lock(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_define_pool(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_delete_pool(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_dismount(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_eject(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_enter(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_lock(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_mount(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_mount_scratch(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_query(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_query_lock(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_set_cap(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_set_clean(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_set_scratch(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_unlock(V2_RESPONSE_TYPE *resp_ptr);
static void st_response_vary(V2_RESPONSE_TYPE *resp_ptr);



STATUS 
lm_cvt_v3_v2 (
    V2_RESPONSE_TYPE *resp_ptr,             /* pointer to request message buffer */
    int *byte_count           /* number of bytes in request.       */
)
{
    MESSAGE_HEADER  *p_msghdr;

    
    /* Check the sanity of incoming request */
    CL_ASSERT(self, (resp_ptr != NULL && *byte_count > 0)); 
    
    
    /* If this is an acknowledgement, there is no variable information to
       convert.  Since we're not converting fixed information between v3
       and v2 packets, we can simply return.  */
    
    p_msghdr = &resp_ptr->generic_response.request_header.message_header;
    if (p_msghdr->message_options & ACKNOWLEDGE)
	return (STATUS_SUCCESS);
    
    
    /* Convert the response status. This converts both the identifier field
       and the status field. We have some new statuses in R4 and we must
       convert them on the way down. Note that we also have to convert
       the identifiers and the VOLUME_ACCESS_DENIED status in the individual
       responses. Note that we make the assumption that the alignment
       of a generic_response is the same as the alignment of all the
       specific types of responses. This is not guaranteed by the compiler
       but works and is also assumed elsewhere in the software.  */
    
    st_convert_response_status(&resp_ptr->generic_response.response_status);
   
    
    /* Do the appropriate thing for the request type */
    switch (resp_ptr->generic_response.request_header.message_header.command) {
	
      case COMMAND_AUDIT:
	st_response_audit(resp_ptr);
	break;
	
      case COMMAND_CANCEL:         /* No changes needed */
	break;
	
      case COMMAND_CLEAR_LOCK:
	st_response_clear_lock(resp_ptr);
	break;
	
      case COMMAND_DEFINE_POOL:
	st_response_define_pool(resp_ptr);
	break;
	
      case COMMAND_DELETE_POOL:
	st_response_delete_pool(resp_ptr);
	break;
	
      case COMMAND_DISMOUNT:
	st_response_dismount(resp_ptr);
	break;
	
      case COMMAND_EJECT:
	st_response_eject(resp_ptr);
	break;
	
      case COMMAND_ENTER:
	st_response_enter(resp_ptr);
	break;
	
      case COMMAND_IDLE:         /* No changes needed */
	break;
	
      case COMMAND_LOCK:
	st_response_lock(resp_ptr);
	break;
	
      case COMMAND_MOUNT:
	st_response_mount(resp_ptr);
	break;
	
      case COMMAND_MOUNT_SCRATCH:
	st_response_mount_scratch(resp_ptr);
	break;
	
      case COMMAND_QUERY:
	st_response_query(resp_ptr);
	break;
	
      case COMMAND_QUERY_LOCK:
	st_response_query_lock(resp_ptr);
	break;

      case COMMAND_SET_CAP:
	st_response_set_cap(resp_ptr);
	break;
	    
      case COMMAND_SET_CLEAN:
	st_response_set_clean(resp_ptr);
	break;
	
      case COMMAND_SET_SCRATCH:
	st_response_set_scratch(resp_ptr);
	break;
	
      case COMMAND_START:         /* No changes needed */
	break; 
	
      case COMMAND_UNLOCK:
	st_response_unlock(resp_ptr);
	break;
	
      case COMMAND_VARY:
	st_response_vary(resp_ptr);
	break;
	
      default:
	return (STATUS_INVALID_COMMAND);
    } 
    
    return (STATUS_SUCCESS);
}

/* -----------------------------------------------------------------
 * Name:  st_convert_acs
 *
 * Description:
 *
 *      This function checks the ACS value and converts the value
 *      in place if necessary.
 *
 * Parameters:
 *
 *      ACS   *p_acs     Pointer to ACS value
 *
 */

static void 
st_convert_acs (ACS *p_acs)
{
    if (*p_acs == ANY_ACS)
	*p_acs = V2_ANY_ACS;
}

/* --------------------------------------------------------------------
 * Name:  st_convert_cap
 *
 * Description:
 *
 *      This function checks the CAP, LSM, and ACS values in a cap_id and
 *      converts the values in place if necessary. It also converts the
 *      passed in status if appropriate.
 *
 * Parameters:
 *
 *      CAPID  *p_capid    Pointer to cap ID.
 *      STATUS *p_status   Pointer to status.
 */

static void 
st_convert_cap (CAPID *p_capid, STATUS *p_status)
{
    if (p_capid->cap == ANY_CAP)
	p_capid->cap = V2_ANY_CAP;
    else if (p_capid->cap == ALL_CAP)
	p_capid->cap = V2_ALL_CAP;
    
    if (p_capid->lsm_id.acs == ANY_ACS)
	p_capid->lsm_id.acs = V2_ANY_ACS;
    
    if (p_capid->lsm_id.lsm == ANY_LSM)
	p_capid->lsm_id.lsm = V2_ANY_LSM;

    /*
     * V3+ packets consider LSMs 16-23 as valid ids, although they may
     * not be configured.  For V2 packets LSMs 16-23 are invalid,
     */ 
    if ((*p_status == STATUS_LSM_NOT_IN_LIBRARY) &&
        (p_capid->lsm_id.lsm >= V2_ANY_LSM)) {
        *p_status = STATUS_INVALID_LSM;
    }

}

/* --------------------------------------------------------------------
 * Name:  st_convert_cap_priority
 *
 * Description:
 *
 *      This function checks the cap priority and converts the value in
 *      place if necessary.
 *
 * Parameters:
 *
 *      CAP_PRIORITY    *p_cap_priority
 *
 */

static void 
st_convert_cap_priority (CAP_PRIORITY *p_cap_priority)
{
    if (*p_cap_priority == SAME_PRIORITY)
	*p_cap_priority = V2_SAME_PRIORITY;
}

/* -------------------------------------------------------------------
 * Name:  st_convert_drv
 *
 * Description:
 *
 *      This function checks the LSM, and ACS values in an drive_id and
 *      converts the values in place if necessary.  The same functionality
 *      could be provided by a call to st_convert_lsm, but having this
 *      st_convert_drive call makes the calling code a little easier to
 *      read by allowing a shorter variable name in the function call.
 *      It also converts the passed in status if appropriate.
 *
 * Parameters:
 *
 *      DRIVEID *p_driveid   Pointer to DRIVEID
 *      STATUS  *p_status    Pointer to status.
 */

static void 
st_convert_drv (DRIVEID *p_driveid, STATUS *p_status)
{
    if (p_driveid->panel_id.lsm_id.acs == ANY_ACS)
	p_driveid->panel_id.lsm_id.acs = V2_ANY_ACS;
    
    if (p_driveid->panel_id.lsm_id.lsm == ANY_LSM)
	p_driveid->panel_id.lsm_id.lsm = V2_ANY_LSM;

    /*
     * V3+ packets consider LSMs 16-23 as valid ids, although they may
     * not be configured.  For V2 packets LSMs 16-23 are invalid,
     */ 
    if ((*p_status == STATUS_LSM_NOT_IN_LIBRARY) &&
       (p_driveid->panel_id.lsm_id.lsm >= V2_ANY_LSM)) {
        *p_status = STATUS_INVALID_LSM;
    }

}

/* -------------------------------------------------------------------
 * Name:  st_convert_lsm
 *
 * Description:
 *
 *      This function checks the LSM, and ACS values in an lsm_id and
 *      converts the values in place if necessary. It also converts the
 *      passed in status if appropriate.
 *
 * Parameters:
 *
 *      LSMID   *p_lsmid     Pointer to LSMID.
 *      STATUS  *p_status    Pointer to status.
 */

static void 
st_convert_lsm (LSMID *p_lsmid, STATUS *p_status)
{
    if (p_lsmid->acs == ANY_ACS)
	p_lsmid->acs = V2_ANY_ACS;
    
    if (p_lsmid->lsm == ANY_LSM)
	p_lsmid->lsm = V2_ANY_LSM;
    
    /* 
     * V3+ packets consider LSMs 16-23 as valid ids, although they may 
     * not be configured.  For V2 packets LSMs 16-23 are invalid, 
     */
    if ((*p_status == STATUS_LSM_NOT_IN_LIBRARY) && 
	(p_lsmid->lsm >= V2_ANY_LSM)) {
        *p_status = STATUS_INVALID_LSM;
    }
}

/* -------------------------------------------------------------------
 * Name:  st_convert_pool
 *
 * Description:
 *
 *      This function checks the pool ID and converts the values in
 *      place if necessary.
 *
 * Parameters:
 *
 *      POOLID *p_poolid   Pointer to POOLID
 *
 */

static void 
st_convert_pool (POOLID *p_poolid)
{
    if (p_poolid->pool == SAME_POOL)
	p_poolid->pool = V2_SAME_POOL;
}

/* --------------------------------------------------------------------
 * Name:  st_convert_response_status
 *
 * Description:
 *
 *      This function converts a RESPONSE_STATUS identifier and 
 *      associated statuses.
 *
 * Parameters:
 *
 *      RESPONSE_STATUS  *p_rstatus   Pointer to a RESPONSE_STATUS structure.
 */

static void 
st_convert_response_status (RESPONSE_STATUS *p_rstatus)
{
    /* Convert the identifier */
    
    switch (p_rstatus->type) {
      case TYPE_ACS:
	st_convert_acs(&p_rstatus->identifier.acs_id);
	break;
      case TYPE_CAP:
	st_convert_cap(&p_rstatus->identifier.cap_id,
		       &p_rstatus->status);
	break;
      case TYPE_DRIVE:
	st_convert_drv(&p_rstatus->identifier.drive_id,
		       &p_rstatus->status);
	break;
      case TYPE_LSM:
	st_convert_lsm(&p_rstatus->identifier.lsm_id,
		       &p_rstatus->status);
	break;
      case TYPE_PANEL:
	st_convert_lsm(&p_rstatus->identifier.panel_id.lsm_id,
		       &p_rstatus->status);
	break;
      case TYPE_POOL:
	st_convert_pool(&p_rstatus->identifier.pool_id);
	break;
      case TYPE_SUBPANEL:
	st_convert_lsm(&p_rstatus->identifier.subpanel_id.panel_id.lsm_id,
		       &p_rstatus->status);
	break;
      default:
	break;
    }
    
    /* Convert the status */
    
    st_convert_status (&p_rstatus->status);
    
}

/* -------------------------------------------------------------------
 * Name:  st_convert_status
 *
 * Description:
 *
 *      This function converts a status. It is used to convert the new
 *      Access Control statuses for downlevel systems.
 *
 * Parameters:
 *
 *      STATUS *p_status    Pointer to status field.
 *
 */

static void 
st_convert_status (STATUS *p_status)
{
    switch (*p_status) {
      case STATUS_VOLUME_ACCESS_DENIED:
	*p_status = STATUS_VOLUME_NOT_IN_LIBRARY;
	break;
      case STATUS_COMMAND_ACCESS_DENIED:
	*p_status = STATUS_INVALID_COMMAND;
	break;
      default:
	break;
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_audit
 *
 * Description:
 *
 *      This function converts an audit response from VERSION3 to VERSION2
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_audit (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;           /* Loop variable                        */
    MESSAGE_HEADER    *mhp;         /* Pointer to message header - used to
				     * make code clearer                    */
    V2_AUDIT_RESPONSE *frp;         /* Pointer to final audit response - used
				     * to make code clearer                 */
    RESPONSE_STATUS   *p_rstatus;
    
    /*
     * There are two types of response, intermediate and final. They have
     * different structures so we have to handle them differently.
     */
    
    mhp = &resp_ptr->generic_response.request_header.message_header;
    
    if (mhp->message_options & INTERMEDIATE) {
	
	/* ------ Intermediate Response -------- */
	
	st_convert_cap(&resp_ptr->eject_enter.cap_id,
		       &resp_ptr->eject_enter.message_status.status);

	for (i=0; i < (int)resp_ptr->eject_enter.count && i < MAX_ID; i++) {
	    st_convert_response_status(&resp_ptr->eject_enter.
				       volume_status[i].status);
	}
	
    }
    else {
	
	/* ---------- Final response --------- */
	
	frp = &resp_ptr->audit_response;
	st_convert_cap(&frp->cap_id, &frp->message_status.status);
	
	
	/* -- Convert the thing being audited -- */
	
	switch (frp->type) {
	    
	  case TYPE_ACS:
	    for (i=0; i < MAX_ID && i < (int)frp->count; i++) {
		
		p_rstatus = &frp->identifier_status.acs_status[i].status;
		
		st_convert_acs(&frp->identifier_status.acs_status[i].acs_id);
		
		st_convert_response_status(p_rstatus);
	    }
	    break;
	    
	  case TYPE_LSM:
	    for (i=0; i < MAX_ID && i < (int)frp->count; i++) {
		
		p_rstatus = &frp->identifier_status.lsm_status[i].status;
		
		st_convert_lsm(&frp->identifier_status.lsm_status[i].lsm_id,
			       &p_rstatus->status);
		
		st_convert_response_status(p_rstatus);
	    }
	    break;
	    
	  case TYPE_PANEL:
	    for (i=0; i < MAX_ID && i < (int)frp->count; i++) {
		
		p_rstatus = &frp->identifier_status.panel_status[i].status;
		
		st_convert_lsm(&frp->identifier_status.panel_status[i].
			       panel_id.lsm_id,
			       &p_rstatus->status);
		
		st_convert_response_status(p_rstatus);
	    }
	    break;
	    
	  case TYPE_SERVER:      /* No changes needed */
	    break;
	    
	  case TYPE_SUBPANEL:
	    for (i=0; i < MAX_ID && i < (int)frp->count; i++) {
		
		p_rstatus = &frp->identifier_status.subpanel_status[i].status;
		
		st_convert_lsm(&frp->identifier_status.subpanel_status[i].
			       subpanel_id.panel_id.lsm_id,
			       &p_rstatus->status);
		
		st_convert_response_status(p_rstatus);
	    }
	    break;
	    
	  default:
	    /* We may be returning a packet with an invalid type,
	       so don't treat this as an error condition   */
	    break;
	    
	}   /* End switch */
    }       /* End else from message_options & INTERMEDIATE */
}

/* -----------------------------------------------------------------------
 * Name:  st_response_clear_lock
 *
 * Description:
 *
 *      This function converts a clear lock response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_clear_lock (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;           /* Loop variables         */
    RESPONSE_STATUS   *p_rstatus;
	    
    if (resp_ptr->clear_lock_response.type == TYPE_DRIVE) {
	
	for (i=0; i < MAX_ID && i < (int)resp_ptr->clear_lock_response.count; i++) {
	    
	    p_rstatus = &resp_ptr->clear_lock_response.
		identifier_status.drive_status[i].status;
	    
	    st_convert_drv(&resp_ptr->clear_lock_response.
			   identifier_status.drive_status[i].drive_id,
			   &p_rstatus->status);
	    
	    /* Convert anything in the individual response statuses */
	    st_convert_response_status(p_rstatus);
	}
    }
    else if (resp_ptr->clear_lock_response.type == TYPE_VOLUME) {
	
	for (i=0; i < MAX_ID && i < (int)resp_ptr->clear_lock_response.count; i++) {
	    
	    p_rstatus = &resp_ptr->clear_lock_response.
		identifier_status.volume_status[i].status;
	    
	    st_convert_response_status(p_rstatus);
	}
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_define_pool
 *
 * Description:
 *
 *      This function converts a define pool response from VERSION3
 *      to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_define_pool (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;           /* Loop variables         */

    for (i=0; i< MAX_ID && i < (int)resp_ptr->define_pool_response.count; i++) {
	
	st_convert_pool(&resp_ptr->define_pool_response.pool_status[i].pool_id);
	
	st_convert_response_status(&resp_ptr->define_pool_response.
				   pool_status[i].status);
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_delete_pool
 *
 * Description:
 *
 *      This function converts a delete pool response from VERSION3
 *      to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_delete_pool (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;           /* Loop variables         */

    for (i=0; i< MAX_ID && i < (int)resp_ptr->delete_pool_response.count; i++) {
	
	st_convert_pool(&resp_ptr->delete_pool_response.pool_status[i].
			pool_id);
	
	st_convert_response_status(&resp_ptr->delete_pool_response.
				   pool_status[i].status);
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_dismount
 *
 * Description:
 *
 *      This function converts a dismount response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_dismount (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    
    st_convert_drv(&resp_ptr->dismount_response.drive_id,
		   &resp_ptr->dismount_response.message_status.status);
}

/* -----------------------------------------------------------------------
 * Name:  st_response_eject
 *
 * Description:
 *
 *      This function converts an eject response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_eject (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;
    
    st_convert_cap(&resp_ptr->eject_response.cap_id,
		   &resp_ptr->eject_response.message_status.status);
    
    for (i=0; i < MAX_ID && i < (int)resp_ptr->eject_response.count; i++) {
	
	st_convert_response_status(&resp_ptr->eject_response.
				   volume_status[i].status);
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_enter
 *
 * Description:
 *
 *      This function converts an enter response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_enter (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int  i;
    
    st_convert_cap(&resp_ptr->enter_response.cap_id,
		   &resp_ptr->enter_response.message_status.status);
    
    for (i=0; i < MAX_ID && i < (int)resp_ptr->enter_response.count; i++) {
	
	st_convert_response_status(&resp_ptr->enter_response.
				   volume_status[i].status);
    }
     
}

/* -----------------------------------------------------------------------
 * Name:  st_response_lock
 *
 * Description:
 *
 *      This function converts a lock response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_lock (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;
    RESPONSE_STATUS   *p_rstatus;
    
    if (resp_ptr->lock_response.type == TYPE_DRIVE) {
	
	for (i=0; i < MAX_ID && i < (int)resp_ptr->lock_response.count; i++) {
	    
	    p_rstatus = &resp_ptr->lock_response.
		identifier_status.drive_status[i].status;
	    
	    st_convert_drv(&resp_ptr->lock_response.identifier_status.
			   drive_status[i].drive_id,
			   &p_rstatus->status);
	    
	    /* Convert anything in the individual response statuses */
	    st_convert_response_status(p_rstatus);
	}
    }	
    else if (resp_ptr->lock_response.type == TYPE_VOLUME) {
	
	for (i=0; i < MAX_ID && i < (int)resp_ptr->lock_response.count; i++) {
	    
	      p_rstatus = &resp_ptr->lock_response.
		  identifier_status.volume_status[i].status;
	      
	      st_convert_response_status(p_rstatus);
	}
    }
}   

/* -----------------------------------------------------------------------
 * Name:  st_response_mount
 *
 * Description:
 *
 *      This function converts a mount response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_mount (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    RESPONSE_STATUS   *p_rstatus;
    
    p_rstatus = &resp_ptr->mount_response.message_status;
    
    st_convert_drv(&resp_ptr->mount_response.drive_id,
		   &p_rstatus->status);
    
}

/* -----------------------------------------------------------------------
 * Name:  st_response_mount_scratch
 *
 * Description:
 *
 *      This function converts a mount scratch response from VERSION3
 *	to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_mount_scratch (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    RESPONSE_STATUS   *p_rstatus;
    
    p_rstatus = &resp_ptr->mount_scratch_response.message_status;
    
    st_convert_pool(&resp_ptr->mount_scratch_response.pool_id);
    st_convert_drv(&resp_ptr->mount_scratch_response.drive_id,
		   &p_rstatus->status);
    
}

/* -----------------------------------------------------------------------
 * Name:  st_response_query
 *
 * Description:
 *
 *      This function converts a query response from VERSION3 to VERSION2.
 *      Note that since we are only converting ANY_, ALL_, SAME_ values,
 *      and these are not generated by Query, we don't make any attempt to
 *      convert data that is generated by Query. We only convert data
 *      that was originally sent to us by the client in the request message,
 *      and possibly some of the statuses we generate.
 *
 *      Note that we do nothing with generated values that might be out of
 *      range for a down-level system.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_query (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;           /* Loop variables         */
    V2_QUERY_RESPONSE *rp;          /* Request pointer - Used to
				     * make code clearer      */
    
    rp = &resp_ptr->query_response;
    
    switch (rp->type) {
	
      case TYPE_ACS:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++)
	    st_convert_acs(&rp->status_response.acs_status[i].acs_id);
	break;
	
      case TYPE_CAP:
	
	/* Don't convert cap_priority because this is generated by Query */
	
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {
	    st_convert_cap(&rp->status_response.cap_status[i].cap_id,
			   &rp->status_response.cap_status[i].status);
        }
	break;
	
      case TYPE_CLEAN:          /* No conversion */ 
	break;
	
      case TYPE_DRIVE:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {
	    st_convert_drv(&rp->status_response.drive_status[i].drive_id,
			   &rp->status_response.drive_status[i].status);
        }
	break;
	
      case TYPE_LSM:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {
	    st_convert_lsm(&rp->status_response.lsm_status[i].lsm_id,
			   &rp->status_response.lsm_status[i].status);
        }
	break;
	
      case TYPE_MOUNT:          /* No conversion */
	
	/* Don't convert drive IDs because these are generated by Query */
	break;
	
      case TYPE_MOUNT_SCRATCH:
	
	/* Don't convert drive IDs because these are generated by Query */
	
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) 	    
	    st_convert_pool(&rp->status_response.mount_scratch_status[i].
			    pool_id);
	break;
	
      case TYPE_POOL:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++)
	    st_convert_pool(&rp->status_response.pool_status[i].pool_id);
	break;
	
      case TYPE_PORT:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++)
	    st_convert_acs(&rp->status_response.port_status[i].port_id.acs);
	break;
	
      case TYPE_REQUEST:          /* No conversion */
	break;
	
      case TYPE_SCRATCH:
	
	/* Don't convert the lsm_id because this is generated by Query  */
	
	for (i=0; i < MAX_ID && i < (int)rp->count; i++)
	    st_convert_pool(&rp->status_response.scratch_status[i].pool_id);
	break;
	
      case TYPE_SERVER:          /* No conversion */
	break;
	
      case TYPE_VOLUME:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {
	    st_convert_status (&rp->status_response.volume_status[i].status);
	}
	
	/* Don't convert the lsm_id or the drive IDs because these are
	   generated by Query */
	break;
	
      default:
	/* We may be returning a packet with an invalid type,
	   so don't treat this as an error condition   */
	break;
	
    }   /* End switch */
}

/* -----------------------------------------------------------------------
 * Name:  st_response_query_lock
 *
 * Description:
 *
 *      This function converts a query lock response from VERSION3
 *	to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_query_lock (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;
    
    if (resp_ptr->query_lock_response.type == TYPE_DRIVE) {
	
	for (i=0; i < MAX_ID && i < (int)resp_ptr->query_lock_response.count; i++)
	    
	    st_convert_drv(&resp_ptr->query_lock_response.
			   identifier_status.drive_status[i].drive_id,
			   &resp_ptr->query_lock_response.
			   identifier_status.drive_status[i].status);
    }	
    
    /* Note that we do not need to convert volumes or volume statuses
       because Query Lock is not controlled by Access Control. */
}

/* -----------------------------------------------------------------------
 * Name:  st_response_set_cap
 *
 * Description:
 *
 *      This function converts a set cap response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_set_cap (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;
    RESPONSE_STATUS   *p_rstatus;
    
    st_convert_cap_priority(&resp_ptr->set_cap_response.cap_priority);
    
    for (i=0; i < MAX_ID && i < (int)resp_ptr->set_cap_response.count; i++) {
	
	p_rstatus = &resp_ptr->set_cap_response.set_cap_status[i].status;
	
	st_convert_cap(&resp_ptr->set_cap_response.set_cap_status[i].cap_id,
		       &p_rstatus->status);
	
	/* Convert anything in the individual response statuses */
	st_convert_response_status(p_rstatus);
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_set_clean
 *
 * Description:
 *
 *      This function converts a set clean response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_set_clean (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;
    
    for (i=0; i < MAX_ID && i < (int)resp_ptr->set_clean_response.count; i++) {
	
	st_convert_response_status(&resp_ptr->set_clean_response.
				   volume_status[i].status);
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_set_scratch
 *
 * Description:
 *
 *      This function converts a set scratch response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_set_scratch (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;
    
    st_convert_pool(&resp_ptr->set_scratch_response.pool_id);
    
    for (i=0; i< MAX_ID && i < (int)resp_ptr->set_scratch_response.count; i++) {
	
	st_convert_response_status(&resp_ptr->set_scratch_response.
				   scratch_status[i].status);
	
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_unlock
 *
 * Description:
 *
 *      This function converts an unlock response from VERSION3 to VERSION2.
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_unlock (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int                i;
    RESPONSE_STATUS   *p_rstatus;
    
    if (resp_ptr->unlock_response.type == TYPE_DRIVE) {
	
	for (i=0; i < MAX_ID && i < (int)resp_ptr->unlock_response.count; i++) {
	    
	    p_rstatus = &resp_ptr->unlock_response.
		identifier_status.drive_status[i].status;
	    
	    st_convert_drv(&resp_ptr->unlock_response.identifier_status.
			   drive_status[i].drive_id,
			   &p_rstatus->status);
	    
	    /* Convert anything in the individual response statuses */
	   st_convert_response_status(p_rstatus);
	}
    }	
    else if (resp_ptr->unlock_response.type == TYPE_VOLUME) {
	
	for (i=0; i < MAX_ID && i < (int)resp_ptr->unlock_response.count; i++) {
	    
	    st_convert_response_status(&resp_ptr->unlock_response.
				       identifier_status.volume_status[i].
				       status);
	}
    }
}

/* -----------------------------------------------------------------------
 * Name:  st_response_vary
 *
 * Description:
 *
 *      This function converts a vary response from VERSION3 to VERSION2
 *
 * Parameters:
 *
 *      V2_RESPONSE_TYPE *resp_ptr    Pointer to V2_RESPONSE_TYPE structure.
 *
 */

static void 
st_response_vary (
    V2_RESPONSE_TYPE *resp_ptr         /* pointer to request type structure    */
)
{
    int               i;            /* Loop variable           */
    V2_VARY_RESPONSE *rp;           /* Request pointer - Used to
				     * make code clearer       */
    RESPONSE_STATUS  *p_rstatus;
	    
    rp = &resp_ptr->vary_response;
    
    switch (rp->type) {
	
      case TYPE_ACS:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {

	    st_convert_acs(&rp->device_status.acs_status[i].acs_id);
	    
	    st_convert_response_status(&rp->device_status.acs_status[i].status);
	}
	break;
	
      case TYPE_CAP:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {
	    p_rstatus = &rp->device_status.cap_status[i].status;

	    st_convert_cap(&rp->device_status.cap_status[i].cap_id,
			   &p_rstatus->status);
	    
	    st_convert_response_status(p_rstatus);
	}
	break;
	
      case TYPE_DRIVE:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {
	    p_rstatus = &rp->device_status.drive_status[i].status;
	    
	    st_convert_drv(&rp->device_status.drive_status[i].drive_id,
			   &p_rstatus->status);
	    
	    st_convert_response_status(p_rstatus);
	}
	break;
	
      case TYPE_LSM:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {
	    p_rstatus = &rp->device_status.lsm_status[i].status;
	    
	    st_convert_lsm(&rp->device_status.lsm_status[i].lsm_id,
			   &p_rstatus->status);
	    
	    st_convert_response_status(p_rstatus);
	}
	break;
	
      case TYPE_PORT:
	for (i=0; i < MAX_ID && i < (int)rp->count; i++) {

	    st_convert_acs(&rp->device_status.port_status[i].port_id.acs);

	    st_convert_response_status(&rp->device_status.port_status[i].
				       status);
	
	}
	break;
	
      default:
	/* We may be returning a packet with an invalid type,
	   so don't treat this as an error condition   */
	break;
	
    }   /* End switch */
}
