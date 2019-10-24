#include "acssys.h"
#ifndef lint
static char SccsId[]= "@(#) %full_name:	1/csrc/acs_cvt_v3_v2.c/2.1.2 %";
#endif
/**PROLOGUE**********************************************************
**
** Copyright (c) C, 2011) Oracle and/or its affiliates.  All rights reserved.
**
**  FUNCTION TITLE:  Convert version 3 request packet format to version
**                   2 packet format.
**
**  FUNCTION NAME:  STATUS acs_cvt_v3_v2(V3_REQUEST_TYPE *req_ptr,
**                                      int   *byte_count)
**
**  FUNCTION DESCRIPTION:
**     This routine will convert a VERSION3 packet to a VERSION2 packet.
**
**     Convert the packet if necessary. The size and structure of the
**     packet does not change - only some of the values. In particular,
**     Version 3 has new values for:
**         ANY_ACS
**         ANY_LSM
**         ANY_CAP
**         ALL_CAP
**         SAME_PRIORITY
**         SAME_POOL
**     Thus, whenever we find the new values, we convert them to the old
**     values. The old values are in #defines with the prefix V2_ and
**     these are found in v2_structs.h.  For example V2_ANY_ACS.
**
**     Return the new packet and its length to the input process. The
**     length is unchanged, but we retain the variable for consistency.
**
**  SERIALIZATION REQUIREMENTS:  None
**
**  ATTRIBUTES:
**     Reentrant
**
**  MODULE ENTRY:
**     INPUT:
**            V3_REQUEST_TYPE *req_ptr
**                             Pointer to input buffer.  The buffer is
**                             written to directly for output. Note that
**                             the REQUEST_TYPE structure is identical
**
**            int *byte_count  Pointer to number of bytes in the
**                             request packet. This value is used both
**                             as input and as output.
**  MODULE EXIT:
**     OUTPUT: format 2 packet
**     RETURN CODES:  STATUS_SUCCESS
**                    STATUS_INVALID_COMMAND (Buffer has invalid cmd)
**                    STATUS_INVALID_TYPE (Request has invalid id type)
**                    STATUS_PROCESS_FAILURE (Software failure)
**
**  GLOBAL DATA:  None
**
**  MESSAGES GENERATED:  None
**
**  SPECIAL CONDITIONS:
**      The transformations are simply changing the values of
**      variables.  There are no structure changes.  Thus, to
**      save time we simply write directly onto the input
**      buffer.  Note also that this routine is called before
**      any specific validation is done.
**
**  CHANGE HISTORY:
**      Bernie Wolf         18-Jan-1993  Original             @R222
**      Ken Stickney        16-Jun-1994  Deleted ml_pub.h include 
**      Ken Stickney        17-Dec-1994  Removed v2_structs.h, lh_defs.h,
**                                       and acslm.h. Replaced with
**                                       acscvt.h. For AS400 portablility.
**                                       acscvt.h supplies packet
**                                       defintions independent of other
**                                       system/platform data
**
***ENDPROLOGUE*******************************************************/

/*
 *      Header Files:
 */
#include <stdio.h>
#include <string.h>
#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

#include "acscvt.h"

/*
 *      Global and Static Variable Declarations:
 */
#undef SELF
#define SELF "acs_cvt_v3_v2"

/*
 * Procedure declarations
 */

static void st_convert_acs(ACS * p_acs);
static void st_convert_cap(CAPID * p_capid);
static void st_convert_cap_priority(CAP_PRIORITY * p_cap_priority);
static void st_convert_drv(DRIVEID * p_driveid);
static void st_convert_lsm(LSMID * p_lsmid);
static void st_convert_pool(POOLID * p_poolid);

static STATUS st_request_audit(V3_REQUEST_TYPE * req_ptr);
static STATUS st_request_query(V3_REQUEST_TYPE * req_ptr);
static STATUS st_request_vary(V3_REQUEST_TYPE * req_ptr);



STATUS
acs_cvt_v3_v2(ALIGNED_BYTES req_ptr,
	      size_t * byte_count)
{
    unsigned short i;			/* Loop counter              */
    STATUS status = STATUS_SUCCESS;
    V3_REQUEST_TYPE * v3_req_ptr;

    v3_req_ptr = (V3_REQUEST_TYPE *)req_ptr;

    acs_trace_entry();


    /* Change the version number */
    v3_req_ptr->generic_request.message_header.version = VERSION2;

    /* Do the appropriate thing for the request type */
    switch (v3_req_ptr->generic_request.message_header.command) {

    case COMMAND_AUDIT:
	status = st_request_audit(v3_req_ptr);
	break;

    case COMMAND_CANCEL:	/* No changes needed */
	break;

    case COMMAND_CLEAR_LOCK:
	if (v3_req_ptr->clear_lock_request.type == TYPE_DRIVE) {
	    for (i = 0; i < MAX_ID && i < v3_req_ptr->clear_lock_request.count; i++)
		st_convert_drv(
		    &v3_req_ptr->clear_lock_request.identifier.drive_id[i]);
	}
	/* Note that volumes don't need to be converted */
	break;

    case COMMAND_DEFINE_POOL:
	for (i = 0; i < MAX_ID && i < v3_req_ptr->define_pool_request.count; i++) {
	    st_convert_pool(&v3_req_ptr->define_pool_request.pool_id[i]);
	}
	break;

    case COMMAND_DELETE_POOL:
	for (i = 0; i < MAX_ID && i < v3_req_ptr->delete_pool_request.count; i++) {
	    st_convert_pool(&v3_req_ptr->delete_pool_request.pool_id[i]);
	}
	break;

    case COMMAND_DISMOUNT:
	st_convert_drv(&v3_req_ptr->dismount_request.drive_id);
	break;

    case COMMAND_EJECT:
	if (v3_req_ptr->generic_request.message_header.extended_options & RANGE)
	    st_convert_cap(&v3_req_ptr->ext_eject_request.cap_id);
	else
	    st_convert_cap(&v3_req_ptr->eject_request.cap_id);
	break;

    case COMMAND_ENTER:
	if (v3_req_ptr->generic_request.message_header.extended_options & VIRTUAL)
	    st_convert_cap(&v3_req_ptr->venter_request.cap_id);
	else
	    st_convert_cap(&v3_req_ptr->enter_request.cap_id);
	break;

    case COMMAND_IDLE:		/* No changes needed */
	break;

    case COMMAND_LOCK:
	if (v3_req_ptr->lock_request.type == TYPE_DRIVE) {
	    for (i = 0; i < MAX_ID && i < v3_req_ptr->lock_request.count; i++)
		st_convert_drv(&v3_req_ptr->lock_request.identifier.drive_id[i]);
	}
	/* Note that volumes don't need to be converted */
	break;

    case COMMAND_MOUNT:
	for (i = 0; i < MAX_ID && i < v3_req_ptr->mount_request.count; i++)
	    st_convert_drv(&v3_req_ptr->mount_request.drive_id[i]);
	break;

    case COMMAND_MOUNT_SCRATCH:
	st_convert_pool(&v3_req_ptr->mount_scratch_request.pool_id);
	for (i = 0; i < MAX_ID && i < v3_req_ptr->mount_scratch_request.count; i++)
	    st_convert_drv(&v3_req_ptr->mount_scratch_request.drive_id[i]);
	break;

    case COMMAND_QUERY:
	status = st_request_query(v3_req_ptr);
	break;

    case COMMAND_QUERY_LOCK:
	if (v3_req_ptr->query_lock_request.type == TYPE_DRIVE) {
	    for (i = 0; i < MAX_ID && i < v3_req_ptr->query_lock_request.count; i++)
		st_convert_drv(
		    &v3_req_ptr->query_lock_request.identifier.drive_id[i]);
	}
	/* Note that volumes don't need to be converted */
	break;

    case COMMAND_SET_CAP:
	st_convert_cap_priority(&v3_req_ptr->set_cap_request.cap_priority);
	for (i = 0; i < MAX_ID && i < v3_req_ptr->set_cap_request.count; i++)
	    st_convert_cap(&v3_req_ptr->set_cap_request.cap_id[i]);
	break;

    case COMMAND_SET_CLEAN:	/* No changes needed */
	break;

    case COMMAND_SET_SCRATCH:
	st_convert_pool(&v3_req_ptr->set_scratch_request.pool_id);
	break;

    case COMMAND_START:	/* No changes needed */
	break;

    case COMMAND_UNLOCK:
	if (v3_req_ptr->unlock_request.type == TYPE_DRIVE) {
	    for (i = 0; i < MAX_ID && i < v3_req_ptr->unlock_request.count; i++)
		st_convert_drv(&v3_req_ptr->unlock_request.identifier.drive_id[i]);
	}
	/* Note that volumes don't need to be converted */
	break;

    case COMMAND_VARY:
	status = st_request_vary(v3_req_ptr);
	break;

    default:
	status = STATUS_INVALID_COMMAND;
    }

    acs_trace_exit(status);
    return (status);
}

/*+ -----------------------------------------------------------------
 * Name:
 *
 *      st_convert_acs
 *
 * Description:
 *
 *      This function checks the ACS value and converts the value
 *      in place if necessary.
 *
 * Return Values:       NONE
 *
 * Parameters:
 *
 *      ACS   p_acs     Pointer to ACS value
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 *+
 */

static void st_convert_acs(ACS * p_acs)
{
    if (*p_acs == ANY_ACS)
	*p_acs = V2_ANY_ACS;
}

/*+ --------------------------------------------------------------------
 * Name:
 *
 *      st_convert_cap
 *
 * Description:
 *
 *      This function checks the CAP, LSM, and ACS values in a cap_id and
 *      converts the values in place if necessary.
 *
 * Return Values:       NONE
 *
 * Parameters:
 *
 *      CAPID  p_capid  Pointer to cap ID.
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 *+
 */

static void st_convert_cap(p_capid)
CAPID *p_capid;
{
    if (p_capid->cap == ANY_CAP)
	p_capid->cap = V2_ANY_CAP;
    else if (p_capid->cap == ALL_CAP)
	p_capid->cap = V2_ALL_CAP;

    if (p_capid->lsm_id.acs == ANY_ACS)
	p_capid->lsm_id.acs = V2_ANY_ACS;

    if (p_capid->lsm_id.lsm == ANY_LSM)
	p_capid->lsm_id.lsm = V2_ANY_LSM;
}

/*+ --------------------------------------------------------------------
 * Name:
 *
 *      st_convert_cap_priority
 *
 * Description:
 *
 *      This module checks the cap priority and converts the value in
 *      place if necessary.
 *
 * Return Values:       NONE
 *
 * Parameters:
 *
 *      CAP_PRIORITY    *p_cap_priority
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 *+
 */

static void st_convert_cap_priority(p_cap_priority)
CAP_PRIORITY *p_cap_priority;
{
    if (*p_cap_priority == SAME_PRIORITY)
	*p_cap_priority = V2_SAME_PRIORITY;
}

/*+ -------------------------------------------------------------------
 * Name:
 *
 *      st_convert_drv
 *
 * Description:
 *
 *      This module checks the LSM, and ACS values in an drive_id and
 *      converts the values in place if necessary.  The same functionality
 *      could be provided by a call to st_convert_lsm, but having this
 *      st_convert_drive call makes the calling code a little easier to
 *      read by allowing a shorter variable name in the function call.
 *
 * Return Values:           NONE
 *
 * Parameters:
 *
 *      DRIVEID p_driveid   Pointer to drive ID
 *
 * Implicit Inputs:         NONE
 *
 * Implicit Outputs:        NONE
 *
 * Considerations:          NONE
 *
 *+
 */

static void st_convert_drv(p_driveid)
DRIVEID *p_driveid;
{
    if (p_driveid->panel_id.lsm_id.acs == ANY_ACS)
	p_driveid->panel_id.lsm_id.acs = V2_ANY_ACS;

    if (p_driveid->panel_id.lsm_id.lsm == ANY_LSM)
	p_driveid->panel_id.lsm_id.lsm = V2_ANY_LSM;
}

/*+ -------------------------------------------------------------------
 * Name:
 *
 *      st_convert_lsm
 *
 * Description:
 *
 *      This module checks the LSM, and ACS values in an lsm_id and
 *      converts the values in place if necessary.
 *
 * Return Values:       NONE
 *
 * Parameters:
 *
 *      LSMID p_lsmid   Pointer to LSMID.
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 *+
 */

static void st_convert_lsm(LSMID * p_lsmid)
{
    if (p_lsmid->acs == ANY_ACS)
	p_lsmid->acs = V2_ANY_ACS;

    if (p_lsmid->lsm == ANY_LSM)
	p_lsmid->lsm = V2_ANY_LSM;
}

/*+ -------------------------------------------------------------------
 * Name:
 *
 *      st_convert_pool
 *
 * Description:
 *
 *      This module checks the pool ID and converts the value in
 *      place if necessary.
 *
 * Return Values:       NONE
 *
 * Parameters:
 *
 *      POOLID p_poolid   Pointer to POOLID
 *
 * Implicit Inputs:       NONE
 *
 * Implicit Outputs:      NONE
 *
 * Considerations:        NONE
 *
 *+
 */
static void st_convert_pool(POOLID * p_poolid)
{
    if (p_poolid->pool == SAME_POOL)
	p_poolid->pool = V2_SAME_POOL;
}

/*+ -----------------------------------------------------------------------
 * Name:
 *
 *      st_request_audit
 *
 * Description:
 *
 *      This module converts an audit request from VERSION3 to VERSION2
 *
 * Return Values:       STATUS_SUCCESS
 *                      STATUS_INVALID_TYPE
 *
 * Parameters:
 *
 *      V2_REQUEST_TYPE *req_ptr    Pointer to V3_REQUEST_TYPE structure.
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 *+
 */

static STATUS
 st_request_audit(V3_REQUEST_TYPE * req_ptr)
{				/* pointer to request type structure    */
    unsigned short i;			/* Loop variable           */
    V3_AUDIT_REQUEST *rp;	/* Request pointer - Used to
				   * make code clearer       */

    rp = &req_ptr->audit_request;

    /* --------- Convert the cap_id if necessary ----------- */

    st_convert_cap(&rp->cap_id);


    /* ----------- Convert the thing being audited ---------- */

    switch (rp->type) {

    case TYPE_ACS:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_acs(&rp->identifier.acs_id[i]);
	break;

    case TYPE_LSM:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_lsm(&rp->identifier.lsm_id[i]);
	break;

    case TYPE_PANEL:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_lsm(&rp->identifier.panel_id[i].lsm_id);
	break;

    case TYPE_SERVER:		/* No changes needed */
	break;

    case TYPE_SUBPANEL:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_lsm(&rp->identifier.subpanel_id[i].panel_id.lsm_id);
	break;

    case TYPE_MIXED_MEDIA_INFO:
      /* Note that this type is new for Version 4 packets, so there is
       * nothing to translate this request into. Just send it on to the
       * server and let it reject the request with STATUS_INVALID TYPE.
       */
      break;  

    default:
	return (STATUS_INVALID_TYPE);

    }				/* End switch */

    return (STATUS_SUCCESS);
}

/*+ -----------------------------------------------------------------------
 * Name:
 *
 *      st_request_query
 *
 * Description:
 *
 *      This module converts a query request from VERSION3 to VERSION2
 *
 * Return Values:       STATUS_SUCCESS
 *                      STATUS_INVALID_TYPE
 *
 * Parameters:
 *
 *      V3_REQUEST_TYPE *req_ptr    Pointer to V3_REQUEST_TYPE structure.
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 *+
 */

static STATUS
 st_request_query(V3_REQUEST_TYPE * req_ptr)
{				/* pointer to request type structure    */
    unsigned short i;			/* Loop variable           */
    V3_QUERY_REQUEST *rp;	/* Request pointer - Used to
				   * make code clearer       */

    rp = &req_ptr->query_request;

    switch (rp->type) {

    case TYPE_ACS:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_acs(&rp->identifier.acs_id[i]);
	break;

    case TYPE_CAP:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_cap(&rp->identifier.cap_id[i]);
	break;

    case TYPE_CLEAN:		/* No conversion of volid */
	break;

    case TYPE_DRIVE:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_drv(&rp->identifier.drive_id[i]);
	break;

    case TYPE_LSM:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_lsm(&rp->identifier.lsm_id[i]);
	break;

    case TYPE_MIXED_MEDIA_INFO:
       /* Note that this type is new for Version 4 packets, so there is
        * nothing to translate this request into. Just send it on to the
        * server and let it reject the request with STATUS_INVALID TYPE.
        */
        break;

    case TYPE_MOUNT:		/* No conversion of volid */
	break;

    case TYPE_MOUNT_SCRATCH:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_pool(&rp->identifier.pool_id[i]);
	break;

    case TYPE_POOL:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_pool(&rp->identifier.pool_id[i]);
	break;

    case TYPE_PORT:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_acs(&rp->identifier.port_id[i].acs);
	break;

    case TYPE_REQUEST:		/* No conversion */
	break;

    case TYPE_SCRATCH:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_pool(&rp->identifier.pool_id[i]);
	break;

    case TYPE_SERVER:		/* No conversion */
	break;

    case TYPE_VOLUME:		/* No conversion of volid */
	break;


    default:
	return (STATUS_INVALID_TYPE);

    }				/* End switch */

    return (STATUS_SUCCESS);
}

/*+ -----------------------------------------------------------------------
 * Name:
 *
 *      st_request_vary
 *
 * Description:
 *
 *      This module converts a vary request from VERSION3 to VERSION2
 *
 * Return Values:       STATUS_SUCCESS
 *                      STATUS_INVALID_TYPE
 *
 * Parameters:
 *
 *      V2_REQUEST_TYPE *req_ptr    Pointer to V2_REQUEST_TYPE structure.
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 *+
 */
static STATUS
 st_request_vary(V3_REQUEST_TYPE * req_ptr)
{				/* pointer to request type structure    */
    unsigned short i;			/* Loop variable             */
    V3_VARY_REQUEST *rp;	/* Request pointer - Used to
				   * make code clearer         */

    rp = &req_ptr->vary_request;

    switch (rp->type) {

    case TYPE_ACS:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_acs(&rp->identifier.acs_id[i]);
	break;

    case TYPE_CAP:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_cap(&rp->identifier.cap_id[i]);
	break;

    case TYPE_DRIVE:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_drv(&rp->identifier.drive_id[i]);
	break;

    case TYPE_LSM:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_lsm(&rp->identifier.lsm_id[i]);
	break;

    case TYPE_PORT:
	for (i = 0; i < MAX_ID && i < rp->count; i++)
	    st_convert_acs(&rp->identifier.port_id[i].acs);
	break;

    default:
	return (STATUS_INVALID_TYPE);

    }				/* End switch */

    return (STATUS_SUCCESS);
}
